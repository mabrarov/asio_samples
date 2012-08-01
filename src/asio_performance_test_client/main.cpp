//
// client.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <iostream>
#include <vector>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/async_connect.hpp>
#include <ma/steady_deadline_timer.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>

namespace {

class work_state : private boost::noncopyable
{
public:
  explicit work_state(std::size_t session_count)
    : session_count_(session_count)
  {
  }

  void notify_session_stop()
  {
    boost::mutex::scoped_lock lock(mutex_);
    --session_count_;
    if (!session_count_)
    {
      condition_.notify_all();
    }
  }

  void wait_for_all_session_stop(
      const boost::posix_time::time_duration& timeout)
  {
    boost::unique_lock<boost::mutex> lock(mutex_);
    if (session_count_)
    {
      condition_.timed_wait(lock, timeout);
    }
  }

private:
  std::size_t session_count_;
  boost::mutex mutex_;
  boost::condition_variable condition_;
}; // struct work_state

class stats : private boost::noncopyable
{
public:
  stats()
    : total_sessions_connected_(0)
    , total_bytes_written_(0)
    , total_bytes_read_(0)
  {
  }

  void add(boost::uint_fast64_t bytes_written, boost::uint_fast64_t bytes_read)
  {
    ++total_sessions_connected_;
    total_bytes_written_ += bytes_written;
    total_bytes_read_ += bytes_read;
  }

  void print()
  {
    std::cout << "Total sessions connected: " << total_sessions_connected_
              << std::endl
              << "Total bytes written     : " << total_bytes_written_
              << std::endl
              << "Total bytes read        : " << total_bytes_read_
              << std::endl;
  }

private:
  std::size_t total_sessions_connected_;
  boost::uint_fast64_t total_bytes_written_;
  boost::uint_fast64_t total_bytes_read_;
}; // class stats

class session : private boost::noncopyable
{
  typedef session this_type;

public:
  typedef boost::asio::ip::tcp protocol;

  session(boost::asio::io_service& io_service, std::size_t buffer_size,
      std::size_t max_connect_attempts, work_state& work_state)
    : strand_(io_service)
    , socket_(io_service)
    , buffer_(buffer_size)
    , max_connect_attempts_(max_connect_attempts)
    , bytes_written_(0)
    , bytes_read_(0)
    , was_connected_(false)
    , write_in_progress_(false)
    , read_in_progress_(false)
    , stopped_(false)
    , work_state_(work_state)
  {
    typedef ma::cyclic_buffer::mutable_buffers_type buffers_type;
    std::size_t filled_size = buffer_size / 2;
    const buffers_type data = buffer_.prepared();
    std::size_t size_to_fill = filled_size;
    for (buffers_type::const_iterator i = data.begin(), end = data.end();
        size_to_fill && (i != end); ++i)
    {
      char* b = boost::asio::buffer_cast<char*>(*i);
      std::size_t s = boost::asio::buffer_size(*i);
      for (; size_to_fill && s; ++b, --size_to_fill, --s)
      {
        *b = static_cast<char>(buffer_size % 128);
      }
    }
    buffer_.consume(filled_size);
  }

  void async_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(write_allocator_,
        boost::bind(&this_type::do_start, this, 0, endpoint_iterator)));
  }

  void async_stop()
  {
    strand_.post(ma::make_custom_alloc_handler(stop_allocator_,
        boost::bind(&this_type::do_stop, this)));
  }

  bool was_connected() const
  {
    return was_connected_;
  }

  boost::uint_fast64_t bytes_written() const
  {
    return bytes_written_;
  }

  boost::uint_fast64_t bytes_read() const
  {
    return bytes_read_;
  }

private:
  void do_start(std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    start_connect(connect_attempt,
        initial_endpoint_iterator, initial_endpoint_iterator);
  }

  void start_connect(std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator,
      const protocol::resolver::iterator& current_endpoint_iterator)
  {
    protocol::endpoint endpoint = *current_endpoint_iterator;
    ma::async_connect(socket_, endpoint, MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(write_allocator_,
            boost::bind(&this_type::handle_connect, this, _1, connect_attempt,
                initial_endpoint_iterator, current_endpoint_iterator))));
  }

  void handle_connect(const boost::system::error_code& error,
      std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator,
      protocol::resolver::iterator current_endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    if (error)
    {
      close_socket();

      ++current_endpoint_iterator;
      if (protocol::resolver::iterator() != current_endpoint_iterator)
      {
        start_connect(connect_attempt,
            initial_endpoint_iterator, current_endpoint_iterator);
        return;
      }

      if (max_connect_attempts_)
      {
        ++connect_attempt;
        if (connect_attempt >= max_connect_attempts_)
        {
          do_stop();
          return;
        }
      }

      start_connect(connect_attempt,
          initial_endpoint_iterator, initial_endpoint_iterator);
      return;
    }

    was_connected_ = true;

    if (boost::system::error_code error = apply_socket_options())
    {
      do_stop();
      return;
    }

    start_write_some();
    start_read_some();
  }

  void handle_read(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    read_in_progress_ = false;
    bytes_read_ += bytes_transferred;
    buffer_.consume(bytes_transferred);

    if (stopped_)
    {
      return;
    }

    if (error)
    {
      do_stop();
      return;
    }

    if (!write_in_progress_)
    {
      start_write_some();
    }
    start_read_some();
  }

  void handle_write(const boost::system::error_code& error,
      std::size_t bytes_transferred)
  {
    write_in_progress_ = false;
    bytes_written_ += bytes_transferred;
    buffer_.commit(bytes_transferred);

    if (stopped_)
    {
      return;
    }

    if (error)
    {
      do_stop();
      return;
    }

    if (!read_in_progress_)
    {
      start_read_some();
    }
    start_write_some();
  }

  void do_stop()
  {
    if (stopped_)
    {
      return;
    }

    close_socket();
    stopped_ = true;
    work_state_.notify_session_stop();
  }

  void start_write_some()
  {
    ma::cyclic_buffer::const_buffers_type write_data = buffer_.data();
    if (!write_data.empty())
    {
      socket_.async_write_some(write_data, MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(write_allocator_,
              boost::bind(&this_type::handle_write, this, _1, _2))));
      write_in_progress_ = true;
    }
  }

  void start_read_some()
  {
    ma::cyclic_buffer::mutable_buffers_type read_data = buffer_.prepared();
    if (!read_data.empty())
    {
      socket_.async_read_some(read_data, MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(read_allocator_,
              boost::bind(&this_type::handle_read, this, _1, _2))));
      read_in_progress_ = true;
    }
  }

  boost::system::error_code apply_socket_options()
  {
    typedef protocol::socket socket_type;

    {
      boost::system::error_code error;
      protocol::no_delay no_delay(true);
      socket_.set_option(no_delay, error);
      if (error)
      {
        return error;
      }
    }

    return boost::system::error_code();
  }

  void close_socket()
  {
    boost::system::error_code ignored;
    socket_.close(ignored);
  }

  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  ma::cyclic_buffer buffer_;
  const std::size_t max_connect_attempts_;
  boost::uint_fast64_t bytes_written_;
  boost::uint_fast64_t bytes_read_;
  bool was_connected_;
  bool write_in_progress_;
  bool read_in_progress_;
  bool stopped_;
  work_state& work_state_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> read_allocator_;
  ma::in_place_handler_allocator<512> write_allocator_;
}; // class session

typedef boost::shared_ptr<session>     session_ptr;
typedef ma::steady_deadline_timer      deadline_timer;
typedef deadline_timer::duration_type  duration_type;
typedef boost::optional<duration_type> optional_duration;

class client : private boost::noncopyable
{
private:
  typedef client this_type;

public:
  typedef session::protocol protocol;

  client(boost::asio::io_service& io_service, std::size_t session_count,
      std::size_t block_size, const optional_duration& block_pause,
      std::size_t buffer_size, std::size_t max_connect_attempts)
    : block_size_(block_size)
    , block_pause_(block_pause)
    , io_service_(io_service)
    , strand_(io_service)
    , timer_(io_service)
    , sessions_()
    , stopped_(false)
    , timer_in_progess_(false)
    , stats_()
    , work_state_(session_count)
  {
    for (std::size_t i = 0; i < session_count; ++i)
    {
      sessions_.push_back(boost::make_shared<session>(boost::ref(io_service_),
          buffer_size, max_connect_attempts, boost::ref(work_state_)));
    }
  }

  ~client()
  {
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&this_type::register_stats, this, _1));
    stats_.print();
  }

  void async_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(start_allocator_,
        boost::bind(&this_type::do_start, this, endpoint_iterator)));
  }

  void async_stop()
  {
    strand_.post(ma::make_custom_alloc_handler(stop_allocator_,
        boost::bind(&this_type::do_stop, this)));
  }

  void wait_until_done(const boost::posix_time::time_duration& timeout)
  {
    work_state_.wait_for_all_session_stop(timeout);
  }

private:
  typedef std::vector<session_ptr> session_vector;

  std::size_t start_block(
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t offset, std::size_t max_block_size)
  {
    std::size_t block_size = 0;
    for (session_vector::const_iterator i = sessions_.begin() + offset,
        end = sessions_.end(); (i != end) && (block_size != max_block_size);
        ++i, ++block_size)
    {
      (*i)->async_start(endpoint_iterator);
    }
    return offset + block_size;
  }

  void do_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    if (block_pause_)
    {
      std::size_t offset = start_block(endpoint_iterator, 0, block_size_);
      if (offset < sessions_.size())
      {
        start_deferred_session_start(endpoint_iterator, offset);
      }
    }
    else
    {
      start_block(endpoint_iterator, 0, sessions_.size());
    }
  }

  void start_deferred_session_start(
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t offset)
  {
    BOOST_ASSERT_MSG(!timer_in_progess_, "invalid timer state");

    timer_.expires_from_now(*block_pause_);
    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_,
            boost::bind(&this_type::handle_timer, this,
                _1, endpoint_iterator, offset))));
    timer_in_progess_ = true;
  }

  void handle_timer(const boost::system::error_code& error,
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t offset)
  {
    timer_in_progess_ = false;

    if (stopped_)
    {
      return;
    }

    if (error && error != boost::asio::error::operation_aborted)
    {
      do_stop();
      return;
    }

    offset = start_block(endpoint_iterator, offset, block_size_);
    if (offset < sessions_.size())
    {
      start_deferred_session_start(endpoint_iterator, offset);
    }
  }

  void cancel_timer()
  {
    if (timer_in_progess_)
    {
      boost::system::error_code ignored;
      timer_.cancel(ignored);
    }
  }

  void do_stop()
  {
    if (stopped_)
    {
      return;
    }

    cancel_timer();
    stopped_ = true;
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&session::async_stop, _1));
  }

  void register_stats(const session_ptr& session)
  {
    if (session->was_connected())
    {
      stats_.add(session->bytes_written(), session->bytes_read());
    }
  }

  const std::size_t        block_size_;
  const optional_duration  block_pause_;
  boost::asio::io_service& io_service_;
  boost::asio::io_service::strand strand_;
  deadline_timer timer_;
  session_vector sessions_;
  bool  stopped_;
  bool  timer_in_progess_;
  stats stats_;
  work_state work_state_;
  ma::in_place_handler_allocator<256> start_allocator_;
  ma::in_place_handler_allocator<256> stop_allocator_;
  ma::in_place_handler_allocator<512> timer_allocator_;
}; // class client

optional_duration to_optional_duration(long milliseconds)
{
  optional_duration duration;
  if (milliseconds)
  {
    duration = ma::to_steady_deadline_timer_duration(
        boost::posix_time::milliseconds(milliseconds));
  }
  return duration;
}

} // anonymous namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 10)
    {
      std::cerr << "Usage: asio_performance_test_client <host> <port>"
                << " <threads> <sessions>"
                << " <block_size> <block_pause_milliseconds>"
                << " <buffer_size> <max_connect_attempts> <time_seconds>\n";
      return EXIT_FAILURE;
    }

    const char* const host = argv[1];
    const char* const port = argv[2];
    const std::size_t thread_count  =
        boost::lexical_cast<std::size_t>(argv[3]);
    const std::size_t session_count =
        boost::lexical_cast<std::size_t>(argv[4]);
    const std::size_t block_size =
        boost::lexical_cast<std::size_t>(argv[5]);
    const long block_pause_millis =
      boost::lexical_cast<long>(argv[6]);
    const std::size_t buffer_size   =
        boost::lexical_cast<std::size_t>(argv[7]);
    const std::size_t max_connect_attempts =
        boost::lexical_cast<std::size_t>(argv[8]);
    const long time_seconds =
        boost::lexical_cast<long>(argv[9]);

    std::cout << "Host      : " << host
              << std::endl
              << "Port      : " << port
              << std::endl
              << "Threads   : " << thread_count
              << std::endl
              << "Sessions  : " << session_count
              << std::endl
              << "Block size: " << block_size
              << std::endl
              << "Block pause (milliseconds): " << block_pause_millis
              << std::endl
              << "Size of buffer (bytes)    : " << buffer_size
              << std::endl
              << "Max connect attempts      : " << max_connect_attempts
              << std::endl
              << "Time (seconds)            : " << time_seconds
              << std::endl;

    optional_duration block_pause =
        to_optional_duration(block_pause_millis);

    boost::asio::io_service io_service(thread_count);
    client::protocol::resolver resolver(io_service);
    client c(io_service, session_count, block_size, block_pause,
        buffer_size, max_connect_attempts);
    c.async_start(resolver.resolve(
        client::protocol::resolver::query(host, port)));

    boost::thread_group work_threads;
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      work_threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &io_service));
    }

    c.wait_until_done(boost::posix_time::seconds(time_seconds));
    c.async_stop();

    work_threads.join_all();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
