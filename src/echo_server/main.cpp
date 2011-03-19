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
#include <boost/cstdint.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/throw_exception.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/console_controller.hpp>
#include <ma/echo/server/session_options.hpp>
#include <ma/echo/server/session_manager_options.hpp>
#include <ma/echo/server/session_manager.hpp>

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
  const char* socket_recv_buffer_size_param = "sock_recv_buffer";
  const char* socket_send_buffer_size_param = "sock_send_buffer";
  const char* socket_no_delay_param         = "sock_no_delay";
  const char* os_default_text = "OS default";

  template <typename IntegerType, IntegerType min, IntegerType max>
  class constrained_integer
  {
  private:
    typedef constrained_integer<IntegerType, min, max> this_type;

  public:
    typedef IntegerType value_type;

    explicit constrained_integer(value_type value)
      : value_(value)
    {
    }

    value_type value() const
    {
      return value_;
    }

    friend std::ostream& operator<<(std::ostream& stream, const this_type& obj)
    {
      stream << obj.value();
      return stream;
    }

    friend void validate(boost::any& option_value, 
      const std::vector<std::string>& text_values,
      this_type*, int)
    {              
      // Make sure no previous assignment to 'a' was made.
      boost::program_options::validators::check_first_occurrence(option_value);
      // Extract the first string from 'values'. If there is more than
      // one string, it's an error, and exception will be thrown.
      const std::string& s = boost::program_options::validators::get_single_string(text_values);
      // Read raw (unchecked, maximal width integer) value
      typedef boost::intmax_t raw_value_type;
      raw_value_type raw_value;
      try 
      {
        raw_value = boost::lexical_cast<raw_value_type>(s);
      }
      catch (const boost::bad_lexical_cast&)
      {
        boost::throw_exception(boost::program_options::validation_error(
          boost::program_options::validation_error::invalid_option_value));
      }
      // Check raw value
      value_type checked_value;
      try 
      {     
        checked_value = boost::numeric_cast<value_type>(raw_value);        
      }
      catch (const boost::bad_numeric_cast&)
      {
        boost::throw_exception(boost::program_options::validation_error(
            boost::program_options::validation_error::invalid_option_value));
      }
      if (checked_value < min || checked_value > max)
      {
        boost::throw_exception(boost::program_options::validation_error(
          boost::program_options::validation_error::invalid_option_value));
      }
      option_value = boost::any(this_type(checked_value));
    }

  private:
    value_type value_;
  }; //class constrained_integer

  typedef constrained_integer<unsigned short, 0, 65535> port_num_type;
  typedef constrained_integer<int, 0, 1024>             listen_backlog_size_type;
  typedef constrained_integer<std::size_t, 1, 1024>     thread_count_type;
  typedef constrained_integer<int, 0, 3600>             timeout_sec_type;
  typedef constrained_integer<std::size_t, 1, 100000>   max_session_count_type;
  typedef constrained_integer<std::size_t, 0, 10000>    recycled_session_count_type;
  typedef constrained_integer<std::size_t, 1, 1048576>  buffer_size_type;
  typedef constrained_integer<int, 0, 1048576>          socket_buffer_size_type;

  template <typename Value>
  void print(std::ostream& stream, const boost::optional<Value>& value, const std::string& default_text)
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
  void print<bool>(std::ostream& stream, const boost::optional<bool>& value, const std::string& default_text)
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

  struct  session_manager_data;
  typedef boost::shared_ptr<session_manager_data> session_manager_data_ptr;
  typedef boost::function<void (void)> exception_handler;  

  class execution_options
  {
  public:    
    typedef boost::posix_time::time_duration time_duration_type;
    
    execution_options(std::size_t session_manager_thread_count,
      std::size_t session_thread_count, 
      const time_duration_type& stop_timeout)
      : session_manager_thread_count_(session_manager_thread_count)
      , session_thread_count_(session_thread_count)
      , stop_timeout_(stop_timeout)
    {
      BOOST_ASSERT_MSG(session_manager_thread_count > 0, "session_manager_thread_count must be > 0");
      BOOST_ASSERT_MSG(session_thread_count > 0, "session_thread_count must be > 0");
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
  }; // class execution_options

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
    ma::echo::server::session_manager_ptr session_manager;    
    boost::condition_variable state_changed;  
    ma::in_place_handler_allocator<128> start_wait_allocator;
    ma::in_place_handler_allocator<128> stop_allocator;

    session_manager_data(boost::asio::io_service& io_service,
      boost::asio::io_service& session_io_service,
      const ma::echo::server::session_manager_options& options)
      : stopped_by_user(false)
      , state(ready_to_start)
      , session_manager(boost::make_shared<ma::echo::server::session_manager>(
          boost::ref(io_service), boost::ref(session_io_service), options))        
    {
    }

    ~session_manager_data()
    {
    }
  }; // session_manager_data

  std::size_t calc_session_manager_thread_count(std::size_t hardware_concurrency);
  std::size_t calc_session_thread_count(std::size_t hardware_concurrency);

  void fill_options_description(boost::program_options::options_description&, std::size_t hardware_concurrency);

  void start_session_manager(const session_manager_data_ptr&);
  void wait_session_manager(const session_manager_data_ptr&);
  void stop_session_manager(const session_manager_data_ptr&);

  void run_io_service(boost::asio::io_service&, exception_handler);

  void handle_work_exception(const session_manager_data_ptr&);
  void handle_program_exit(const session_manager_data_ptr&);
  void handle_session_manager_start(const session_manager_data_ptr&, const boost::system::error_code&);
  void handle_session_manager_wait(const session_manager_data_ptr&, const boost::system::error_code&);
  void handle_session_manager_stop(const session_manager_data_ptr&, const boost::system::error_code&);

  execution_options read_execution_options(const boost::program_options::variables_map& options_values);
  ma::echo::server::session_options read_session_options(const boost::program_options::variables_map& options_values);
  ma::echo::server::session_manager_options read_session_manager_options(
    const boost::program_options::variables_map& options_values,
    const ma::echo::server::session_options& session_options);

  void print_options(std::ostream& stream, std::size_t cpu_count,
    const execution_options& the_execution_options,
    const ma::echo::server::session_manager_options& session_manager_options);

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
  
  execution_options read_execution_options(const boost::program_options::variables_map& options_values)
  {
    std::size_t session_manager_thread_count = 
      options_values[session_manager_threads_param].as<thread_count_type>().value();

    std::size_t session_thread_count = 
      options_values[session_threads_param].as<thread_count_type>().value();    

    int stop_timeout_sec = 
      options_values[stop_timeout_param].as<timeout_sec_type>().value();
    
    return execution_options(session_manager_thread_count, session_thread_count, 
      boost::posix_time::seconds(stop_timeout_sec));
  } 

  boost::optional<int> read_socket_buffer_size(const boost::program_options::variables_map& options_values,
    const char* option_name)
  {     
    if (!options_values.count(option_name))
    {
      return boost::optional<int>();
    }
    return options_values[option_name].as<socket_buffer_size_type>().value();
  }

  ma::echo::server::session_options read_session_options(const boost::program_options::variables_map& options_values)
  {  
    boost::optional<bool> no_delay;
    if (options_values.count(socket_no_delay_param))
    {
      no_delay = !options_values[socket_no_delay_param].as<bool>();
    }

    std::size_t buffer_size = 
      options_values[buffer_size_param].as<buffer_size_type>().value();

    boost::optional<int> socket_recv_buffer_size = 
      read_socket_buffer_size(options_values, socket_recv_buffer_size_param);

    boost::optional<int> socket_send_buffer_size =
      read_socket_buffer_size(options_values, socket_send_buffer_size_param);
    
    return ma::echo::server::session_options(buffer_size, 
      socket_recv_buffer_size, socket_send_buffer_size, no_delay);
  }

  ma::echo::server::session_manager_options read_session_manager_options(
    const boost::program_options::variables_map& options_values,
    const ma::echo::server::session_options& session_options)
  {    
    unsigned short port = options_values[port_param].as<port_num_type>().value();

    std::size_t max_sessions = 
      options_values[max_sessions_param].as<max_session_count_type>().value();    

    std::size_t recycled_sessions = 
      options_values[recycled_sessions_param].as<recycled_session_count_type>().value();    

    int listen_backlog = 
      options_values[listen_backlog_param].as<listen_backlog_size_type>().value();
    
    using boost::asio::ip::tcp;
    return ma::echo::server::session_manager_options(tcp::endpoint(tcp::v4(), port),
      max_sessions, recycled_sessions, listen_backlog, session_options);
  }

  void print_options(std::ostream& stream, std::size_t cpu_count,
    const execution_options& the_execution_options,
    const ma::echo::server::session_manager_options& session_manager_options)
  {
    const ma::echo::server::session_options session_options = session_manager_options.managed_session_options();

    stream << "Number of found CPU(s)             : " << cpu_count                                            << std::endl
           << "Number of session manager's threads: " << the_execution_options.session_manager_thread_count() << std::endl
           << "Number of sessions' threads        : " << the_execution_options.session_thread_count()         << std::endl
           << "Total number of work threads       : " 
           << the_execution_options.session_thread_count() + the_execution_options.session_manager_thread_count() << std::endl
           << "Server listen port                 : " << session_manager_options.accepting_endpoint().port()  << std::endl
           << "Server stop timeout (seconds)      : " << the_execution_options.stop_timeout().total_seconds() << std::endl
           << "Maximum number of active sessions  : " << session_manager_options.max_session_count()          << std::endl
           << "Maximum number of recycled sessions: " << session_manager_options.recycled_session_count()     << std::endl
           << "TCP listen backlog size            : " << session_manager_options.listen_backlog()             << std::endl
           << "Size of session's buffer (bytes)   : " << session_options.buffer_size()                        << std::endl;

    stream << "Size of session's socket receive buffer (bytes): ";
    print(stream, session_options.socket_recv_buffer_size(), os_default_text);
    stream << std::endl;

    stream << "Size of session's socket send buffer (bytes)   : ";
    print(stream, session_options.socket_send_buffer_size(), os_default_text);
    stream << std::endl;

    stream << "Session's socket Nagle algorithm is: ";
    print(stream, session_options.no_delay(), os_default_text);
    stream << std::endl;      
  }

  std::size_t calc_session_manager_thread_count(std::size_t hardware_concurrency)
  {
    if (hardware_concurrency < 2)
    {
      return 1;
    }
    return 2;
  }

  std::size_t calc_session_thread_count(std::size_t hardware_concurrency)
  {
    if (hardware_concurrency)
    {
      if ((std::numeric_limits<std::size_t>::max)() == hardware_concurrency)
      {
        return hardware_concurrency;
      }
      return hardware_concurrency + 1;
    }
    return 2;
  }

  void fill_options_description(boost::program_options::options_description& options_description, 
    std::size_t hardware_concurrency)
  {
    std::size_t session_manager_thread_count = calc_session_manager_thread_count(hardware_concurrency);
    std::size_t session_thread_count = calc_session_thread_count(hardware_concurrency);    
    options_description.add_options()
      (
        help_param, 
        "produce help message"
      )
      (
        port_param, 
        boost::program_options::value<port_num_type>(), 
        "set the TCP port number for incoming connections' listening"
      )
      (
        session_manager_threads_param, 
        boost::program_options::value<thread_count_type>()->default_value(
          thread_count_type(session_manager_thread_count)), 
        "set the number of session manager's threads"
      )    
      (
        session_threads_param, 
        boost::program_options::value<thread_count_type>()->default_value(
        thread_count_type(session_thread_count)), 
        "set the number of sessions' threads"
      )    
      (
        stop_timeout_param, 
        boost::program_options::value<timeout_sec_type>()->default_value(
          timeout_sec_type(60)), 
        "set the server stop timeout, at one's expiration server work will be terminated (seconds)"
      )
      (
        max_sessions_param, 
        boost::program_options::value<max_session_count_type>()->default_value(
          max_session_count_type(10000)), 
        "set the maximum number of simultaneously active sessions"
      )
      (
        recycled_sessions_param, 
        boost::program_options::value<recycled_session_count_type>()->default_value(
          recycled_session_count_type(100)), 
        "set the maximum number of pooled inactive sessions"
      )
      (
        listen_backlog_param, 
        boost::program_options::value<listen_backlog_size_type>()->default_value(
          listen_backlog_size_type(6)), 
        "set the size of TCP listen backlog"
      )
      (
        buffer_size_param, 
        boost::program_options::value<buffer_size_type>()->default_value(
          buffer_size_type(1024)),
        "set the session's buffer size (bytes)"
      )
      (
        socket_recv_buffer_size_param, 
        boost::program_options::value<socket_buffer_size_type>(),
        "set the size of session's socket receive buffer (bytes)"
      )  
      (
        socket_send_buffer_size_param, 
        boost::program_options::value<socket_buffer_size_type>(),
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
    the_session_manager_data->session_manager->async_start(
      ma::make_custom_alloc_handler(the_session_manager_data->start_wait_allocator,
        boost::bind(handle_session_manager_start, the_session_manager_data, _1)));
    the_session_manager_data->state = session_manager_data::start_in_progress;
  }

  void wait_session_manager(const session_manager_data_ptr& the_session_manager_data)
  {
    the_session_manager_data->session_manager->async_wait(
      ma::make_custom_alloc_handler(the_session_manager_data->start_wait_allocator,
        boost::bind(handle_session_manager_wait, the_session_manager_data, _1)));
  }
  
  void stop_session_manager(const session_manager_data_ptr& the_session_manager_data)
  {
    the_session_manager_data->session_manager->async_stop(
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
    execution_options the_execution_options = read_execution_options(options_values);    
    ma::echo::server::session_options session_options = read_session_options(options_values);
    ma::echo::server::session_manager_options session_manager_options = 
      read_session_manager_options(options_values, session_options);

    print_options(std::cout, cpu_count, the_execution_options, session_manager_options);
                  
    // Before session_manager_io_service
    boost::asio::io_service session_io_service(the_execution_options.session_thread_count());      
    // ... for the right destruction order
    boost::asio::io_service session_manager_io_service(the_execution_options.session_manager_thread_count());

    session_manager_data_ptr the_session_manager_data(boost::make_shared<session_manager_data>(
      boost::ref(session_manager_io_service), boost::ref(session_io_service), session_manager_options));
      
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
    for (std::size_t i = 0; i != the_execution_options.session_thread_count(); ++i)
    {
      work_threads.create_thread(boost::bind(run_io_service, 
        boost::ref(session_io_service), work_exception_handler));
    }
    // Create work threads for session manager operations
    for (std::size_t i = 0; i != the_execution_options.session_manager_thread_count(); ++i)
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
        session_manager_lock, the_execution_options.stop_timeout()))
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
