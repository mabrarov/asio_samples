//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <string>
#include <limits>
#include <boost/optional.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/throw_exception.hpp>
#include "config.hpp"

namespace echo_server {

namespace {

const char* help_option_name = "help";
const char* port_option_name = "port";
const char* session_manager_threads_option_name = "session_manager_threads";
const char* session_threads_option_name         = "session_threads";
const char* stop_timeout_option_name            = "stop_timeout";
const char* max_sessions_option_name            = "max_sessions";
const char* recycled_sessions_option_name       = "recycled_sessions";
const char* listen_backlog_option_name          = "listen_backlog";
const char* buffer_size_option_name             = "buffer";
const char* inactivity_timeout_option_name      = "inactivity_timeout";
const char* max_transfer_size_option_name       = "max_transfer";
const char* socket_recv_buffer_size_option_name = "sock_recv_buffer";
const char* socket_send_buffer_size_option_name = "sock_send_buffer";
const char* socket_no_delay_option_name         = "sock_no_delay";
const char* demux_option_name                   = "demux_per_work_thread";
const std::string default_system_value          = "system default";

template <typename Value>
void validate_option(const std::string& option_name, const Value& option_value,
    const Value& min, const Value& max = (std::numeric_limits<Value>::max)())
{
  if ((option_value < min) || (option_value > max))
  {
    using boost::program_options::validation_error;
    boost::throw_exception(validation_error(
        validation_error::invalid_option_value, std::string(), option_name));
  }
}

template <typename Value>
std::string to_string(const boost::optional<Value>& value,
    const std::string& default_text)
{
  if (value)
  {
    return boost::lexical_cast<std::string>(*value);
  }
  else
  {
    return default_text;
  }
}

std::string to_string(bool value)
{
  if (value)
  {
    return "on";
  }
  else
  {
    return "off";
  }
}

template <>
std::string to_string<bool>(const boost::optional<bool>& value,
    const std::string& default_text)
{
  if (value)
  {
    return to_string(*value);
  }
  else
  {
    return default_text;
  }
}

boost::optional<int> read_socket_buffer_size(
    const boost::program_options::variables_map& options_values,
    const std::string& option_name)
{
  if (!options_values.count(option_name))
  {
    return boost::optional<int>();
  }
  int buffer_size = options_values[option_name].as<int>();
  validate_option<int>(option_name, buffer_size, 0);
  return buffer_size;
}

std::size_t calc_session_manager_thread_count(
    std::size_t /*hardware_concurrency*/)
{
  return 1;
}

std::size_t calc_session_thread_count(std::size_t hardware_concurrency)
{
  if (hardware_concurrency)
  {
    return hardware_concurrency;
  }
  return 2;
}

} // anonymous namespace

boost::program_options::options_description build_cmd_options_description(
    std::size_t hardware_concurrency)
{
  boost::program_options::options_description description("Allowed options");

  std::size_t session_manager_thread_count =
      calc_session_manager_thread_count(hardware_concurrency);
  std::size_t session_thread_count =
      calc_session_thread_count(hardware_concurrency);

#if defined(WIN32)
  bool default_ios_per_work_thread = false;
#else
  bool default_ios_per_work_thread = true;
#endif

  description.add_options()
    (
      help_option_name,
      "produce help message"
    )
    (
      port_option_name,
      boost::program_options::value<unsigned short>(),
      "set the TCP port number for incoming connections' listening"
    )
    (
      session_manager_threads_option_name,
      boost::program_options::value<std::size_t>()->default_value(
          session_manager_thread_count),
      "set the number of session manager's threads"
    )
    (
      session_threads_option_name,
      boost::program_options::value<std::size_t>()->default_value(
          session_thread_count),
      "set the number of sessions' threads"
    )
    (
      stop_timeout_option_name,
      boost::program_options::value<long>()->default_value(60),
      "set the server stop timeout at one's expiration server work" \
          " will be terminated (seconds)"
    )
    (
      max_sessions_option_name,
      boost::program_options::value<std::size_t>()->default_value(10000),
      "set the maximum number of simultaneously active sessions"
    )
    (
      recycled_sessions_option_name,
      boost::program_options::value<std::size_t>()->default_value(100),
      "set the maximum number of pooled inactive sessions"
    )
    (
      listen_backlog_option_name,
      boost::program_options::value<int>()->default_value(6),
      "set the size of TCP listen backlog"
    )
    (
      buffer_size_option_name,
      boost::program_options::value<std::size_t>()->default_value(4096),
      "set the session's buffer size (bytes)"
    )
    (
      inactivity_timeout_option_name,
      boost::program_options::value<long>(),
      "set the timeout at one's expiration session will be considered" \
          " as inactive and will be closed (seconds)"
    )
    (
      max_transfer_size_option_name,
      boost::program_options::value<std::size_t>()->default_value(4096),
      "set the maximum size of single async transfer (bytes)"
    )
    (
      socket_recv_buffer_size_option_name,
      boost::program_options::value<int>(),
      "set the size of session's socket receive buffer (bytes)"
    )
    (
      socket_send_buffer_size_option_name,
      boost::program_options::value<int>(),
      "set the size of session's socket send buffer (bytes)"
    )
    (
      socket_no_delay_option_name,
      boost::program_options::value<bool>(),
      "set TCP_NODELAY option of session's socket"
    )
    (
      demux_option_name,
      boost::program_options::value<bool>()->default_value(
          default_ios_per_work_thread),
      "set demultiplexer-per-work-thread mode on"
    );

  return description;
}

#if defined(WIN32)
boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, _TCHAR* argv[])
#else
boost::program_options::variables_map parse_cmd_line(
    const boost::program_options::options_description& options_description,
    int argc, char* argv[])
#endif
{
  boost::program_options::variables_map values;
  boost::program_options::store(boost::program_options::parse_command_line(
      argc, argv, options_description), values);
  boost::program_options::notify(values);
  return values;
}

void print_config(std::ostream& stream, std::size_t cpu_count,
    const execution_config& exec_config,
    const ma::echo::server::session_manager_config& session_manager_config)
{
  const ma::echo::server::session_config& session_config =
      session_manager_config.managed_session_config;

  boost::optional<long> session_inactivity_timeout_sec;
  if (ma::echo::server::session_config::optional_time_duration timeout =
      session_config.inactivity_timeout)
  {
    session_inactivity_timeout_sec = timeout->total_seconds();
  }

  stream << "Number of found CPU(s)                : "
         << cpu_count
         << std::endl
         << "Number of session manager's threads   : "
         << exec_config.session_manager_thread_count
         << std::endl
         << "Number of sessions' threads           : "
         << exec_config.session_thread_count
         << std::endl
         << "Total number of work threads          : "
         << exec_config.session_thread_count
                + exec_config.session_manager_thread_count
         << std::endl
         << "Demultiplexer-per-work-thread mode    : "
         << to_string(exec_config.ios_per_work_thread)
         << std::endl
         << "Server listen port                    : "
         << session_manager_config.accepting_endpoint.port()
         << std::endl
         << "Server stop timeout (seconds)         : "
         << exec_config.stop_timeout.total_seconds()
         << std::endl
         << "Maximum number of active sessions     : "
         << session_manager_config.max_session_count
         << std::endl
         << "Maximum number of recycled sessions   : "
         << session_manager_config.recycled_session_count
         << std::endl
         << "TCP listen backlog size               : "
         << session_manager_config.listen_backlog
         << std::endl
         << "Size of session's buffer (bytes)      : "
         << session_config.buffer_size
         << std::endl
         << "Session's max size of single transfer (bytes)  : "
         << session_config.max_transfer_size
         << std::endl
         << "Session's inactivity timeout (seconds)         : "
         << to_string(session_inactivity_timeout_sec, "none")
         << std::endl
         << "Size of session's socket receive buffer (bytes): "
         << to_string(session_config.socket_recv_buffer_size,
                default_system_value)
         << std::endl
         << "Size of session's socket send buffer (bytes)   : "
         << to_string(session_config.socket_send_buffer_size,
                default_system_value)
         << std::endl
         << "Session's socket Nagle algorithm is            : "
         << to_string(session_config.no_delay, default_system_value)
         << std::endl;
}

bool is_help_mode(const boost::program_options::variables_map& options_values)
{
  return 0 != options_values.count(help_option_name);
}

bool is_required_specified(
    const boost::program_options::variables_map& options_values)
{
  return 0 != options_values.count(port_option_name);
}

execution_config build_execution_config(
    const boost::program_options::variables_map& options_values)
{
  std::size_t session_manager_thread_count =
      options_values[session_manager_threads_option_name].as<std::size_t>();

  validate_option<std::size_t>(session_manager_threads_option_name,
      session_manager_thread_count, 1);

  std::size_t session_thread_count =
      options_values[session_threads_option_name].as<std::size_t>();

  validate_option<std::size_t>(session_threads_option_name,
      session_thread_count, 1);

  long stop_timeout_sec = options_values[stop_timeout_option_name].as<long>();

  validate_option<long>(stop_timeout_option_name, stop_timeout_sec, 0);

  bool ios_per_work_thread =
      options_values[demux_option_name].as<bool>();

  return execution_config(ios_per_work_thread, session_manager_thread_count,
      session_thread_count, boost::posix_time::seconds(stop_timeout_sec));
}

ma::echo::server::session_config build_session_config(
    const boost::program_options::variables_map& options_values)
{
  using ma::echo::server::session_config;

  boost::optional<bool> no_delay;
  if (options_values.count(socket_no_delay_option_name))
  {
    no_delay = !options_values[socket_no_delay_option_name].as<bool>();
  }

  std::size_t buffer_size =
      options_values[buffer_size_option_name].as<std::size_t>();
  validate_option<std::size_t>(buffer_size_option_name, buffer_size, 1);

  session_config::optional_time_duration inactivity_timeout;
  if (options_values.count(inactivity_timeout_option_name))
  {
    long timeout_sec =
        options_values[inactivity_timeout_option_name].as<long>();
    validate_option<long>(inactivity_timeout_option_name, timeout_sec, 0);
    inactivity_timeout = boost::posix_time::seconds(timeout_sec);
  }

  std::size_t max_transfer_size =
      options_values[max_transfer_size_option_name].as<std::size_t>();
  validate_option<std::size_t>(
      max_transfer_size_option_name, max_transfer_size, 1);

  boost::optional<int> socket_recv_buffer_size = read_socket_buffer_size(
      options_values, socket_recv_buffer_size_option_name);

  boost::optional<int> socket_send_buffer_size = read_socket_buffer_size(
      options_values, socket_send_buffer_size_option_name);

  return session_config(buffer_size, max_transfer_size,
      socket_recv_buffer_size, socket_send_buffer_size, no_delay,
      inactivity_timeout);
}

ma::echo::server::session_manager_config build_session_manager_config(
    const boost::program_options::variables_map& options_values,
    const ma::echo::server::session_config& session_config)
{
  unsigned short port = options_values[port_option_name].as<unsigned short>();

  std::size_t max_sessions =
      options_values[max_sessions_option_name].as<std::size_t>();

  validate_option<std::size_t>(max_sessions_option_name, max_sessions, 1);

  std::size_t recycled_sessions =
      options_values[recycled_sessions_option_name].as<std::size_t>();

  int listen_backlog = options_values[listen_backlog_option_name].as<int>();

  using boost::asio::ip::tcp;

  return ma::echo::server::session_manager_config(
      tcp::endpoint(tcp::v4(), port), max_sessions, recycled_sessions,
      listen_backlog, session_config);
}

} // namespace echo_server
