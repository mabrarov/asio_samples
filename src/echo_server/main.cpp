//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <tchar.h>
#include <windows.h>
#include <iostream>
#include <boost/smart_ptr.hpp>
#include <boost/ref.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <console_controller.hpp>

struct session_manager_data;
typedef boost::shared_ptr<session_manager_data> session_manager_data_ptr;
typedef boost::function<void (void)> exception_handler;

struct session_manager_data : private boost::noncopyable
{  
  enum state
  {
    ready_to_start,
    start_in_progress,
    started,
    stop_in_progress,
    stopped
  };

  boost::mutex mutex_;  
  ma::echo::server::session_manager_ptr session_manager_;
  state state_;
  bool stopped_by_program_exit_;
  boost::condition_variable state_changed_;  
  ma::in_place_handler_allocator<256> start_wait_allocator_;
  ma::in_place_handler_allocator<256> stop_allocator_;

  explicit session_manager_data(boost::asio::io_service& io_service,
    boost::asio::io_service& session_io_service,
    const ma::echo::server::session_manager::settings& settings)
    : session_manager_(new ma::echo::server::session_manager(io_service, session_io_service, settings))    
    , state_(ready_to_start)
    , stopped_by_program_exit_(false)
  {
  }

  ~session_manager_data()
  {
  }
}; // session_manager_data

void start_session_manager(const session_manager_data_ptr&);

void wait_session_manager(const session_manager_data_ptr&);

void stop_session_manager(const session_manager_data_ptr&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const session_manager_data_ptr&);

void handle_program_exit(const session_manager_data_ptr&);

void handle_session_manager_start(const session_manager_data_ptr&,
  const boost::system::error_code&);

void handle_session_manager_wait(const session_manager_data_ptr&,
  const boost::system::error_code&);

void handle_session_manager_stop(const session_manager_data_ptr&,
  const boost::system::error_code&);

int _tmain(int argc, _TCHAR* argv[])
{ 
  boost::program_options::options_description options_description("Allowed options");
  options_description.add_options()
    (
      "help", 
      "produce help message"
    )
    (
      "port", 
      boost::program_options::value<unsigned short>(), 
      "set TCP port number for to listen for incoming connections"
    )
    (
      "stop_timeout", 
      boost::program_options::value<long>()->default_value(60), 
      "set server stop timeout, at one's expiration server work will be terminated (seconds)"
    )
    (
      "max_sessions", 
      boost::program_options::value<std::size_t>()->default_value(10000), 
      "set maximum number of simultaneously active sessions"
    )
    (
      "recycled_sessions", 
      boost::program_options::value<std::size_t>()->default_value(100), 
      "set maximum number of inactive sessions used for new sessions' acceptation"
    )
    (
      "listen_backlog", 
      boost::program_options::value<int>()->default_value(6), 
      "set size of TCP listen backlog"
    )
    (
      "buffer_size", 
      boost::program_options::value<std::size_t>()->default_value(1024),
      "set size of the session's buffer (bytes)"
    );

  int exit_code = EXIT_SUCCESS;
  try 
  {
    boost::program_options::variables_map options_values;  
    boost::program_options::store(
      boost::program_options::parse_command_line(argc, argv, options_description), 
      options_values);
    boost::program_options::notify(options_values);

    if (options_values.count("help"))
    {
      std::cout << options_description;
    }
    else if (!options_values.count("port"))
    {    
      exit_code = EXIT_FAILURE;
      std::cout << "Port not set.\n" << options_description;
    }
    else
    {        
      std::size_t cpu_count = boost::thread::hardware_concurrency();
      std::size_t session_thread_count = cpu_count ? cpu_count : 2;
      std::size_t session_manager_thread_count = 1;

      unsigned short listen_port = options_values["port"].as<unsigned short>();
      boost::asio::ip::tcp::endpoint listen_endpoint(boost::asio::ip::tcp::v4(), listen_port);
      boost::posix_time::time_duration stop_timeout = 
        boost::posix_time::seconds(options_values["stop_timeout"].as<long>());

      ma::echo::server::session_manager::settings server_settings(
        listen_endpoint,
        options_values["max_sessions"].as<std::size_t>(),
        options_values["recycled_sessions"].as<std::size_t>(),
        options_values["listen_backlog"].as<int>(),
        ma::echo::server::session::settings(options_values["buffer_size"].as<std::size_t>()));      

      std::cout << "Number of found CPU(s)             : " << cpu_count << '\n'               
                << "Number of session manager's threads: " << session_manager_thread_count << '\n'
                << "Number of sessions' threads        : " << session_thread_count << '\n' 
                << "Total number of work threads       : " << session_thread_count + session_manager_thread_count << '\n'
                << "Server listen port                 : " << listen_port << '\n'
                << "Server stop timeout (seconds)      : " << stop_timeout.total_seconds() << '\n'
                << "Maximum number of active sessions  : " << server_settings.max_sessions_ << '\n'
                << "Maximum number of recycled sessions: " << server_settings.recycled_sessions_ << '\n'
                << "TCP listen backlog size            : " << server_settings.listen_backlog_ << '\n'
                << "Size of session's buffer (bytes)   : " << server_settings.session_settings_.buffer_size_ << '\n';
      
      // Before session_manager_io_service
      boost::asio::io_service session_io_service;
      // ... for the right destruction order
      boost::asio::io_service session_manager_io_service;

      // Create session_manager
      session_manager_data_ptr sm_data(
        new session_manager_data(session_manager_io_service, session_io_service, server_settings));
      
      std::cout << "Server is starting...\n";
      
      // Start the server
      boost::unique_lock<boost::mutex> server_data_lock(sm_data->mutex_);    
      start_session_manager(sm_data);
      server_data_lock.unlock();
      
      // Setup console controller
      ma::console_controller console_controller(
        boost::bind(handle_program_exit, sm_data));

      std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

      // Exception handler for automatic server aborting
      exception_handler work_exception_handler(
        boost::bind(handle_work_exception, sm_data));

      // Create work for sessions' io_service to prevent threads' stop
      boost::asio::io_service::work session_work(session_io_service);
      // Create work for session_manager's io_service to prevent threads' stop
      boost::asio::io_service::work session_manager_work(session_manager_io_service);    

      // Create work threads for session operations
      boost::thread_group work_threads;    
      for (std::size_t i = 0; i != session_thread_count; ++i)
      {
        work_threads.create_thread(
          boost::bind(run_io_service, boost::ref(session_io_service), 
            work_exception_handler));
      }
      // Create work threads for session manager operations
      for (std::size_t i = 0; i != session_manager_thread_count; ++i)
      {
        work_threads.create_thread(
          boost::bind(run_io_service, boost::ref(session_manager_io_service), 
            work_exception_handler));      
      }

      // Wait until server stops
      server_data_lock.lock();
      while (session_manager_data::stop_in_progress != sm_data->state_ 
        && session_manager_data::stopped != sm_data->state_)
      {
        sm_data->state_changed_.wait(server_data_lock);
      }
      if (session_manager_data::stopped != sm_data->state_)
      {
        if (!sm_data->state_changed_.timed_wait(server_data_lock, stop_timeout))      
        {
          std::cout << "Server stop timeout expiration. Terminating server work...\n";
          exit_code = EXIT_FAILURE;
        }
        sm_data->state_ = session_manager_data::stopped;
      }
      if (!sm_data->stopped_by_program_exit_)
      {
        exit_code = EXIT_FAILURE;
      }
      server_data_lock.unlock();
          
      session_manager_io_service.stop();
      session_io_service.stop();

      std::cout << "Server work was terminated. Waiting until all of the work threads will stop...\n";
      work_threads.join_all();
      std::cout << "Work threads have stopped. Process will close.\n";    
    }
  }
  catch (const boost::program_options::error&)
  {
    exit_code = EXIT_FAILURE;
    std::cout << "Invalid options.\n" << options_description;      
  }  
  return exit_code;
}

void start_session_manager(const session_manager_data_ptr& sm_data)
{
  sm_data->session_manager_->async_start
  (
    ma::make_custom_alloc_handler
    (
      sm_data->start_wait_allocator_,
      boost::bind(handle_session_manager_start, sm_data, _1)
    )
  );
  sm_data->state_ = session_manager_data::start_in_progress;
}

void wait_session_manager(const session_manager_data_ptr& sm_data)
{
  sm_data->session_manager_->async_wait
  (
    ma::make_custom_alloc_handler
    (
      sm_data->start_wait_allocator_,
      boost::bind(handle_session_manager_wait, sm_data, _1)
    )
  );
}

void stop_session_manager(const session_manager_data_ptr& sm_data)
{
  sm_data->session_manager_->async_stop
  (
    ma::make_custom_alloc_handler
    (
      sm_data->stop_allocator_,
      boost::bind(handle_session_manager_stop, sm_data, _1)
    )
  );
  sm_data->state_ = session_manager_data::stop_in_progress;
}

void run_io_service(boost::asio::io_service& io_service, 
  exception_handler io_service_exception_handler)
{
  try 
  {
    io_service.run();
  }
  catch (...)
  {
    io_service_exception_handler();
  }
}

void handle_work_exception(const session_manager_data_ptr& sm_data)
{
  boost::unique_lock<boost::mutex> server_data_lock(sm_data->mutex_);  
  sm_data->state_ = session_manager_data::stopped;  
  std::cout << "Terminating server work due to unexpected exception...\n";
  server_data_lock.unlock();
  sm_data->state_changed_.notify_one();      
}

void handle_program_exit(const session_manager_data_ptr& sm_data)
{
  boost::unique_lock<boost::mutex> server_data_lock(sm_data->mutex_);
  std::cout << "Program exit request detected.\n";
  if (session_manager_data::stopped == sm_data->state_)
  {    
    std::cout << "Server has already stopped.\n";
  }
  else if (session_manager_data::stop_in_progress == sm_data->state_)
  {    
    sm_data->state_ = session_manager_data::stopped;
    std::cout << "Server is already stopping. Terminating server work...\n";
    server_data_lock.unlock();
    sm_data->state_changed_.notify_one();       
  }
  else
  {    
    // Start server stop
    stop_session_manager(sm_data);
    sm_data->stopped_by_program_exit_ = true;
    std::cout << "Server is stopping. Press Ctrl+C (Ctrl+Break) to terminate server work...\n";
    server_data_lock.unlock();    
    sm_data->state_changed_.notify_one();
  }  
}

void handle_session_manager_start(const session_manager_data_ptr& sm_data,
  const boost::system::error_code& error)
{   
  boost::unique_lock<boost::mutex> server_data_lock(sm_data->mutex_);
  if (session_manager_data::start_in_progress == sm_data->state_)
  {   
    if (error)
    {       
      sm_data->state_ = session_manager_data::stopped;
      std::cout << "Server can't start due to error.\n";
      server_data_lock.unlock();
      sm_data->state_changed_.notify_one();
    }
    else
    {
      sm_data->state_ = session_manager_data::started;
      wait_session_manager(sm_data);
      std::cout << "Server has started.\n";
    }
  }
}

void handle_session_manager_wait(const session_manager_data_ptr& sm_data,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> server_data_lock(sm_data->mutex_);
  if (session_manager_data::started == sm_data->state_)
  {
    stop_session_manager(sm_data);
    std::cout << "Server can't continue work due to error. Server is stopping...\n";
    server_data_lock.unlock();
    sm_data->state_changed_.notify_one();    
  }
}

void handle_session_manager_stop(const session_manager_data_ptr& sm_data,
  const boost::system::error_code&)
{
  boost::unique_lock<boost::mutex> server_data_lock(sm_data->mutex_);
  if (session_manager_data::stop_in_progress == sm_data->state_)
  {      
    sm_data->state_ = session_manager_data::stopped;
    std::cout << "Server has stopped.\n";    
    server_data_lock.unlock();
    sm_data->state_changed_.notify_one();
  }
}