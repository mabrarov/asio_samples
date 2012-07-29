//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
  {
  }

  void add(std::size_t connect_count)
  {
    total_sessions_connected_ += connect_count;
  }

  void print()
  {
    std::cout << total_sessions_connected_ << " total sessions connected\n";    
  }

private:  
  boost::uint_fast64_t total_sessions_connected_;  
}; // class stats

typedef ma::steady_deadline_timer      deadline_timer;
typedef deadline_timer::duration_type  duration_type;
typedef boost::optional<duration_type> optional_duration;

class session : private boost::noncopyable
{
  typedef session this_type;  

public:
  typedef boost::asio::ip::tcp protocol;

  session(boost::asio::io_service& io_service, 
      const optional_duration& connect_pause, work_state& work_state)
    : connect_pause_(connect_pause)
    , strand_(io_service)
    , socket_(io_service)
    , timer_(io_service)
    , connect_count_(0)    
    , stopped_(false)
    , work_state_(work_state)
  {    
  }

  void start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(connect_allocator_,
        boost::bind(&this_type::start_connect, this, 0, 
            endpoint_iterator, endpoint_iterator)));
  }

  void stop()
  {
    strand_.post(boost::bind(&this_type::do_stop, this));
  }

  std::size_t connect_count() const
  {
    return connect_count_;
  }

private:
  void start_connect(std::size_t connect_attempt,
      const protocol::resolver::iterator& initial_endpoint_iterator,
      const protocol::resolver::iterator& current_endpoint_iterator)
  {
    protocol::endpoint endpoint = *current_endpoint_iterator;
    ma::async_connect(socket_, endpoint, MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(connect_allocator_,
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
      } 
      else 
      {
        start_connect(connect_attempt, 
            initial_endpoint_iterator, initial_endpoint_iterator);
      }
      return;      
    }

    ++connect_count_;    
    if (boost::system::error_code error = apply_socket_options())
    {
      do_stop();
      return;
    }
    
    if (connect_pause_)
    {
      timer_.expires_from_now(*connect_pause_);
      timer_.async_wait(MA_STRAND_WRAP(strand_,
          ma::make_custom_alloc_handler(timer_allocator_,
              boost::bind(&this_type::handle_timer, this, 
                  _1, initial_endpoint_iterator))));
    }
    else
    {
      continue_work(initial_endpoint_iterator);
    }
  }  

  void handle_timer(const boost::system::error_code& error, 
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    if (error && error != boost::asio::error::operation_aborted)
    {
      do_stop();
      return;
    }

    continue_work(initial_endpoint_iterator);
  }

  void continue_work(const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    shutdown_socket();
    close_socket();
    start_connect(0, initial_endpoint_iterator, initial_endpoint_iterator);
  }

  boost::system::error_code apply_socket_options()
  {
    typedef protocol::socket socket_type;

    // Setup abortive shutdown sequence for closesocket
    {
      boost::system::error_code error;
      socket_type::linger opt(false, 0);
      socket_.set_option(opt, error);
      if (error)
      {
        return error;
      }
    }

    return boost::system::error_code();
  }

  boost::system::error_code shutdown_socket()
  {
    boost::system::error_code error;
    socket_.shutdown(protocol::socket::shutdown_send, error);
    return error;
  }

  void close_socket()
  {
    boost::system::error_code ignored;
    socket_.close(ignored);
  }

  void do_stop()
  {
    if (!stopped_)
    {
      close_socket();
      stopped_ = true;
      work_state_.notify_session_stop();      
    }
  }  

  const optional_duration connect_pause_;
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  deadline_timer   timer_;
  std::size_t      connect_count_;
  bool             stopped_;
  work_state&      work_state_;
  ma::in_place_handler_allocator<512> connect_allocator_;
  ma::in_place_handler_allocator<256> timer_allocator_;
}; // class session

typedef boost::shared_ptr<session> session_ptr;

class client : private boost::noncopyable
{
private:
  typedef client this_type;

public:
  typedef session::protocol protocol;

  client(boost::asio::io_service& io_service, std::size_t session_count, 
      const optional_duration& connect_pause)
    : io_service_(io_service)
    , sessions_()
    , stats_()
    , work_state_(session_count)
  {
    for (std::size_t i = 0; i < session_count; ++i)
    {
      sessions_.push_back(boost::make_shared<session>(
          boost::ref(io_service_), connect_pause, boost::ref(work_state_)));
    }
  }

  ~client()
  {
    std::for_each(sessions_.begin(), sessions_.end(),
        boost::bind(&this_type::register_stats, this, _1));
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
  void register_stats(const session_ptr& session)
  {
    stats_.add(session->connect_count());
  }

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
      std::cerr << "Usage: async_connect <host> <port> <threads> <sessions>"
                << " <connect_pause_milliseconds> <time_seconds>\n";
      return EXIT_FAILURE;
    }
    
    const char* const host = argv[1];
    const char* const port = argv[2];
    const std::size_t thread_count  = 
        boost::lexical_cast<std::size_t>(argv[3]);    
    const std::size_t session_count = 
        boost::lexical_cast<std::size_t>(argv[4]);
    const long connect_pause_millis = 
        boost::lexical_cast<long>(argv[5]);
    const long timeout = 
        boost::lexical_cast<long>(argv[6]);

    optional_duration connect_pause;
    if (connect_pause_millis)
    {
      connect_pause = ma::to_steady_deadline_timer_duration(
          boost::posix_time::milliseconds(connect_pause_millis));
    }

    boost::asio::io_service io_service(thread_count);
    client::protocol::resolver resolver(io_service);
    client c(io_service, session_count, connect_pause);
    c.start(resolver.resolve(client::protocol::resolver::query(host, port)));

    boost::thread_group work_threads;
    for (std::size_t i = 0; i != thread_count; ++i)
    {
      work_threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &io_service));
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
