//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <algorithm>
#include <iostream>
#include <vector>
#include <boost/assert.hpp>
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
    std::cout << "Total sessions connected: " 
              << total_sessions_connected_ << std::endl;
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
    , timer_in_progess_(false)
    , work_state_(work_state)
  {    
  }

  void async_start(const protocol::resolver::iterator& endpoint_iterator)
  {
    strand_.post(ma::make_custom_alloc_handler(connect_allocator_,
        boost::bind(&this_type::do_start, this, endpoint_iterator)));
  }

  void async_stop()
  {
    strand_.post(ma::make_custom_alloc_handler(stop_allocator_,
        boost::bind(&this_type::do_stop, this)));
  }

  std::size_t connect_count() const
  {
    return connect_count_;
  }

private:
  void do_start(const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    if (stopped_)
    {
      return;
    }

    start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
  }

  void start_connect(
      const protocol::resolver::iterator& initial_endpoint_iterator,
      const protocol::resolver::iterator& current_endpoint_iterator)
  {
    protocol::endpoint endpoint = *current_endpoint_iterator;
    ma::async_connect(socket_, endpoint, MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(connect_allocator_,
            boost::bind(&this_type::handle_connect, this, _1, 
                initial_endpoint_iterator, current_endpoint_iterator))));
  }

  void handle_connect(const boost::system::error_code& error, 
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
        start_connect(initial_endpoint_iterator, current_endpoint_iterator);
      } 
      else 
      {
        start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
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
      start_deferred_continue_work(initial_endpoint_iterator);
    }
    else
    {
      continue_work(initial_endpoint_iterator);
    }
  }

  void start_deferred_continue_work(
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    BOOST_ASSERT_MSG(!timer_in_progess_, "invalid timer state");

    timer_.expires_from_now(*connect_pause_);
    timer_.async_wait(MA_STRAND_WRAP(strand_,
        ma::make_custom_alloc_handler(timer_allocator_,
            boost::bind(&this_type::handle_timer, this, 
                _1, initial_endpoint_iterator))));
    timer_in_progess_ = true;
  }

  void handle_timer(const boost::system::error_code& error, 
      const protocol::resolver::iterator& initial_endpoint_iterator)
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

    continue_work(initial_endpoint_iterator);
  }

  void continue_work(
      const protocol::resolver::iterator& initial_endpoint_iterator)
  {
    shutdown_socket();
    close_socket();
    start_connect(initial_endpoint_iterator, initial_endpoint_iterator);
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
    
    close_socket();
    cancel_timer();
    stopped_ = true;
    work_state_.notify_session_stop();
  }  

  const optional_duration connect_pause_;
  boost::asio::io_service::strand strand_;
  protocol::socket socket_;
  deadline_timer   timer_;
  std::size_t      connect_count_;
  bool             stopped_;
  bool             timer_in_progess_;
  work_state&      work_state_;
  ma::in_place_handler_allocator<256> stop_allocator_;
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
      std::size_t block_size, const optional_duration& block_pause,
      const optional_duration& connect_pause)
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
    stats_.add(session->connect_count());
  }

  const std::size_t       block_size_;
  const optional_duration block_pause_;
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
    if (argc != 9)
    {
      std::cerr << "Usage: async_connect <host> <port> <threads>"
                << " <sessions> <block_size> <block_pause_milliseconds>"
                << " <connect_pause_milliseconds> <time_seconds>\n";
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
    const long connect_pause_millis = 
        boost::lexical_cast<long>(argv[7]);
    const long time_seconds = 
        boost::lexical_cast<long>(argv[8]);

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
              << "Block pause (milliseconds)  : " << block_pause_millis 
              << std::endl
              << "Connect pause (milliseconds): " << connect_pause_millis
              << std::endl
              << "Time (seconds)              : " << time_seconds
              << std::endl;

    optional_duration block_pause = 
        to_optional_duration(block_pause_millis);
    optional_duration connect_pause = 
        to_optional_duration(connect_pause_millis);

    boost::asio::io_service io_service(thread_count);
    client::protocol::resolver resolver(io_service);
    client c(io_service, 
        session_count, block_size, block_pause, connect_pause);
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
