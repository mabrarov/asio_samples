//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <string>
#include <cstdlib>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/assert.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/console_controller.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/session_manager.hpp>

namespace
{
  class config_read_error : public std::exception
  {
  public:
    explicit config_read_error()
      : std::exception()
    {
    }

#if defined(__GNUC__)
    ~config_read_error() throw ()
    {
    }
#endif
  }; // class config_read_error

  class invalid_config_param : public config_read_error
  {
  public:
    typedef boost::shared_ptr<std::string> string_ptr;

    explicit invalid_config_param(const string_ptr& param_name, const string_ptr& message)
      : config_read_error()
      , param_name_(param_name)
      , message_(message)
    {
    }

    string_ptr param_name() const
    {
      return param_name_;
    }

    string_ptr message() const
    {
      return message_;
    }

  private:
    string_ptr param_name_;
    string_ptr message_;
  }; // class invalid_config_param
}

template <typename Value>
void print_param(std::ostream& stream, 
  const boost::optional<Value>& value, 
  const std::string& default_text)
{
  if (value) 
  {
    stream << *value;
  }
  else 
  {
    stream << default_text;
  }
}

template <>
void print_param<bool>(std::ostream& stream, 
  const boost::optional<bool>& value, 
  const std::string& default_text)
{
  if (value) 
  {
    if (*value)
    {
      stream << "on";
    }
    else 
    {
      stream << "off";
    }    
  }
  else 
  {
    stream << default_text;
  }
}

template <typename IntegralType>
void verify_param(const std::string& name, IntegralType value,
  IntegralType min, IntegralType max = (std::numeric_limits<IntegralType>::max)())
{
  if (value < min || value > max)
  {
    //todo: format message to incude max and min values
    std::string message = "out of range";
    boost::throw_exception(invalid_config_param(
      boost::make_shared<std::string>(name), 
      boost::make_shared<std::string>(message)));
  }  
}

namespace server = ma::echo::server;

namespace
{
  const char* help_param = "help"; 
  const char* port_param = "port";
  const char* session_manager_threads_param = "session_manager_threads";
  const char* session_threads_param   = "session_threads";
  const char* stop_timeout_param      = "stop_timeout";
  const char* max_sessions_param      = "max_sessions";
  const char* recycled_sessions_param = "recycled_sessions";
  const char* listen_backlog_param    = "listen_backlog";
  const char* buffer_size_param       = "buffer";
  const char* socket_recv_buffer_size_param = "socket_recv_buffer";
  const char* socket_send_buffer_size_param = "socket_send_buffer";
  const char* socket_no_delay_param   = "socket_no_delay";
  const char* os_default_text = "OS default";

  struct  session_manager_data;
  typedef boost::shared_ptr<session_manager_data> session_manager_data_ptr;
  typedef boost::function<void (void)> exception_handler;  

  class execution_config
  {
  public:    
    typedef boost::posix_time::time_duration time_duration_type;
    
    execution_config(
      std::size_t session_manager_thread_count,
      std::size_t session_thread_count, 
      const time_duration_type& stop_timeout)
      : session_manager_thread_count_(session_manager_thread_count)
      , session_thread_count_(session_thread_count)
      , stop_timeout_(stop_timeout)
    {
      BOOST_ASSERT_MSG(session_manager_thread_count > 0, "session_manager_thread_count must be > 0");
      BOOST_ASSERT_MSG(session_thread_count > 1, "session_thread_count must be > 0");
    }
    
    std::size_t session_manager_thread_count() const
    {
      return session_manager_thread_count_;
    }

    std::size_t session_thread_count() const
    {
      return session_thread_count_;
    }

    time_duration_type stop_timeout() const
    {
      return stop_timeout_;
    }

  private:
    std::size_t session_manager_thread_count_;
    std::size_t session_thread_count_;
    time_duration_type stop_timeout_;
  }; // class execution_config

  struct session_manager_data : private boost::noncopyable
  {
    typedef boost::mutex mutex_type;

    enum state_type
    {
      ready_to_start,
      start_in_progress,
      started,
      stop_in_progress,
      stopped
    }; // enum state_type

    volatile bool stopped_by_user;
    volatile state_type state;
    mutex_type access_mutex;  
    server::session_manager_ptr the_session_manager;    
    boost::condition_variable state_changed;  
    ma::in_place_handler_allocator<128> start_wait_allocator;
    ma::in_place_handler_allocator<128> stop_allocator;

    session_manager_data(boost::asio::io_service& io_service,
      boost::asio::io_service& session_io_service,
      const server::session_manager_config& config)
      : stopped_by_user(false)
      , state(ready_to_start)
      , the_session_manager(boost::make_shared<server::session_manager>(
          boost::ref(io_service), boost::ref(session_io_service), config))        
    {
    }

    ~session_manager_data()
    {
    }
  }; // session_manager_data

  void fill_options_description(boost::program_options::options_description&, std::size_t cpu_count);

  void start_session_manager(const session_manager_data_ptr&);

  void wait_session_manager(const session_manager_data_ptr&);

  void stop_session_manager(const session_manager_data_ptr&);

  void run_io_service(boost::asio::io_service&, exception_handler);

  void handle_work_exception(const session_manager_data_ptr&);

  void handle_program_exit(const session_manager_data_ptr&);

  void handle_session_manager_start(const session_manager_data_ptr&, const boost::system::error_code&);

  void handle_session_manager_wait(const session_manager_data_ptr&, const boost::system::error_code&);

  void handle_session_manager_stop(const session_manager_data_ptr&, const boost::system::error_code&);

  execution_config read_execution_config(const boost::program_options::variables_map& options_values);

  server::session_config read_session_config(const boost::program_options::variables_map& options_values);

  server::session_manager_config read_session_manager_config(
    const boost::program_options::variables_map& options_values,
    const server::session_config& session_config);

  void print_config(std::ostream& stream, std::size_t cpu_count,
    const execution_config& the_execution_config,
    const server::session_manager_config& session_manager_config);

  void handle_session_manager_start(const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code& error)
  {   
    boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(the_session_manager_data->access_mutex);
    if (session_manager_data::start_in_progress == the_session_manager_data->state)
    {     
      if (error)
      {       
        the_session_manager_data->state = session_manager_data::stopped;
        std::cout << "Server can't start due to error: " << error.message() << '\n';
        wrapper_lock.unlock();
        the_session_manager_data->state_changed.notify_one();
        return;
      }    
      the_session_manager_data->state = session_manager_data::started;
      wait_session_manager(the_session_manager_data);
      std::cout << "Server has started.\n";
    }
  }

  void handle_session_manager_wait(const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code& error)
  {
    boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(the_session_manager_data->access_mutex);
    if (session_manager_data::started == the_session_manager_data->state)
    {
      stop_session_manager(the_session_manager_data);
      std::cout << "Server can't continue work due to error: " << error.message() << '\n'
                << "Server is stopping...\n";
      wrapper_lock.unlock();
      the_session_manager_data->state_changed.notify_one();    
    }
  }

  void handle_session_manager_stop(const session_manager_data_ptr& the_session_manager_data, 
    const boost::system::error_code&)
  {
    boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(the_session_manager_data->access_mutex);
    if (session_manager_data::stop_in_progress == the_session_manager_data->state)
    {      
      the_session_manager_data->state = session_manager_data::stopped;
      std::cout << "Server has stopped.\n";    
      wrapper_lock.unlock();
      the_session_manager_data->state_changed.notify_one();
    }
  }  
  
  execution_config read_execution_config(const boost::program_options::variables_map& options_values)
  {
    std::size_t session_manager_thread_count = options_values[session_manager_threads_param].as<std::size_t>();
    verify_param<std::size_t>(session_manager_threads_param, session_manager_thread_count, 1);

    std::size_t session_thread_count = options_values[session_threads_param].as<std::size_t>();
    verify_param<std::size_t>(session_threads_param, session_thread_count, 1);

    long stop_timeout_sec = options_values[stop_timeout_param].as<long>();
    verify_param<long>(stop_timeout_param, stop_timeout_sec, 0);

    return execution_config(session_manager_thread_count, session_thread_count, 
      boost::posix_time::seconds(stop_timeout_sec));
  }  

  server::session_config read_session_config(const boost::program_options::variables_map& options_values)
  {  
    boost::optional<bool> no_delay;
    if (options_values.count(socket_no_delay_param))
    {
      no_delay = !options_values[socket_no_delay_param].as<bool>();
    }

    std::size_t buffer_size = options_values[buffer_size_param].as<std::size_t>();
    verify_param<std::size_t>(buffer_size_param, buffer_size, 1);

    boost::optional<int> socket_recv_buffer_size;
    if (options_values.count(socket_recv_buffer_size_param))
    {
      int value = options_values[socket_recv_buffer_size_param].as<int>();
      verify_param<int>(socket_recv_buffer_size_param, value, 0);
      socket_recv_buffer_size = value;
    }

    boost::optional<int> socket_send_buffer_size;
    if (options_values.count(socket_send_buffer_size_param))
    {
      int value = options_values[socket_send_buffer_size_param].as<int>();
      verify_param<int>(socket_send_buffer_size_param, value, 0);
      socket_send_buffer_size = value;
    }
    
    return server::session_config(buffer_size, socket_recv_buffer_size, socket_send_buffer_size, no_delay);
  }

  server::session_manager_config read_session_manager_config(
    const boost::program_options::variables_map& options_values,
    const server::session_config& session_config)
  {    
    unsigned short port = options_values[port_param].as<unsigned short>();

    std::size_t max_sessions = options_values[max_sessions_param].as<std::size_t>();
    verify_param<std::size_t>(max_sessions_param, max_sessions, 1);

    std::size_t recycled_sessions = options_values[recycled_sessions_param].as<std::size_t>();    

    int listen_backlog = options_values[listen_backlog_param].as<int>();
    verify_param<int>(listen_backlog_param, listen_backlog, 0);

    using boost::asio::ip::tcp;
    return server::session_manager_config(tcp::endpoint(tcp::v4(), port),
      max_sessions, recycled_sessions, listen_backlog, session_config);
  }

  void print_config(std::ostream& stream, std::size_t cpu_count,
    const execution_config& the_execution_config,
    const server::session_manager_config& session_manager_config)
  {
    const server::session_config session_config = session_manager_config.managed_session_config();

    stream << "Number of found CPU(s)             : " << cpu_count                                           << std::endl
           << "Number of session manager's threads: " << the_execution_config.session_manager_thread_count() << std::endl
           << "Number of sessions' threads        : " << the_execution_config.session_thread_count()         << std::endl
           << "Total number of work threads       : " 
           << the_execution_config.session_thread_count() + the_execution_config.session_manager_thread_count() << std::endl
           << "Server listen port                 : " << session_manager_config.accepting_endpoint().port()  << std::endl
           << "Server stop timeout (seconds)      : " << the_execution_config.stop_timeout().total_seconds() << std::endl
           << "Maximum number of active sessions  : " << session_manager_config.max_session_count()          << std::endl
           << "Maximum number of recycled sessions: " << session_manager_config.recycled_session_count()     << std::endl
           << "TCP listen backlog size            : " << session_manager_config.listen_backlog()             << std::endl
           << "Size of session's buffer (bytes)   : " << session_config.buffer_size()                        << std::endl;

    stream << "Size of session's socket receive buffer (bytes): ";
    print_param(stream, session_config.socket_recv_buffer_size(), os_default_text);
    stream << std::endl;

    stream << "Size of session's socket send buffer (bytes)   : ";
    print_param(stream, session_config.socket_send_buffer_size(), os_default_text);
    stream << std::endl;

    stream << "Session's socket Nagle algorithm is: ";
    print_param(stream, session_config.no_delay(), os_default_text);
    stream << std::endl;      
  }

  void fill_options_description(
    boost::program_options::options_description& options_description, 
    std::size_t cpu_count)
  {
    std::size_t session_manager_thread_count = cpu_count > 1 ? 2 : 1;
    std::size_t session_thread_count = cpu_count ? cpu_count + 1 : 2;

    options_description.add_options()
      (
        help_param, 
        "produce help message"
      )
      (
        port_param, 
        boost::program_options::value<unsigned short>(), 
        "set the TCP port number for incoming connections' listening"
      )
      (
        session_manager_threads_param, 
        boost::program_options::value<std::size_t>()->default_value(session_manager_thread_count), 
        "set the number of session manager's threads"
      )    
      (
        session_threads_param, 
        boost::program_options::value<std::size_t>()->default_value(session_thread_count), 
        "set the number of sessions' threads"
      )    
      (
        stop_timeout_param, 
        boost::program_options::value<long>()->default_value(60), 
        "set the server stop timeout, at one's expiration server work will be terminated (seconds)"
      )
      (
        max_sessions_param, 
        boost::program_options::value<std::size_t>()->default_value(10000), 
        "set the maximum number of simultaneously active sessions"
      )
      (
        recycled_sessions_param, 
        boost::program_options::value<std::size_t>()->default_value(100), 
        "set the maximum number of pooled inactive sessions"
      )
      (
        listen_backlog_param, 
        boost::program_options::value<int>()->default_value(6), 
        "set the size of TCP listen backlog"
      )
      (
        buffer_size_param, 
        boost::program_options::value<std::size_t>()->default_value(1024),
        "set the session's buffer size (bytes)"
      )
      (
        socket_recv_buffer_size_param, 
        boost::program_options::value<int>(),
        "set the size of session's socket receive buffer (bytes)"
      )  
      (
        socket_send_buffer_size_param, 
        boost::program_options::value<int>(),
        "set the size of session's socket send buffer (bytes)"
      )
      (
        socket_no_delay_param, 
        boost::program_options::value<bool>(),
        "set TCP_NODELAY option of session's socket"
      );  
  }

  void start_session_manager(const session_manager_data_ptr& the_session_manager_data)
  {
    the_session_manager_data->the_session_manager->async_start(
      ma::make_custom_alloc_handler(the_session_manager_data->start_wait_allocator,
        boost::bind(handle_session_manager_start, the_session_manager_data, _1)));
    the_session_manager_data->state = session_manager_data::start_in_progress;
  }

  void wait_session_manager(const session_manager_data_ptr& the_session_manager_data)
  {
    the_session_manager_data->the_session_manager->async_wait(
      ma::make_custom_alloc_handler(the_session_manager_data->start_wait_allocator,
        boost::bind(handle_session_manager_wait, the_session_manager_data, _1)));
  }

  void stop_session_manager(const session_manager_data_ptr& the_session_manager_data)
  {
    the_session_manager_data->the_session_manager->async_stop(
      ma::make_custom_alloc_handler(the_session_manager_data->stop_allocator,
        boost::bind(handle_session_manager_stop, the_session_manager_data, _1)));
    the_session_manager_data->state = session_manager_data::stop_in_progress;
  }

  void run_io_service(boost::asio::io_service& io_service, 
    exception_handler the_exception_handler)
  {
    try 
    {
      io_service.run();
    }
    catch (...)
    {
      the_exception_handler();
    }
  }

  void handle_work_exception(const session_manager_data_ptr& the_session_manager_data)
  {
    boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(the_session_manager_data->access_mutex);
    if (session_manager_data::stopped != the_session_manager_data->state)
    {
      the_session_manager_data->state = session_manager_data::stopped;  
      std::cout << "Terminating server work due to unexpected exception...\n";
      wrapper_lock.unlock();
      the_session_manager_data->state_changed.notify_one();      
    }  
  }

  void handle_program_exit(const session_manager_data_ptr& the_session_manager_data)
  {
    boost::unique_lock<session_manager_data::mutex_type> wrapper_lock(the_session_manager_data->access_mutex);
    std::cout << "Program exit request detected.\n";
    if (session_manager_data::stopped == the_session_manager_data->state)
    {    
      std::cout << "Server has already stopped.\n";
      return;
    }
    if (session_manager_data::stop_in_progress == the_session_manager_data->state)
    {    
      the_session_manager_data->state = session_manager_data::stopped;
      std::cout << "Server is already stopping. Terminating server work...\n";
      wrapper_lock.unlock();
      the_session_manager_data->state_changed.notify_one();       
      return;
    }  
    // Begin server stop
    stop_session_manager(the_session_manager_data);
    the_session_manager_data->stopped_by_user = true;
    std::cout << "Server is stopping. Press Ctrl+C (Ctrl+Break) to terminate server work...\n";
    wrapper_lock.unlock();    
    the_session_manager_data->state_changed.notify_one();  
  }

} // anonymous namespace

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  try 
  {    
    std::size_t cpu_count = boost::thread::hardware_concurrency();

    // Initialize options description... 
    boost::program_options::options_description options_description("Allowed options");
    fill_options_description(options_description, cpu_count);
    // ... and read options values
    boost::program_options::variables_map options_values;  
    boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, options_description), options_values);
    boost::program_options::notify(options_values);
    
    // Check start mode
    if (options_values.count(help_param))
    {
      std::cout << options_description;
      return EXIT_SUCCESS;
    }
    if (!options_values.count(port_param))
    {          
      std::cout << "Port must be specified" << std::endl << options_description;
      return EXIT_FAILURE;
    }        

    // Read configuration
    execution_config the_execution_config = read_execution_config(options_values);    
    server::session_config session_config = read_session_config(options_values);
    server::session_manager_config session_manager_config = 
      read_session_manager_config(options_values, session_config);

    print_config(std::cout, cpu_count, the_execution_config, session_manager_config);
                  
    // Before session_manager_io_service
    boost::asio::io_service session_io_service(the_execution_config.session_thread_count());      
    // ... for the right destruction order
    boost::asio::io_service session_manager_io_service(the_execution_config.session_manager_thread_count());

    session_manager_data_ptr the_session_manager_data(boost::make_shared<session_manager_data>(
      boost::ref(session_manager_io_service), boost::ref(session_io_service), session_manager_config));
      
    std::cout << "Server is starting...\n";
      
    // Start the server      
    boost::unique_lock<session_manager_data::mutex_type> session_manager_lock(the_session_manager_data->access_mutex);    
    start_session_manager(the_session_manager_data);
    session_manager_lock.unlock();
      
    // Setup console controller
    ma::console_controller console_controller(boost::bind(handle_program_exit, the_session_manager_data));
    std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

    // Exception handler for automatic server aborting
    exception_handler work_exception_handler(boost::bind(handle_work_exception, the_session_manager_data));

    // Create work for sessions' io_service to prevent threads' stop
    boost::asio::io_service::work session_work(session_io_service);
    // Create work for session_manager's io_service to prevent threads' stop
    boost::asio::io_service::work session_manager_work(session_manager_io_service);    

    // Create work threads for session operations
    boost::thread_group work_threads;    
    for (std::size_t i = 0; i != the_execution_config.session_thread_count(); ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_io_service), work_exception_handler));
    }
    // Create work threads for session manager operations
    for (std::size_t i = 0; i != the_execution_config.session_manager_thread_count(); ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_manager_io_service), work_exception_handler));      
    }

    int exit_code = EXIT_SUCCESS;

    // Wait until server begins stopping or stops
    session_manager_lock.lock();
    while (session_manager_data::stop_in_progress != the_session_manager_data->state 
      && session_manager_data::stopped != the_session_manager_data->state)
    {
      the_session_manager_data->state_changed.wait(session_manager_lock);
    }        
    if (session_manager_data::stopped != the_session_manager_data->state)
    {
      // Wait until server stops with timeout
      if (!the_session_manager_data->state_changed.timed_wait(
        session_manager_lock, the_execution_config.stop_timeout()))
      {
        std::cout << "Server stop timeout expiration. Terminating server work...\n";
        exit_code = EXIT_FAILURE;
      }
      // Mark server as stopped
      the_session_manager_data->state = session_manager_data::stopped;
    }
    // Check the reason of server stop and signal it by exit code
    if (EXIT_SUCCESS == exit_code && !the_session_manager_data->stopped_by_user)
    {      
      exit_code = EXIT_FAILURE;
    }
    session_manager_lock.unlock();
     
    // Shutdown execution queues...
    session_manager_io_service.stop();
    session_io_service.stop();
    std::cout << "Server work was terminated. Waiting until all of the work threads will stop...\n";
    // ...and wait until all work done.
    work_threads.join_all();
    std::cout << "Work threads have stopped. Process will close.\n";        

    return exit_code;
  }
  catch (const invalid_config_param& e)
  {    
    std::cerr << "Error reading options. Invalid argument value. " 
      << *e.param_name() << ": " << *e.message() << std::endl;
  }
  catch (const boost::program_options::error& e)
  {    
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {    
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  return EXIT_FAILURE;
}
