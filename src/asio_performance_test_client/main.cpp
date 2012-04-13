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
#include <boost/mem_fn.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
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

  void wait_for_all_session_stop(const boost::posix_time::time_duration& timeout)
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

  session(boost::asio::io_service& ios, std::size_t block_size, stats& s, 
      work_state& work_state)
    : strand_(ios)
    , socket_(ios)
    , block_size_(block_size)
    , read_data_(new char[block_size])
    , read_data_length_(0)
    , write_data_(new char[block_size])
    , unwritten_count_(0)
    , bytes_written_(0)
    , bytes_read_(0)
    , is_stopped_(false)
    , stats_(s)
    , work_state_(work_state)
  {
    for (std::size_t i = 0; i < block_size_; ++i)
    {
      write_data_[i] = static_cast<char>(i % 128);
    }
  }

  ~session()
  {
    stats_.add(bytes_written_, bytes_read_);
  }

  void start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(connect_allocator_,
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
        ma::make_custom_alloc_handler(connect_allocator_,
            boost::bind(&session::handle_connect, this,
                boost::asio::placeholders::error, endpoint_iterator))));
  }

  void handle_connect(const boost::system::error_code& err,
      protocol::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      boost::system::error_code set_option_err;
      protocol::no_delay no_delay(true);
      socket_.set_option(no_delay, set_option_err);
      if (!set_option_err)
      {
        ++unwritten_count_;

        async_write(socket_, 
            boost::asio::buffer(write_data_.get(), block_size_),
            MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
                write_allocator_, boost::bind(&session::handle_write, this, 
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred))));

        socket_.async_read_some(
            boost::asio::buffer(read_data_.get(), block_size_),
            MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
                read_allocator_, boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred))));
      }
    }
    else 
    {
      boost::system::error_code ignored;
      socket_.close(ignored);
      ++endpoint_iterator;
      if (endpoint_iterator == protocol::resolver::iterator())
      {
        do_stop();
      }
      else
      {
        start_connect(endpoint_iterator);
      }
    }
  }

  void handle_read(const boost::system::error_code& err, std::size_t length)
  {
    if (err)
    {
      do_stop();
    }
    else
    {
      bytes_read_ += length;

      read_data_length_ = length;
      ++unwritten_count_;
      if (unwritten_count_ == 1)
      {
        read_data_.swap(write_data_);

        async_write(socket_, 
            boost::asio::buffer(write_data_.get(), read_data_length_),
            MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
                write_allocator_, boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred))));

        socket_.async_read_some(
            boost::asio::buffer(read_data_.get(), block_size_),
            MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
                read_allocator_, boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred))));
      }
    }
  }

  void handle_write(const boost::system::error_code& err, std::size_t length)
  {
    if (err)
    {
      do_stop();
    }
    else if (length)
    {
      bytes_written_ += length;

      --unwritten_count_;
      if (unwritten_count_ == 1)
      {
        read_data_.swap(write_data_);

        async_write(socket_, 
            boost::asio::buffer(write_data_.get(), read_data_length_),
            MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
                write_allocator_, boost::bind(&session::handle_write, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred))));

        socket_.async_read_some(
            boost::asio::buffer(read_data_.get(), block_size_),
            MA_STRAND_WRAP(strand_, ma::make_custom_alloc_handler(
                read_allocator_, boost::bind(&session::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred))));
      }
    }
  }

  void do_stop()
  {
    if (!is_stopped_)
    {
      boost::system::error_code ignored;
      socket_.close(ignored);
      work_state_.notify_session_stop();    
      is_stopped_ = true;
    }
  }

private:
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  std::size_t block_size_;
  boost::scoped_array<char> read_data_;
  std::size_t read_data_length_;
  boost::scoped_array<char> write_data_;
  int unwritten_count_;
  std::size_t bytes_written_;
  std::size_t bytes_read_;
  bool is_stopped_;
  stats& stats_;
  work_state& work_state_;
  ma::in_place_handler_allocator<512> connect_allocator_;
  ma::in_place_handler_allocator<512> read_allocator_;
  ma::in_place_handler_allocator<512> write_allocator_;
}; // class session

typedef boost::shared_ptr<session> session_ptr;

class client : private boost::noncopyable
{  
public:
  typedef session::protocol protocol;

  client(boost::asio::io_service& ios, 
      const protocol::resolver::iterator& endpoint_iterator,
      std::size_t block_size, std::size_t session_count)
    : io_service_(ios)    
    , sessions_()
    , stats_()
    , work_state_(session_count)    
  {
    for (std::size_t i = 0; i < session_count; ++i)
    {
      session_ptr new_session = boost::make_shared<session>(
          boost::ref(io_service_), block_size, 
          boost::ref(stats_), boost::ref(work_state_));
      new_session->start(endpoint_iterator);
      sessions_.push_back(new_session);
    }
  }

  ~client()
  {
    sessions_.clear();
    stats_.print();
  }

  void stop()
  {
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::mem_fn(&session::stop));
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
      std::cerr << "Usage: client <host> <port> <threads> <blocksize> ";
      std::cerr << "<sessions> <time>\n";
      return 1;
    }

    using namespace std; // For atoi.
    const char* host = argv[1];
    const char* port = argv[2];
    int thread_count = atoi(argv[3]);
    std::size_t block_size = atoi(argv[4]);
    std::size_t session_count = atoi(argv[5]);
    int timeout = atoi(argv[6]);

    boost::asio::io_service ios;

    client::protocol::resolver r(ios);
    client::protocol::resolver::iterator iter =
        r.resolve(client::protocol::resolver::query(host, port));

    client c(ios, iter, block_size, session_count);

    boost::thread_group work_threads;
    for (int i = 0; i < thread_count; ++i)
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
  }
  return EXIT_SUCCESS;
}
