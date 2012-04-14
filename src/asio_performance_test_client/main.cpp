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
#include <string>
#include <vector>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <ma/cyclic_buffer.hpp>
#include <ma/async_connect.hpp>
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
    : mutex_()
    , total_bytes_written_(0)
    , total_bytes_read_(0)
  {
  }

  void add(std::size_t bytes_written, std::size_t bytes_read)
  {
    boost::mutex::scoped_lock lock(mutex_);
    total_bytes_written_ += bytes_written;
    total_bytes_read_ += bytes_read;
  }

  void print()
  {
    boost::mutex::scoped_lock lock(mutex_);
    std::cout << total_bytes_written_ << " total bytes written\n";
    std::cout << total_bytes_read_ << " total bytes read\n";
  }

private:
  boost::mutex mutex_;
  std::size_t total_bytes_written_;
  std::size_t total_bytes_read_;
}; // class stats

class session : private boost::noncopyable
{
  typedef session this_type;

public:
  typedef boost::asio::ip::tcp protocol;

  session(boost::asio::io_service& ios, std::size_t buffer_size,
      stats& s, work_state& work_state)
    : strand_(ios)
    , socket_(ios)
    , buffer_(buffer_size)
    , bytes_written_(0)
    , bytes_read_(0)
    , write_in_progress_(false)
    , read_in_progress_(false)
    , stopped_(false)
    , stats_(s)
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

  ~session()
  {
    stats_.add(bytes_written_, bytes_read_);
  }

  void start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(write_allocator_,
        boost::bind(&this_type::start_connect, this, endpoint_iterator)));
  }

  void stop()
  {
    strand_.post(boost::bind(&session::do_stop, this));
  }

private:
  void start_connect(const protocol::resolver::iterator& endpoint_iterator)
  {
    protocol::endpoint endpoint = *endpoint_iterator;
    ma::async_connect(socket_, endpoint, MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(write_allocator_,
            boost::bind(&session::handle_connect, this,
                boost::asio::placeholders::error, endpoint_iterator))));
  }

  void handle_connect(const boost::system::error_code& error,
      protocol::resolver::iterator endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    if (error)
    {
      ++endpoint_iterator;
      if (protocol::resolver::iterator() != endpoint_iterator)
      {
        boost::system::error_code ignored;
        socket_.close(ignored);
        start_connect(endpoint_iterator);
        return;
      }
      do_stop();
      return;
    }

    boost::system::error_code set_option_error;
    protocol::no_delay no_delay(true);
    socket_.set_option(no_delay, set_option_error);
    if (set_option_error)
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
    if (!stopped_)
    {
      boost::system::error_code ignored;
      socket_.close(ignored);
      work_state_.notify_session_stop();
      stopped_ = true;
    }
  }

  void start_write_some()
  {
    ma::cyclic_buffer::const_buffers_type write_data = buffer_.data();
    if (!write_data.empty())
    {
      socket_.async_write_some(write_data, MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(write_allocator_,
              boost::bind(&session::handle_write, this,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred))));
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
              boost::bind(&session::handle_read, this,
                  boost::asio::placeholders::error,
                  boost::asio::placeholders::bytes_transferred))));
      read_in_progress_ = true;
    }
  }

  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  ma::cyclic_buffer buffer_;
  std::size_t bytes_written_;
  std::size_t bytes_read_;
  bool write_in_progress_;
  bool read_in_progress_;
  bool stopped_;
  stats& stats_;
  work_state& work_state_;
  ma::in_place_handler_allocator<512> read_allocator_;
  ma::in_place_handler_allocator<512> write_allocator_;
}; // class session

typedef boost::shared_ptr<session> session_ptr;

class client : private boost::noncopyable
{
public:
  typedef session::protocol protocol;

  client(boost::asio::io_service& ios, std::size_t buffer_size, 
      std::size_t session_count)
    : io_service_(ios)
    , sessions_()
    , stats_()
    , work_state_(session_count)
  {
    for (std::size_t i = 0; i < session_count; ++i)
    {
      sessions_.push_back(boost::make_shared<session>(boost::ref(io_service_),
          buffer_size, boost::ref(stats_), boost::ref(work_state_)));
    }
  }

  ~client()
  {
    sessions_.clear();
    stats_.print();
  }

  void start(const protocol::resolver::iterator& endpoint_iterator)
  {
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&session::start, _1, endpoint_iterator));
  }

  void stop()
  {
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&session::stop, _1));
  }

  void wait_until_done(const boost::posix_time::time_duration& timeout)
  {
    work_state_.wait_for_all_session_stop(timeout);
  }

private:
  boost::asio::io_service& io_service_;
  std::vector<session_ptr> sessions_;
  stats stats_;
  work_state work_state_;
}; // class client

} // anonymous namespace

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 7)
    {
      std::cerr << "Usage: client <host> <port> <threads> <buffersize> ";
      std::cerr << "<sessions> <time>\n";
      return EXIT_FAILURE;
    }

    using namespace std; // For atoi.
    const char* host = argv[1];
    const char* port = argv[2];
    int thread_count = atoi(argv[3]);
    std::size_t buffer_size = atoi(argv[4]);
    std::size_t session_count = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    boost::asio::io_service ios;
    client::protocol::resolver r(ios);
    client c(ios, buffer_size, session_count);
    c.start(r.resolve(client::protocol::resolver::query(host, port)));

    boost::thread_group work_threads;
    for (int i = 0; i != thread_count; ++i)
    {
      work_threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &ios));
    }

    c.wait_until_done(boost::posix_time::seconds(timeout));
    c.stop();

    work_threads.join_all();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
