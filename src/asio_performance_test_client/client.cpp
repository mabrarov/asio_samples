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
#include <list>
#include <string>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/mem_fn.hpp>
#include <boost/scoped_array.hpp>
#include <ma/async_connect.hpp>
#include <ma/handler_allocator.hpp>
#include <ma/custom_alloc_handler.hpp>
#include <ma/strand_wrapped_handler.hpp>

class stats
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

class session
{
public:
  session(boost::asio::io_service& ios, std::size_t block_size, stats& s)
    : strand_(ios)
    , socket_(ios)
    , block_size_(block_size)
    , read_data_(new char[block_size])
    , read_data_length_(0)
    , write_data_(new char[block_size])
    , unwritten_count_(0)
    , bytes_written_(0)
    , bytes_read_(0)
    , stats_(s)
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

  void start(boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
    ma::async_connect(socket_, endpoint,
        MA_STRAND_WRAP(strand_, boost::bind(&session::handle_connect, this,
            boost::asio::placeholders::error, ++endpoint_iterator)));
  }

  void stop()
  {
    strand_.post(boost::bind(&session::close_socket, this));
  }

private:
  void handle_connect(const boost::system::error_code& err,
      boost::asio::ip::tcp::resolver::iterator endpoint_iterator)
  {
    if (!err)
    {
      boost::system::error_code set_option_err;
      boost::asio::ip::tcp::no_delay no_delay(true);
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
    else if (endpoint_iterator != boost::asio::ip::tcp::resolver::iterator())
    {
      socket_.close();
      boost::asio::ip::tcp::endpoint endpoint = *endpoint_iterator;
      ma::async_connect(socket_, endpoint,
          MA_STRAND_WRAP(strand_, boost::bind(&session::handle_connect, this,
              boost::asio::placeholders::error, ++endpoint_iterator)));
    }
  }

  void handle_read(const boost::system::error_code& err, std::size_t length)
  {
    if (!err)
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
    if (!err && length > 0)
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

  void close_socket()
  {
    socket_.close();
  }

private:
  boost::asio::io_service::strand strand_;
  boost::asio::ip::tcp::socket socket_;
  std::size_t block_size_;
  boost::scoped_array<char> read_data_;
  std::size_t read_data_length_;
  boost::scoped_array<char> write_data_;
  int unwritten_count_;
  std::size_t bytes_written_;
  std::size_t bytes_read_;
  stats& stats_;
  ma::in_place_handler_allocator<512> read_allocator_;
  ma::in_place_handler_allocator<512> write_allocator_;
};

class client
{
public:
  client(boost::asio::io_service& ios,
      const boost::asio::ip::tcp::resolver::iterator endpoint_iterator,
      std::size_t block_size, std::size_t session_count, int timeout)
    : io_service_(ios)
    , stop_timer_(ios)
    , sessions_()
    , stats_()
  {
    stop_timer_.expires_from_now(boost::posix_time::seconds(timeout));
    stop_timer_.async_wait(boost::bind(&client::handle_timeout, this));

    for (std::size_t i = 0; i < session_count; ++i)
    {
      session* new_session = new session(io_service_, block_size, stats_);
      new_session->start(endpoint_iterator);
      sessions_.push_back(new_session);
    }
  }

  ~client()
  {
    while (!sessions_.empty())
    {
      delete sessions_.front();
      sessions_.pop_front();
    }

    stats_.print();
  }

  void handle_timeout()
  {
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::mem_fn(&session::stop));
  }

private:
  boost::asio::io_service& io_service_;
  boost::asio::deadline_timer stop_timer_;
  std::list<session*> sessions_;
  stats stats_;
};

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

    boost::asio::ip::tcp::resolver r(ios);
    boost::asio::ip::tcp::resolver::iterator iter =
        r.resolve(boost::asio::ip::tcp::resolver::query(host, port));

    client c(ios, iter, block_size, session_count, timeout);

    std::list<boost::thread*> threads;
    while (--thread_count > 0)
    {
      boost::thread* new_thread = new boost::thread(
          boost::bind(&boost::asio::io_service::run, &ios));
      threads.push_back(new_thread);
    }

    ios.run();

    while (!threads.empty())
    {
      threads.front()->join();
      delete threads.front();
      threads.pop_front();
    }
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}
