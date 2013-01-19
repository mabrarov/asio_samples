//
// Copyright (c) 2010-2013 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#include <windows.h>
#endif

#include <cstdlib>
#include <cstddef>
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
#include <boost/tuple/tuple.hpp>
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
    if (help_requested(cmd_options))
    {
      std::cout << cmd_options_description;
      return EXIT_SUCCESS;
    }

    // Check required options
    if (!required_options_specified(cmd_options))
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

    // Show actual server configuration
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
    std::cerr << "Unexpected error: " << e.what() << std::endl;
  }
  catch (...)
  {
    std::cerr << "Unknown error" << std::endl;
  }
  return EXIT_FAILURE;
}

namespace {

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef std::vector<io_service_ptr>                io_service_vector;
typedef boost::shared_ptr<ma::echo::server::session_factory>
    session_factory_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> io_service_work_ptr;
typedef std::vector<io_service_work_ptr> io_service_work_vector;

class server_base_0 : private boost::noncopyable
{
public:
  explicit server_base_0(const echo_server::execution_config& config)
    : ios_per_work_thread_(config.ios_per_work_thread)
    , session_manager_thread_count_(config.session_manager_thread_count)
    , session_thread_count_(config.session_thread_count)
    , session_io_services_(create_session_io_services(config))
  {
  }

  io_service_vector session_io_services() const
  {
    return session_io_services_;
  }

protected:
  ~server_base_0()
  {
  }

  const bool ios_per_work_thread_;
  const std::size_t session_manager_thread_count_;
  const std::size_t session_thread_count_;
  const io_service_vector session_io_services_;

private:
  static io_service_vector create_session_io_services(
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
      io_service_ptr io_service = boost::make_shared<boost::asio::io_service>(
          exec_config.session_thread_count);
      io_services.push_back(io_service);
    }
    return io_services;
  }
}; // class server_base_0

class server_base_1 : public server_base_0
{
public:
  server_base_1(const echo_server::execution_config& execution_config,
      const ma::echo::server::session_manager_config& session_manager_config)
    : server_base_0(execution_config)
    , session_factory_(create_session_factory(execution_config,
          session_manager_config, session_io_services_))
  {
  }

  ma::echo::server::session_factory& session_factory() const
  {
    return *session_factory_;
  }

protected:
  ~server_base_1()
  {
  }

  const session_factory_ptr session_factory_;

private:
  static session_factory_ptr create_session_factory(
      const echo_server::execution_config& exec_config,
      const ma::echo::server::session_manager_config& session_manager_config,
      const io_service_vector& session_io_services)
  {
    if (exec_config.ios_per_work_thread)
    {
      return boost::make_shared<ma::echo::server::pooled_session_factory>(
          session_io_services, session_manager_config.recycled_session_count);
    }
    else
    {
      boost::asio::io_service& io_service = *session_io_services.front();
      return boost::make_shared<ma::echo::server::simple_session_factory>(
          boost::ref(io_service),
          session_manager_config.recycled_session_count);
    }
  }
}; // class server_base_1

class server_base_2 : public server_base_1
{
public:
  explicit server_base_2(const echo_server::execution_config& execution_config,
      const ma::echo::server::session_manager_config& session_manager_config)
    : server_base_1(execution_config, session_manager_config)
    , session_manager_io_service_(
          execution_config.session_manager_thread_count)
  {
  }

  boost::asio::io_service& session_manager_io_service()
  {
    return session_manager_io_service_;
  }

protected:
  ~server_base_2()
  {
  }

  boost::asio::io_service session_manager_io_service_;
}; // class server_base_2

class server_base_3 : public server_base_2
{
private:
  typedef server_base_3 this_type;

public:
  template <typename Handler>
  server_base_3(const echo_server::execution_config& execution_config,
      const ma::echo::server::session_manager_config& session_manager_config,
      const Handler& exception_handler)
    : server_base_2(execution_config, session_manager_config)
    , threads_stopped_(false)
    , session_work_(create_works(session_io_services_))
    , session_manager_work_(session_manager_io_service_)
    , threads_()
  {
    create_threads(exception_handler);
  }

  ~server_base_3()
  {
    stop_threads();
  }

  void stop_threads()
  {
    if (!threads_stopped_)
    {
      session_manager_io_service_.stop();
      stop(session_io_services_);
      threads_.join_all();
      threads_stopped_ = true;
    }
  }

private:
  template <typename Handler>
  void create_threads(const Handler& handler)
  {
    typedef boost::tuple<Handler> wrapped_handler_type;
    typedef void (*thread_func_type)(boost::asio::io_service&,
        wrapped_handler_type);

    wrapped_handler_type wrapped_handler = boost::make_tuple(handler);
    thread_func_type func = &this_type::thread_func<Handler>;

    if (ios_per_work_thread_)
    {
      for (io_service_vector::const_iterator i = session_io_services_.begin(),
          end = session_io_services_.end(); i != end; ++i)
      {
        threads_.create_thread(
            boost::bind(func, boost::ref(**i), wrapped_handler));
      }
    }
    else
    {
      boost::asio::io_service& io_service = *session_io_services_.front();
      for (std::size_t i = 0; i != session_thread_count_; ++i)
      {
        threads_.create_thread(
            boost::bind(func, boost::ref(io_service), wrapped_handler));
      }
    }

    for (std::size_t i = 0; i != session_manager_thread_count_; ++i)
    {
      threads_.create_thread(boost::bind(func,
          boost::ref(session_manager_io_service_), wrapped_handler));
    }
  }

  template <typename Handler>
  static void thread_func(boost::asio::io_service& io_service,
      boost::tuple<Handler> handler)
  {
    try
    {
      io_service.run();
    }
    catch (...)
    {
      boost::get<0>(handler)();
    }
  }

  static io_service_work_vector create_works(
      const io_service_vector& io_services)
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

  static void stop(const io_service_vector& io_services)
  {
    for (io_service_vector::const_iterator i = io_services.begin(),
        end = io_services.end(); i != end; ++i)
    {
      (*i)->stop();
    }
  }

  bool threads_stopped_;
  const io_service_work_vector session_work_;
  const boost::asio::io_service::work session_manager_work_;
  boost::thread_group threads_;
}; // class server_base_3

class server : public server_base_3
{
private:
  typedef server this_type;

public:
  template <typename Handler>
  server(const echo_server::execution_config& execution_config,
      const ma::echo::server::session_manager_config& session_manager_config,
      const Handler& exception_handler)
    : server_base_3(execution_config, session_manager_config,
          exception_handler)
    , session_manager_(ma::echo::server::session_manager::create(
          session_manager_io_service_, *session_factory_,
          session_manager_config))
  {
  }

  ~server()
  {
  }

  template <typename Handler>
  void async_start(const Handler& handler)
  {
    session_manager_->async_start(handler);
  }

  template <typename Handler>
  void async_wait(const Handler& handler)
  {
    session_manager_->async_wait(handler);
  }

  template <typename Handler>
  void async_stop(const Handler& handler)
  {
    session_manager_->async_stop(handler);
  }

  ma::echo::server::session_manager_stats stats() const
  {
    return session_manager_->stats();
  }

private:
  const ma::echo::server::session_manager_ptr session_manager_;
}; // class server

struct server_state : private boost::noncopyable
{
public:
  typedef boost::mutex                   mutex_type;
  typedef boost::lock_guard<mutex_type>  lock_guard;
  typedef boost::unique_lock<mutex_type> unique_lock;

  enum value_t {starting, working, stopping, stopped};

  server_state()
    : value(starting)
    , user_initiated_stop(false)
  {
  }

  mutex_type                 mutex;
  boost::condition_variable  condition_variable;
  value_t                    value;
  bool                       user_initiated_stop;
}; // struct server_state

void switch_to_stopped(const server_state::lock_guard&,
    server_state& the_server_state)
{
  the_server_state.value = server_state::stopped;
  the_server_state.condition_variable.notify_all();
}

void switch_to_working(const server_state::lock_guard&,
    server_state& the_server_state)
{
  the_server_state.value = server_state::working;
  the_server_state.condition_variable.notify_all();
}

void switch_to_stopping(const server_state::lock_guard&,
    server_state& the_server_state, bool user_initiated)
{
  the_server_state.value = server_state::stopping;
  the_server_state.user_initiated_stop = user_initiated;
  the_server_state.condition_variable.notify_all();
}

void wait_until_server_stopping(server_state& the_server_state)
{
  server_state::unique_lock lock(the_server_state.mutex);
  while ((server_state::stopping != the_server_state.value)
      && (server_state::stopped  != the_server_state.value))
  {
    the_server_state.condition_variable.wait(lock);
  }
}

bool wait_until_server_stopped(server_state& the_server_state,
  const boost::posix_time::time_duration& duration)
{
  server_state::unique_lock lock(the_server_state.mutex);
  if (server_state::stopped == the_server_state.value)
  {
    return true;
  }
  return the_server_state.condition_variable.timed_wait(lock, duration);
}

void handle_work_thread_exception(server_state&);
void handle_app_exit(server_state&, server&);
void handle_server_start(server_state&, server&,
    const boost::system::error_code&);
void handle_server_wait(server_state&, server&,
    const boost::system::error_code&);
void handle_server_stop(server_state&, server&,
    const boost::system::error_code&);

void handle_work_thread_exception(server_state& the_server_state)
{
  server_state::lock_guard lock_guard(the_server_state.mutex);
  switch (the_server_state.value)
  {
  case server_state::stopped:
    // Do nothing - server has already stopped
    break;

  default:
    std::cout << "Terminating server due to unexpected error." << std::endl;
    switch_to_stopped(lock_guard, the_server_state);
  }
}

void handle_app_exit(server_state& the_server_state, server& the_server)
{
  std::cout << "Application exit request detected." << std::endl;

  server_state::lock_guard lock_guard(the_server_state.mutex);
  switch (the_server_state.value)
  {
  case server_state::stopped:
    std::cout << "Server has already stopped." << std::endl;
    break;

  case server_state::stopping:
    std::cout << "Server is already stopping. Terminating server."
              << std::endl;
    switch_to_stopped(lock_guard, the_server_state);
    break;

  default:
    the_server.async_stop(boost::bind(handle_server_stop,
        boost::ref(the_server_state), boost::ref(the_server), _1));
    switch_to_stopping(lock_guard, the_server_state, true);
    std::cout << "Server is stopping." \
        " Press Ctrl+C (Ctrl+Break) to terminate server." << std::endl;
    break;
  }
}

void handle_server_start(server_state& the_server_state, server& the_server,
    const boost::system::error_code& error)
{
  server_state::lock_guard lock_guard(the_server_state.mutex);
  switch (the_server_state.value)
  {
  case server_state::starting:
    if (error)
    {
      std::cout << "Server can't start due to error: "
                << error.message() << std::endl;
      switch_to_stopped(lock_guard, the_server_state);
    }
    else
    {
      std::cout << "Server has started." << std::endl;
      switch_to_working(lock_guard, the_server_state);
      the_server.async_wait(boost::bind(handle_server_wait,
          boost::ref(the_server_state), boost::ref(the_server), _1));
    }
    break;

  default:
    // Do nothing - we are late
    break;
  }
}

void handle_server_wait(server_state& the_server_state, server& the_server,
    const boost::system::error_code& error)
{
  server_state::lock_guard lock_guard(the_server_state.mutex);
  switch (the_server_state.value)
  {
  case server_state::working:
    if (error)
    {
      std::cout << "Server can't continue work due to error: "
                << error.message() << std::endl;
    }
    else
    {
      std::cout << "Server can't continue work." << std::endl;
    }
    the_server.async_stop(boost::bind(handle_server_stop,
        boost::ref(the_server_state), boost::ref(the_server), _1));
    switch_to_stopping(lock_guard, the_server_state, false);
    break;

  default:
    // Do nothing - we are late
    break;
  }
}

void handle_server_stop(server_state& the_server_state, server&,
    const boost::system::error_code&)
{
  server_state::lock_guard lock_guard(the_server_state.mutex);
  std::cout << "Server has stopped." << std::endl;
  switch_to_stopped(lock_guard, the_server_state);
}

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

void print_stats(const ma::echo::server::session_manager_stats& stats)
{
  std::cout << "Active sessions            : "
            << boost::lexical_cast<std::string>(stats.active)
            << std::endl
            << "Maximum of active sessions : "
            << boost::lexical_cast<std::string>(stats.max_active)
            << std::endl
            << "Recycled sessions          : "
            << boost::lexical_cast<std::string>(stats.recycled)
            << std::endl
            << "Total accepeted sessions   : "
            << to_string(stats.total_accepted)
            << std::endl
            << "Active shutdowned sessions : "
            << to_string(stats.active_shutdowned)
            << std::endl
            << "Passive shutdowned sessions: "
            << to_string(stats.out_of_work)
            << std::endl
            << "Timed out sessions         : "
            << to_string(stats.timed_out)
            << std::endl
            << "Error stopped sessions     : "
            << to_string(stats.error_stopped)
            << std::endl;
}

} // anonymous namespace

int echo_server::run_server(const echo_server::execution_config& exec_config,
    const ma::echo::server::session_manager_config& session_manager_config)
{
  server_state the_server_state;

  server the_server(exec_config, session_manager_config,
      boost::bind(handle_work_thread_exception, boost::ref(the_server_state)));

  std::cout << "Server is starting." << std::endl;
  the_server.async_start(boost::bind(handle_server_start,
      boost::ref(the_server_state), boost::ref(the_server), _1));

  // Lookup for app termination
  ma::console_controller console_controller(boost::bind(handle_app_exit,
      boost::ref(the_server_state), boost::ref(the_server)));
  std::cout << "Press Ctrl+C (Ctrl+Break) to exit." << std::endl;

  int exit_code = EXIT_SUCCESS;

  wait_until_server_stopping(the_server_state);
  if (!wait_until_server_stopped(the_server_state, exec_config.stop_timeout))
  {
    // Timeout of server stop has expired - terminate server
    std::cout << "Server stop timeout expiration. Terminating server."
              << std::endl;
    exit_code = EXIT_FAILURE;
  }

  // Check the reason of server stop and signal it by exit code
  if (EXIT_SUCCESS == exit_code)
  {
    server_state::lock_guard lock_guard(the_server_state.mutex);
    if (!the_server_state.user_initiated_stop)
    {
      exit_code = EXIT_FAILURE;
    }
  }

  std::cout << "Waiting until work threads stop." << std::endl;
  the_server.stop_threads();
  std::cout << "Work threads have stopped." << std::endl;

  print_stats(the_server.stats());
  return exit_code;
}
