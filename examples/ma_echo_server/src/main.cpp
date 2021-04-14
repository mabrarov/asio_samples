//
// Copyright (c) 2010-2015 Marat Abrarov (abrarov@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#if defined(WIN32)
#include <tchar.h>
#endif

#include <cstdlib>
#include <cstddef>
#include <vector>
#include <string>
#include <iostream>
#include <exception>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <ma/config.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/handler_invoke_helpers.hpp>
#include <ma/io_context_helpers.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/console_close_signal.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/thread_group.hpp>
#include <ma/echo/server/simple_session_factory.hpp>
#include <ma/echo/server/pooled_session_factory.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/detail/tuple.hpp>
#include <ma/detail/memory.hpp>
#include <ma/detail/functional.hpp>
#include <ma/detail/thread.hpp>
#include <ma/detail/utility.hpp>
#include "config.hpp"

namespace echo_server {

int run_server(const execution_config&,
    const ma::echo::server::session_manager_config&);

} // namespace echo_server

#if defined(MA_WIN32_TMAIN)
int _tmain(int argc, _TCHAR* argv[])
#else
int main(int argc, char* argv[])
#endif
{
  try
  {
    using namespace echo_server;

    const std::size_t cpu_count = ma::detail::thread::hardware_concurrency();
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

typedef ma::detail::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef std::vector<io_service_ptr> io_service_vector;
typedef ma::detail::shared_ptr<ma::echo::server::session_factory>
    session_factory_ptr;
typedef ma::detail::shared_ptr<boost::asio::io_service::work>
    io_service_work_ptr;
typedef std::vector<io_service_work_ptr>  io_service_work_vector;

class server : private boost::noncopyable
{
private:
  typedef server this_type;

public:
  template <typename Handler>
  server(const echo_server::execution_config& execution_config,
      const ma::echo::server::session_manager_config& session_manager_config,
      const Handler& exception_handler)
    : session_io_services_(create_session_io_services(execution_config))
    , session_factory_(create_session_factory(execution_config,
        session_manager_config, session_io_services_))
    , session_manager_io_service_(ma::to_io_context_concurrency_hint(
          execution_config.session_manager_thread_count))
    , threads_stopped_(false)
    , session_work_(create_works(session_io_services_))
    , session_manager_work_(session_manager_io_service_)
    , threads_()
    , session_manager_(ma::echo::server::session_manager::create(
          session_manager_io_service_, *session_factory_,
          session_manager_config))
  {
    create_threads(exception_handler, execution_config.ios_per_work_thread,
        execution_config.session_manager_thread_count,
        execution_config.session_thread_count);
  }

  ~server()
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

  template <typename Handler>
  void async_start(MA_FWD_REF(Handler) handler)
  {
    session_manager_->async_start(ma::detail::forward<Handler>(handler));
  }

  template <typename Handler>
  void async_wait(MA_FWD_REF(Handler) handler)
  {
    session_manager_->async_wait(ma::detail::forward<Handler>(handler));
  }

  template <typename Handler>
  void async_stop(MA_FWD_REF(Handler) handler)
  {
    session_manager_->async_stop(ma::detail::forward<Handler>(handler));
  }

  ma::echo::server::session_manager_stats stats() const
  {
    return session_manager_->stats();
  }

private:
  const io_service_vector session_io_services_;
  const session_factory_ptr session_factory_;
  boost::asio::io_service session_manager_io_service_;
  bool threads_stopped_;
  const io_service_work_vector session_work_;
  const boost::asio::io_service::work session_manager_work_;
  ma::thread_group threads_;
  const ma::echo::server::session_manager_ptr session_manager_;

  static io_service_vector create_session_io_services(
      const echo_server::execution_config& exec_config)
  {
    namespace detail = ma::detail;

    io_service_vector io_services;
    if (exec_config.ios_per_work_thread)
    {
      for (std::size_t i = 0; i != exec_config.session_thread_count; ++i)
      {
        io_services.push_back(detail::make_shared<boost::asio::io_service>(
            ma::to_io_context_concurrency_hint(1)));
      }
    }
    else
    {
      io_service_ptr io_service = detail::make_shared<boost::asio::io_service>(
          ma::to_io_context_concurrency_hint(exec_config.session_thread_count));
      io_services.push_back(io_service);
    }
    return io_services;
  }

  static session_factory_ptr create_session_factory(
      const echo_server::execution_config& exec_config,
      const ma::echo::server::session_manager_config& session_manager_config,
      const io_service_vector& session_io_services)
  {
    using ma::echo::server::pooled_session_factory;
    using ma::echo::server::simple_session_factory;
    namespace detail = ma::detail;

    if (exec_config.ios_per_work_thread)
    {
      return detail::make_shared<pooled_session_factory>(session_io_services,
          session_manager_config.recycled_session_count);
    }
    else
    {
      boost::asio::io_service& io_service = *session_io_services.front();
      return detail::make_shared<simple_session_factory>(
          detail::ref(io_service),
          session_manager_config.recycled_session_count);
    }
  }

  template <typename Handler>
  void create_threads(const Handler& handler, bool ios_per_work_thread,
      std::size_t session_manager_thread_count,
      std::size_t session_thread_count)
  {
    namespace detail = ma::detail;

    typedef detail::tuple<Handler> wrapped_handler_type;
    typedef void (*thread_func_type)(boost::asio::io_service&,
        wrapped_handler_type);

    wrapped_handler_type wrapped_handler = detail::make_tuple(handler);
    thread_func_type func = &this_type::thread_func<Handler>;

    if (ios_per_work_thread)
    {
      for (io_service_vector::const_iterator i = session_io_services_.begin(),
          end = session_io_services_.end(); i != end; ++i)
      {
        threads_.create_thread(
            detail::bind(func, detail::ref(**i), wrapped_handler));
      }
    }
    else
    {
      boost::asio::io_service& io_service = *session_io_services_.front();
      for (std::size_t i = 0; i != session_thread_count; ++i)
      {
        threads_.create_thread(
            detail::bind(func, detail::ref(io_service), wrapped_handler));
      }
    }

    for (std::size_t i = 0; i != session_manager_thread_count; ++i)
    {
      threads_.create_thread(
          detail::bind(func, detail::ref(session_manager_io_service_),
              wrapped_handler));
    }
  }

  template <typename Handler>
  static void thread_func(boost::asio::io_service& io_service,
      ma::detail::tuple<Handler> handler)
  {
    try
    {
      io_service.run();
    }
    catch (...)
    {
      ma_handler_invoke_helpers::invoke(
          ma::detail::get<0>(handler), ma::detail::get<0>(handler));
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
          ma::detail::make_shared<boost::asio::io_service::work>(
              ma::detail::ref(**i)));
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
}; // class server

struct execution_context : private boost::noncopyable
{
public:
  enum state_t {starting, working, stopping, stopped};

  execution_context(const echo_server::execution_config& the_exec_config,
      boost::asio::io_service& the_event_loop,
      ma::steady_deadline_timer& the_stop_timer,
      ma::console_close_signal& the_close_signal)
    : exec_config(the_exec_config)
    , event_loop(the_event_loop)
    , stop_timer(the_stop_timer)
    , close_signal(the_close_signal)
    , state(starting)
    , user_initiated_stop(false)
  {
  }

  const echo_server::execution_config& exec_config;
  boost::asio::io_service&   event_loop;
  ma::steady_deadline_timer& stop_timer;
  ma::console_close_signal&  close_signal;
  state_t state;
  bool    user_initiated_stop;
}; // struct execution_context

void stop_event_loop(execution_context& context);

void start_server_stop(execution_context& context, server& the_server,
    bool user_initiated_stop);

void handle_work_thread_exception(execution_context& context);

void handle_app_exit(execution_context& context, server& the_server);

void handle_server_start(execution_context& context, server& the_server,
    const boost::system::error_code& error);

void handle_server_wait(execution_context& context, server& the_server,
    const boost::system::error_code& error);

void handle_server_stop_timeout(execution_context& context,
    const boost::system::error_code& error);

void handle_server_stop(execution_context& context,
    const boost::system::error_code& error);

void stop_event_loop(execution_context& context)
{
  context.event_loop.stop();
  context.state = execution_context::stopped;
}

void start_server_stop(execution_context& context, server& the_server,
    bool user_initiated_stop)
{
  namespace detail = ma::detail;

  the_server.async_stop(context.event_loop.wrap(detail::bind(
      handle_server_stop, detail::ref(context), detail::placeholders::_1)));

  context.state = execution_context::stopping;
  context.user_initiated_stop = user_initiated_stop;

  // Start timer for server stop
  context.stop_timer.expires_from_now(ma::to_steady_deadline_timer_duration(
      context.exec_config.stop_timeout));
  context.stop_timer.async_wait(context.event_loop.wrap(detail::bind(
      handle_server_stop_timeout, detail::ref(context),
      detail::placeholders::_1)));

  context.close_signal.async_wait(context.event_loop.wrap(detail::bind(
      handle_app_exit, detail::ref(context), detail::ref(the_server))));

  std::cout << "Server is stopping." \
      " Press Ctrl+C to terminate server." << std::endl;
}

void handle_work_thread_exception(execution_context& context)
{
  switch (context.state)
  {
  case execution_context::stopped:
    // Do nothing - server has already stopped
    break;

  default:
    std::cout << "Terminating server due to unexpected error." << std::endl;
    stop_event_loop(context);
    break;
  }
}

void handle_app_exit(execution_context& context, server& the_server)
{
  namespace detail = ma::detail;

  std::cout << "Application exit request detected." << std::endl;

  switch (context.state)
  {
  case execution_context::stopped:
    std::cout << "Server has already stopped." << std::endl;
    break;

  case execution_context::stopping:
    std::cout << "Server is already stopping. Terminating server."
              << std::endl;
    stop_event_loop(context);
    break;

  default:
    start_server_stop(context, the_server, true);
    break;
  }
}

void handle_server_start(execution_context& context, server& the_server,
    const boost::system::error_code& error)
{
  namespace detail = ma::detail;

  switch (context.state)
  {
  case execution_context::starting:
    if (error)
    {
      std::cout << "Server can't start due to error: " << error.message()
                << std::endl;
      stop_event_loop(context);
    }
    else
    {
      std::cout << "Server has started." << std::endl;
      context.state = execution_context::working;
      the_server.async_wait(context.event_loop.wrap(detail::bind(
          handle_server_wait, detail::ref(context), detail::ref(the_server),
          detail::placeholders::_1)));
    }
    break;

  default:
    // Do nothing - we are late
    break;
  }
}

void handle_server_wait(execution_context& context, server& the_server,
    const boost::system::error_code& error)
{
  namespace detail = ma::detail;

  switch (context.state)
  {
  case execution_context::working:
    if (error)
    {
      std::cout << "Server can't continue work due to error: "
                << error.message() << std::endl;
    }
    else
    {
      std::cout << "Server can't continue work." << std::endl;
    }
    start_server_stop(context, the_server, false);
    break;

  default:
    // Do nothing - we are late
    break;
  }
}

void handle_server_stop_timeout(execution_context& context,
    const boost::system::error_code& error)
{
  if (boost::asio::error::operation_aborted == error)
  {
    return;
  }

  switch (context.state)
  {
  case execution_context::stopping:
    std::cout << "Stop timeout expired. Terminating server." << std::endl;
    stop_event_loop(context);
    break;

  default:
    // Nothing to handle
    break;
  }
}

void handle_server_stop(execution_context& context,
    const boost::system::error_code&)
{
  std::cout << "Server has stopped." << std::endl;
  stop_event_loop(context);
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
            << "Total accepted sessions    : "
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
  namespace detail = ma::detail;

  boost::asio::io_service   event_loop(ma::to_io_context_concurrency_hint(1));
  ma::steady_deadline_timer stop_timer(event_loop);
  ma::console_close_signal  close_signal(event_loop);
  execution_context context(exec_config, event_loop, stop_timer, close_signal);

  server the_server(exec_config, session_manager_config, event_loop.wrap(
      detail::bind(handle_work_thread_exception, detail::ref(context))));

  // Wait for console close
  std::cout << "Press Ctrl+C to exit." << std::endl;
  close_signal.async_wait(event_loop.wrap(detail::bind(handle_app_exit,
      detail::ref(context), detail::ref(the_server))));

  std::cout << "Server is starting." << std::endl;
  the_server.async_start(event_loop.wrap(detail::bind(handle_server_start,
      detail::ref(context), detail::ref(the_server),
      detail::placeholders::_1)));

  // Run event loop
  boost::asio::io_service::work event_loop_stop_guard(event_loop);
  event_loop.run();
  (void) event_loop_stop_guard;

  std::cout << "Waiting until work threads stop." << std::endl;
  the_server.stop_threads();
  std::cout << "Work threads have stopped." << std::endl;

  print_stats(the_server.stats());

  return context.user_initiated_stop ? EXIT_SUCCESS : EXIT_FAILURE;
}
