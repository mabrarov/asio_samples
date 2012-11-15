//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <cstdlib>
#include <vector>
#include <string>
#include <iostream>
#include <exception>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/program_options.hpp>
#include <boost/thread/condition_variable.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/console_controller.hpp>
#include <ma/echo/server/simple_session_factory.hpp>
#include <ma/echo/server/pooled_session_factory.hpp>
#include <ma/echo/server/session_manager.hpp>
#include "config.hpp"

namespace echo_server {

void print_stats(const ma::echo::server::session_manager_stats&);

int run_server(const execution_config&,
    const ma::echo::server::session_manager_config&);

} // namespace echo_server

#if defined(WIN32)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  try
  {
    using namespace echo_server;

    const std::size_t cpu_count = boost::thread::hardware_concurrency();

    const boost::program_options::options_description
        cmd_options_description = build_cmd_options_description(cpu_count);

    // Parse user defined command line options
    const boost::program_options::variables_map cmd_options =
        parse_cmd_line(cmd_options_description, argc, argv);

    // Check work mode
    if (is_help_mode(cmd_options))
    {
      std::cout << cmd_options_description;
      return EXIT_SUCCESS;
    }

    // Check required options
    if (!is_required_specified(cmd_options))
    {
      std::cout << "Required options not specified" << std::endl
                << cmd_options_description;
      return EXIT_FAILURE;
    }

    // Parse configuration
    const execution_config exec_config = build_execution_config(cmd_options);
    const ma::echo::server::session_config session_config =
        build_session_config(cmd_options);
    const ma::echo::server::session_manager_config session_manager_config =
        build_session_manager_config(cmd_options, session_config);

    // Show server configuration
    print_config(std::cout, cpu_count, exec_config, session_manager_config);

    // Do the work
    return run_server(exec_config, session_manager_config);
  }
  catch (const boost::program_options::error& e)
  {
    std::cerr << "Error reading options: " << e.what() << std::endl;
  }
  catch (const std::exception& e)
  {
    std::cerr << "Unexpected exception: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown exception" << std::endl;
  }
  return EXIT_FAILURE;
}

namespace {

struct  session_manager_wrapper;
typedef boost::shared_ptr<session_manager_wrapper> wrapped_session_manager_ptr;
typedef boost::function<void (void)> exception_handler;

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef std::vector<io_service_ptr> io_service_vector;
typedef boost::shared_ptr<ma::echo::server::session_factory>
    session_factory_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> io_service_work_ptr;
typedef std::vector<io_service_work_ptr> io_service_work_vector;

struct session_manager_wrapper : private boost::noncopyable
{
  typedef boost::mutex mutex_type;
  typedef boost::unique_lock<session_manager_wrapper::mutex_type> unique_lock;

  struct state_type
  {
    enum value_t {ready, start, work, stop, stopped};
  };

  mutex_type mutex;
  boost::condition_variable state_changed;
  ma::echo::server::session_manager_ptr session_manager;
  state_type::value_t state;
  bool stopped_by_user;

  ma::in_place_handler_allocator<128> start_wait_allocator;
  ma::in_place_handler_allocator<128> stop_allocator;

  session_manager_wrapper(boost::asio::io_service& io_service,
      ma::echo::server::session_factory& session_factory,
      const ma::echo::server::session_manager_config& config)
    : session_manager(ma::echo::server::session_manager::create(io_service,
          session_factory, config))
    , state(state_type::ready)
    , stopped_by_user(false)
  {
  }

#if !defined(NDEBUG)
  ~session_manager_wrapper()
  {
  }
#endif

  ma::echo::server::session_manager_stats stats() const
  {
    return session_manager->stats();
  }

  bool is_starting() const
  {
    return state_type::start == state;
  }

  bool is_working() const
  {
    return state_type::work == state;
  }

  bool is_stopping() const
  {
    return state_type::stop == state;
  }

  bool is_stopped() const
  {
    return state_type::stopped == state;
  }

  void wait_for_state_change(unique_lock& lock)
  {
    state_changed.wait(lock);
  }

  bool wait_for_state_change(unique_lock& lock,
      const echo_server::execution_config::time_duration_type& timeout)
  {
    return state_changed.timed_wait(lock, timeout);
  }

  void mark_as_stopped()
  {
    state = state_type::stopped;
    state_changed.notify_one();
  }

  void mark_as_starting()
  {
    state = state_type::start;
    state_changed.notify_one();
  }

  void mark_as_working()
  {
    state = state_type::work;
    state_changed.notify_one();
  }

  void mark_as_stopping()
  {
    state = state_type::stop;
    state_changed.notify_one();
  }

}; // struct session_manager_wrapper

template <typename Integer>
std::string to_string(const ma::limited_int<Integer>& limited_value)
{
  if (limited_value.overflowed())
  {
    return ">" + boost::lexical_cast<std::string>(limited_value.value());
  }
  else
  {
    return boost::lexical_cast<std::string>(limited_value.value());
  }
}

io_service_vector create_session_io_services(
    const echo_server::execution_config&);

session_factory_ptr create_session_factory(
    const echo_server::execution_config&,
    const ma::echo::server::session_manager_config&,
    const io_service_vector&);

void create_session_threads(boost::thread_group&,
    const echo_server::execution_config&, const io_service_vector&,
    const exception_handler&);

void create_session_manager_threads(boost::thread_group&,
    const echo_server::execution_config&, boost::asio::io_service&,
    const exception_handler&);

io_service_work_vector create_works(const io_service_vector&);

void stop(const io_service_vector&);

void run_io_service(boost::asio::io_service&, exception_handler);

void handle_work_exception(const wrapped_session_manager_ptr&);

void handle_program_exit(const wrapped_session_manager_ptr&);

void handle_session_manager_start(const wrapped_session_manager_ptr&,
    const boost::system::error_code&);

void handle_session_manager_wait(const wrapped_session_manager_ptr&,
    const boost::system::error_code&);

void handle_session_manager_stop(const wrapped_session_manager_ptr&,
    const boost::system::error_code&);

void start_session_manager_start(const wrapped_session_manager_ptr&);

void start_session_manager_wait(const wrapped_session_manager_ptr&);

void start_session_manager_stop(const wrapped_session_manager_ptr&);

io_service_vector create_session_io_services(
    const echo_server::execution_config& exec_config)
{
  io_service_vector io_services;
  if (exec_config.ios_per_work_thread)
  {
    for (std::size_t i = 0; i != exec_config.session_thread_count; ++i)
    {
      io_services.push_back(boost::make_shared<boost::asio::io_service>(1));
    }
  }
  else
  {
    io_services.push_back(boost::make_shared<boost::asio::io_service>(
        exec_config.session_thread_count));
  }
  return io_services;
}

session_factory_ptr create_session_factory(
    const echo_server::execution_config& exec_config,
    const ma::echo::server::session_manager_config& session_manager_config,
    const io_service_vector& io_services)
{
  if (exec_config.ios_per_work_thread)
  {
    return boost::make_shared<ma::echo::server::pooled_session_factory>(
        io_services, session_manager_config.recycled_session_count);
  }
  else
  {
    return boost::make_shared<ma::echo::server::simple_session_factory>(
        boost::ref(*io_services.front()),
        session_manager_config.recycled_session_count);
  }
}

void create_session_threads(boost::thread_group& threads,
    const echo_server::execution_config& exec_config,
    const io_service_vector& io_services,
    const exception_handler& work_exception_handler)
{
  if (exec_config.ios_per_work_thread)
  {
    for (io_service_vector::const_iterator i = io_services.begin(),
        end = io_services.end(); i != end; ++i)
    {
      threads.create_thread(boost::bind(run_io_service,
          boost::ref(**i), work_exception_handler));
    }
  }
  else
  {
    for (std::size_t i = 0; i != exec_config.session_thread_count; ++i)
    {
      threads.create_thread(boost::bind(run_io_service,
          boost::ref(*io_services.front()), work_exception_handler));
    }
  }
}

void create_session_manager_threads(boost::thread_group& threads,
    const echo_server::execution_config& exec_config,
    boost::asio::io_service& io_service,
    const exception_handler& work_exception_handler)
{
  for (std::size_t i = 0; i != exec_config.session_manager_thread_count; ++i)
  {
    threads.create_thread(boost::bind(run_io_service,
        boost::ref(io_service), work_exception_handler));
  }
}

void stop(const io_service_vector& io_services)
{
  for (io_service_vector::const_iterator i = io_services.begin(),
      end = io_services.end(); i != end; ++i)
  {
    (*i)->stop();
  }
}

io_service_work_vector create_works(const io_service_vector& io_services)
{
  io_service_work_vector works;
  for (io_service_vector::const_iterator i = io_services.begin(),
      end = io_services.end(); i != end; ++i)
  {
    works.push_back(
        boost::make_shared<boost::asio::io_service::work>(boost::ref(**i)));
  }
  return works;
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

void handle_work_exception(const wrapped_session_manager_ptr& session_manager)
{
  session_manager_wrapper::unique_lock lock(session_manager->mutex);

  if (session_manager->is_stopped())
  {
    return;
  }

  session_manager->mark_as_stopped();
  std::cout << "Terminating server work due to unexpected exception...\n";
}

void handle_program_exit(const wrapped_session_manager_ptr& session_manager)
{
  session_manager_wrapper::unique_lock lock(session_manager->mutex);

  std::cout << "Program exit request detected.\n";

  if (session_manager->is_stopped())
  {
    std::cout << "Server has already stopped.\n";
    return;
  }

  if (session_manager->is_stopping())
  {
    session_manager->mark_as_stopped();
    std::cout << "Server is already stopping. Terminating server work...\n";
    return;
  }

  start_session_manager_stop(session_manager);
  session_manager->mark_as_stopping();
  session_manager->stopped_by_user = true;

  std::cout << "Server is stopping." \
      " Press Ctrl+C (Ctrl+Break) to terminate server work...\n";
}

void handle_session_manager_start(
    const wrapped_session_manager_ptr& session_manager,
    const boost::system::error_code& error)
{
  session_manager_wrapper::unique_lock lock(session_manager->mutex);

  if (!session_manager->is_starting())
  {
    return;
  }

  if (error)
  {
    session_manager->mark_as_stopped();
    std::cout << "Server can't start due to error: "
        << error.message() << '\n';
    return;
  }

  session_manager->mark_as_working();
  start_session_manager_wait(session_manager);

  std::cout << "Server has started.\n";
}

void handle_session_manager_wait(
    const wrapped_session_manager_ptr& session_manager,
    const boost::system::error_code& error)
{
  session_manager_wrapper::unique_lock lock(session_manager->mutex);

  if (!session_manager->is_working())
  {
    return;
  }

  start_session_manager_stop(session_manager);
  session_manager->mark_as_stopping();

  std::cout << "Server can't continue work due to error: "
      << error.message() << '\n' << "Server is stopping...\n";
}

void handle_session_manager_stop(
    const wrapped_session_manager_ptr& session_manager,
    const boost::system::error_code&)
{
  session_manager_wrapper::unique_lock lock(session_manager->mutex);
  if (!session_manager->is_stopping())
  {
    return;
  }
  session_manager->mark_as_stopped();
  std::cout << "Server has stopped.\n";
}

void start_session_manager_start(
    const wrapped_session_manager_ptr& session_manager)
{
  session_manager->session_manager->async_start(ma::make_custom_alloc_handler(
      session_manager->start_wait_allocator,
      boost::bind(handle_session_manager_start, session_manager, _1)));
}

void start_session_manager_wait(
    const wrapped_session_manager_ptr& session_manager)
{
  session_manager->session_manager->async_wait(ma::make_custom_alloc_handler(
      session_manager->start_wait_allocator,
      boost::bind(handle_session_manager_wait, session_manager, _1)));
}

void start_session_manager_stop(
    const wrapped_session_manager_ptr& session_manager)
{
  session_manager->session_manager->async_stop(ma::make_custom_alloc_handler(
      session_manager->stop_allocator,
      boost::bind(handle_session_manager_stop, session_manager, _1)));
}

} // anonymous namespace

void echo_server::print_stats(
    const ma::echo::server::session_manager_stats& stats)
{
  std::cout << "Active sessions           : "
            << boost::lexical_cast<std::string>(stats.active)
            << std::endl
            << "Maximum of active sessions: "
            << boost::lexical_cast<std::string>(stats.max_active)
            << std::endl
            << "Recycled sessions         : "
            << boost::lexical_cast<std::string>(stats.recycled)
            << std::endl
            << "Total accepeted sessions  : "
            << to_string(stats.total_accepted)
            << std::endl
            << "Active shutdowned sessions: "
            << to_string(stats.active_shutdowned)
            << std::endl
            << "Out of work sessions      : "
            << to_string(stats.out_of_work)
            << std::endl
            << "Timed out sessions        : "
            << to_string(stats.timed_out)
            << std::endl
            << "Error stopped sessions    : "
            << to_string(stats.error_stopped)
            << std::endl;
}

int echo_server::run_server(const echo_server::execution_config& exec_config,
    const ma::echo::server::session_manager_config& session_manager_config)
{
  // Before session_manager_io_service for the right destruction order
  const io_service_vector session_io_services =
      create_session_io_services(exec_config);
  // After session_ios for the right destruction order
  const session_factory_ptr session_factory = create_session_factory(
      exec_config, session_manager_config, session_io_services);

  boost::asio::io_service session_manager_io_service(
      exec_config.session_manager_thread_count);

  const wrapped_session_manager_ptr session_manager(
      boost::make_shared<session_manager_wrapper>(
          boost::ref(session_manager_io_service),
          boost::ref(*session_factory),
          session_manager_config));

  std::cout << "Server is starting...\n";

  // Start the server
  session_manager_wrapper::unique_lock session_manager_lock(
      session_manager->mutex);
  start_session_manager_start(session_manager);
  session_manager->mark_as_starting();
  session_manager_lock.unlock();

  // Setup console controller
  ma::console_controller console_controller(
      boost::bind(handle_program_exit, session_manager));

  std::cout << "Press Ctrl+C (Ctrl+Break) to exit.\n";

  // Exception handler for automatic server aborting
  const exception_handler work_exception_handler(
      boost::bind(handle_work_exception, session_manager));

  // Create work for sessions' io_services to prevent threads' stop
  const io_service_work_vector session_work = 
      create_works(session_io_services);
  // Create work for session_manager's io_service to prevent threads' stop
  const boost::asio::io_service::work session_manager_work(
      session_manager_io_service);

  boost::thread_group work_threads;
  // Create work threads for session operations
  create_session_threads(work_threads, exec_config,
      session_io_services, work_exception_handler);
  // Create work threads for session manager operations
  create_session_manager_threads(work_threads, exec_config,
      session_manager_io_service, work_exception_handler);

  int exit_code = EXIT_SUCCESS;

  // Wait until server starts stopping or stops
  session_manager_lock.lock();
  while (!session_manager->is_stopping() && !session_manager->is_stopped())
  {
    session_manager->wait_for_state_change(session_manager_lock);
  }

  // Server is not working
  if (!session_manager->is_stopped())
  {
    // Wait until server stops with timeout
    if (!session_manager->wait_for_state_change(session_manager_lock,
        exec_config.stop_timeout))
    {
      // Timeout of server stop has expired - terminate server
      std::cout << "Server stop timeout expiration." \
            " Terminating server work...\n";
      exit_code = EXIT_FAILURE;
    }
    session_manager->mark_as_stopped();
  }

  // Check the reason of server stop and signal it by exit code
  if ((EXIT_SUCCESS == exit_code) && !session_manager->stopped_by_user)
  {
    exit_code = EXIT_FAILURE;
  }

  session_manager_lock.unlock();

  // Shutdown execution queues...
  session_manager_io_service.stop();
  stop(session_io_services);

  std::cout << "Server work was terminated." \
      " Waiting until all of the work threads will stop...\n";

  // ...and wait until all workers stop.
  work_threads.join_all();
  std::cout << "Work threads have stopped. Process will close.\n";

  print_stats(session_manager->stats());
  return exit_code;
}
