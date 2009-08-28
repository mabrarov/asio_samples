//
// Copyright (c) 2008-2009 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef MA_ECHO_SERVER_SESSION_MANAGER_HPP
#define MA_ECHO_SERVER_SESSION_MANAGER_HPP

#include <stdexcept>
#include <boost/utility.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp>
#include <ma/handler_allocation.hpp>
#include <ma/handler_storage.hpp>
#include <ma/echo/server/session.hpp>

namespace ma
{    
  namespace echo
  {    
    namespace server
    {    
      class session_manager;
      typedef boost::shared_ptr<session_manager> session_manager_ptr;    
      typedef boost::weak_ptr<session_manager>   session_manager_weak_ptr;    

      class session_manager 
        : private boost::noncopyable
        , public boost::enable_shared_from_this<session_manager>
      {
      private:
        typedef session_manager this_type;
        enum state
        {
          ready_to_start,
          start_in_progress,
          started,
          stop_in_progress,
          stopped
        };
        struct session_data;
        typedef boost::shared_ptr<session_data> session_data_ptr;
        typedef boost::weak_ptr<session_data>   session_data_weak_ptr;      
        
        struct session_data : private boost::noncopyable
        {        
          session_data_weak_ptr prev_;
          session_data_ptr next_;
          session_ptr session_;        
          boost::asio::ip::tcp::endpoint endpoint_;        
          std::size_t pending_operations_;
          state state_;        
          in_place_allocator<256> start_wait_allocator_;
          in_place_allocator<256> stop_allocator_;

          explicit session_data(boost::asio::io_service& io_service,
            const session::settings& session_settings)
            : session_(new session(io_service, session_settings))
            , pending_operations_(0)
            , state_(ready_to_start)
          {
          }

          ~session_data()
          {
          }
        }; // session_data

        class session_data_list : private boost::noncopyable
        {
        public:
          explicit session_data_list()
            : size_(0)
          {
          }

          void push_front(const session_data_ptr& value)
          {
            value->next_ = front_;
            value->prev_.reset();
            if (front_)
            {
              front_->prev_ = value;
            }
            front_ = value;
            ++size_;
          }

          void erase(const session_data_ptr& value)
          {
            if (front_ == value)
            {
              front_ = front_->next_;
            }
            session_data_ptr prev = value->prev_.lock();
            if (prev)
            {
              prev->next_ = value->next_;
            }
            if (value->next_)
            {
              value->next_->prev_ = prev;
            }
            value->prev_.reset();
            value->next_.reset();
            --size_;
          }

          std::size_t size() const
          {
            return size_;
          }

          bool empty() const
          {
            return 0 == size_;
          }

          session_data_ptr front() const
          {
            return front_;
          }

        private:
          std::size_t size_;
          session_data_ptr front_;
        }; // session_data_list

      public:
        struct settings
        {      
          boost::asio::ip::tcp::endpoint endpoint_;
          std::size_t max_sessions_;
          std::size_t recycled_sessions_;
          int listen_backlog_;
          session::settings session_settings_;

          explicit settings(const boost::asio::ip::tcp::endpoint& endpoint,
            std::size_t max_sessions,
            std::size_t recycled_sessions,
            int listen_backlog,
            const session::settings& session_settings)
            : endpoint_(endpoint)
            , max_sessions_(max_sessions)
            , recycled_sessions_(recycled_sessions)
            , listen_backlog_(listen_backlog)
            , session_settings_(session_settings)
          {
            if (1 > max_sessions_)
            {
              boost::throw_exception(std::invalid_argument("maximum sessions number must be >= 1"));
            }
          }
        }; // struct settings

        explicit session_manager(boost::asio::io_service& io_service,
          boost::asio::io_service& session_io_service,
          const settings& settings)
          : io_service_(io_service)
          , strand_(io_service)
          , acceptor_(io_service)
          , session_io_service_(session_io_service)                
          , wait_handler_(io_service)
          , stop_handler_(io_service)
          , settings_(settings)
          , pending_operations_(0)
          , state_(ready_to_start)
          , accept_in_progress_(false)
        {          
        }

        ~session_manager()
        {        
        }

        template <typename Handler>
        void async_start(Handler handler)
        {
          strand_.dispatch
          (
            ma::make_context_alloc_handler
            (
              handler, 
              boost::bind
              (
                &this_type::do_start<Handler>,
                shared_from_this(),              
                boost::make_tuple(handler)
              )
            )
          );  
        } // async_start

        template <typename Handler>
        void async_stop(Handler handler)
        {
          strand_.dispatch
          (
            ma::make_context_alloc_handler
            (
              handler, 
              boost::bind
              (
                &this_type::do_stop<Handler>,
                shared_from_this(),
                boost::make_tuple(handler)
              )
            )
          ); 
        } // async_stop

        template <typename Handler>
        void async_wait(Handler handler)
        {
          strand_.dispatch
          (
            ma::make_context_alloc_handler
            (
              handler, 
              boost::bind
              (
                &this_type::do_wait<Handler>,
                shared_from_this(),
                boost::make_tuple(handler)
              )
            )
          );  
        } // async_wait

      private:
        template <typename Handler>
        void do_start(boost::tuple<Handler> handler)
        {
          if (stopped == state_ || stop_in_progress == state_)
          {          
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                boost::asio::error::operation_aborted
              )
            );          
          } 
          else if (ready_to_start != state_)
          {          
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                boost::asio::error::operation_not_supported
              )
            );          
          }
          else
          {
            state_ = start_in_progress;
            boost::system::error_code error;
            acceptor_.open(settings_.endpoint_.protocol(), error);
            if (!error)
            {
              acceptor_.bind(settings_.endpoint_, error);
              if (!error)
              {
                acceptor_.listen(settings_.listen_backlog_, error);
              }          
            }          
            if (error)
            {
              boost::system::error_code ignored;
              acceptor_.close(ignored);
              state_ = stopped;
            }
            else
            {            
              accept_new_session();            
              state_ = started;
            }
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                error
              )
            );                        
          }        
        } // do_start

        template <typename Handler>
        void do_stop(boost::tuple<Handler> handler)
        {
          if (stopped == state_ || stop_in_progress == state_)
          {          
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                boost::asio::error::operation_aborted
              )
            );          
          }
          else
          {
            // Start shutdown
            state_ = stop_in_progress;

            // Do shutdown - abort inner operations          
            acceptor_.close(stop_error_); 

            // Start stop for all active sessions
            session_data_ptr curr_session_data(active_session_datas_.front());
            while (curr_session_data)
            {
              if (stop_in_progress != curr_session_data->state_)
              {
                stop_session(curr_session_data);
              }
              curr_session_data = curr_session_data->next_;
            }
            
            // Do shutdown - abort outer operations
            wait_handler_.cancel();

            // Check for shutdown continuation
            if (may_complete_stop())
            {
              state_ = stopped;          
              // Signal shutdown completion
              io_service_.post
              (
                boost::asio::detail::bind_handler
                (
                  boost::get<0>(handler), 
                  stop_error_
                )
              );
            }
            else
            { 
              stop_handler_.store(
                boost::asio::error::operation_aborted,                        
                boost::get<0>(handler));            
            }
          }
        } // do_stop

        template <typename Handler>
        void do_wait(boost::tuple<Handler> handler)
        {
          if (stopped == state_ || stop_in_progress == state_)
          {          
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                boost::asio::error::operation_aborted
              )
            );          
          } 
          else if (started != state_)
          {          
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                boost::asio::error::operation_not_supported
              )
            );          
          }
          else if (last_accept_error_ && active_session_datas_.empty())
          {
            io_service_.post
            (
              boost::asio::detail::bind_handler
              (
                boost::get<0>(handler), 
                last_accept_error_
              )
            );
          }
          else
          {          
            wait_handler_.store(
              boost::asio::error::operation_aborted,                        
              boost::get<0>(handler));
          }  
        } // do_wait

        void accept_new_session()
        {
          // Get new ready to start session
          session_data_ptr new_session_data;
          if (recycled_session_datas_.empty())
          {
            new_session_data.reset(
              new session_data(session_io_service_, settings_.session_settings_));
          }
          else
          {
            new_session_data = recycled_session_datas_.front();
            recycled_session_datas_.erase(new_session_data);
          }
          // Start session acceptation
          acceptor_.async_accept
          (
            new_session_data->session_->socket(),
            new_session_data->endpoint_, 
            strand_.wrap
            (
              make_custom_alloc_handler
              (
                accept_allocator_,
                boost::bind
                (
                  &this_type::handle_accept,
                  shared_from_this(),
                  new_session_data,
                  _1
                )
              )
            )
          );
          ++pending_operations_;
          accept_in_progress_ = true;
        } // accept_new_session

        void handle_accept(const session_data_ptr& new_session_data,
          const boost::system::error_code& error)
        {
          --pending_operations_;
          accept_in_progress_ = false;
          if (stop_in_progress == state_)
          {
            if (may_complete_stop())  
            {
              state_ = stopped;
              // Signal shutdown completion
              stop_handler_.post(stop_error_);
            }
          }
          else if (error)
          {   
            last_accept_error_ = error;
            if (active_session_datas_.empty()) 
            {
              // Server can't work more time
              wait_handler_.post(error);
            }
          }
          else
          { 
            // Start accepted session 
            start_session(new_session_data);          
            // Save session as active
            active_session_datas_.push_front(new_session_data);
            // Continue session acceptation if can
            if (active_session_datas_.size() < settings_.max_sessions_)
            {
              accept_new_session();
            }          
          }        
        } // handle_accept        

        bool may_complete_stop() const
        {
          return 0 == pending_operations_ && active_session_datas_.empty();
        }      

        void start_session(const session_data_ptr& accepted_session_data)
        {        
          accepted_session_data->session_->async_start
          (
            make_custom_alloc_handler
            (
              accepted_session_data->start_wait_allocator_,
              boost::bind
              (
                &this_type::dispatch_session_start,
                session_manager_weak_ptr(shared_from_this()),
                accepted_session_data,
                _1
              )
            )           
          );  
          ++pending_operations_;
          ++accepted_session_data->pending_operations_;
          accepted_session_data->state_ = start_in_progress;
        } // start_session

        void stop_session(const session_data_ptr& started_session_data)
        {
          started_session_data->session_->async_stop
          (
            make_custom_alloc_handler
            (
              started_session_data->stop_allocator_,
              boost::bind
              (
                &this_type::dispatch_session_stop,
                session_manager_weak_ptr(shared_from_this()),
                started_session_data,
                _1
              )
            )
          );
          ++pending_operations_;
          ++started_session_data->pending_operations_;
          started_session_data->state_ = stop_in_progress;
        } // stop_session

        void wait_session(const session_data_ptr& started_session_data)
        {
          started_session_data->session_->async_wait
          (
            make_custom_alloc_handler
            (
              started_session_data->start_wait_allocator_,
              boost::bind
              (
                &this_type::dispatch_session_wait,
                session_manager_weak_ptr(shared_from_this()),
                started_session_data,
                _1
              )
            )
          );
          ++pending_operations_;
          ++started_session_data->pending_operations_;
        } // wait_session

        static void dispatch_session_start(const session_manager_weak_ptr& weak_session_manager,
          const session_data_ptr& started_session_data, const boost::system::error_code& error)
        {
          session_manager_ptr this_ptr(weak_session_manager.lock());
          if (this_ptr)
          {
            this_ptr->strand_.dispatch
            (
              make_custom_alloc_handler
              (
                started_session_data->start_wait_allocator_,
                boost::bind
                (
                  &this_type::handle_session_start,
                  this_ptr,
                  started_session_data,
                  error
                )
              )
            );
          }
        } // dispatch_session_start

        void handle_session_start(const session_data_ptr& started_session_data,
          const boost::system::error_code& error)
        {
          --pending_operations_;
          --started_session_data->pending_operations_;
          if (start_in_progress == started_session_data->state_)
          {          
            if (error)
            {
              started_session_data->state_ = stopped;
              active_session_datas_.erase(started_session_data);            
              if (stop_in_progress == state_)
              {
                if (may_complete_stop())  
                {
                  state_ = stopped;
                  // Signal shutdown completion
                  stop_handler_.post(stop_error_);
                }
              }
              else if (last_accept_error_ && active_session_datas_.empty()) 
              {
                wait_handler_.post(last_accept_error_);
              }
              else
              {
                recycle_session(started_session_data);
                // Continue session acceptation if can
                if (!accept_in_progress_ && !last_accept_error_
                  && active_session_datas_.size() < settings_.max_sessions_)
                {                
                  accept_new_session();
                }                
              }
            }
            else // !error
            {
              started_session_data->state_ = started;
              if (stop_in_progress == state_)  
              {                            
                stop_session(started_session_data);
              }
              else
              { 
                // Wait until session needs to stop
                wait_session(started_session_data);
              }            
            }  
          } 
          else if (stop_in_progress == state_) 
          {
            if (may_complete_stop())  
            {
              state_ = stopped;
              // Signal shutdown completion
              stop_handler_.post(stop_error_);
            }
          }
          else
          {
            recycle_session(started_session_data);
          }        
        } // handle_session_start

        static void dispatch_session_wait(const session_manager_weak_ptr& weak_session_manager,
          const session_data_ptr& waited_session_data, const boost::system::error_code& error)
        {
          session_manager_ptr this_ptr(weak_session_manager.lock());
          if (this_ptr)
          {
            this_ptr->strand_.dispatch
            (
              make_custom_alloc_handler
              (
                waited_session_data->start_wait_allocator_,
                boost::bind
                (
                  &this_type::handle_session_wait,
                  this_ptr,
                  waited_session_data,
                  error
                )
              )
            );
          }
        } // dispatch_session_wait

        void handle_session_wait(const session_data_ptr& waited_session_data,
          const boost::system::error_code& /*error*/)
        {
          --pending_operations_;
          --waited_session_data->pending_operations_;
          if (started == waited_session_data->state_)
          {
            stop_session(waited_session_data);
          }
          else if (stop_in_progress == state_)
          {
            if (may_complete_stop())  
            {
              state_ = stopped;
              // Signal shutdown completion
              stop_handler_.post(stop_error_);
            }
          }
          else
          {
            recycle_session(waited_session_data);
          }
        } // handle_session_wait

        static void dispatch_session_stop(const session_manager_weak_ptr& weak_session_manager,
          const session_data_ptr& stopped_session_data, const boost::system::error_code& error)
        {
          session_manager_ptr this_ptr(weak_session_manager.lock());
          if (this_ptr)
          {
            this_ptr->strand_.dispatch
            (
              make_custom_alloc_handler
              (
                stopped_session_data->stop_allocator_,
                boost::bind
                (
                  &this_type::handle_session_stop,
                  this_ptr,
                  stopped_session_data,
                  error
                )
              )
            );
          }
        } // dispatch_session_stop

        void handle_session_stop(const session_data_ptr& stopped_session_data,
          const boost::system::error_code& /*error*/)
        {
          --pending_operations_;
          --stopped_session_data->pending_operations_;
          if (stop_in_progress == stopped_session_data->state_)        
          {
            stopped_session_data->state_ = stopped;
            active_session_datas_.erase(stopped_session_data);            
            if (stop_in_progress == state_)
            {
              if (may_complete_stop())  
              {
                state_ = stopped;
                // Signal shutdown completion
                stop_handler_.post(stop_error_);
              }
            }
            else if (last_accept_error_ && active_session_datas_.empty()) 
            {
              wait_handler_.post(last_accept_error_);
            }
            else
            {
              recycle_session(stopped_session_data);
              // Continue session acceptation if can
              if (!accept_in_progress_ && !last_accept_error_
                && active_session_datas_.size() < settings_.max_sessions_)
              {                
                accept_new_session();
              }              
            }
          }
          else if (stop_in_progress == state_)
          {
            if (may_complete_stop())  
            {
              state_ = stopped;
              // Signal shutdown completion
              stop_handler_.post(stop_error_);
            }
          }
          else
          {
            recycle_session(stopped_session_data);
          }
        } // handle_session_stop

        void recycle_session(const session_data_ptr& recycled_session_data)
        {
          if (0 == recycled_session_data->pending_operations_
            && recycled_session_datas_.size() < settings_.recycled_sessions_)
          {
            recycled_session_data->session_->reset();
            recycled_session_data->state_ = ready_to_start;
            recycled_session_datas_.push_front(recycled_session_data);
          }        
        } // recycle_session

        boost::asio::io_service& io_service_;
        boost::asio::io_service::strand strand_;      
        boost::asio::ip::tcp::acceptor acceptor_;
        boost::asio::io_service& session_io_service_;            
        ma::handler_storage<boost::system::error_code> wait_handler_;
        ma::handler_storage<boost::system::error_code> stop_handler_;
        session_data_list active_session_datas_;
        session_data_list recycled_session_datas_;
        boost::system::error_code last_accept_error_;
        boost::system::error_code stop_error_;      
        settings settings_;
        std::size_t pending_operations_;
        state state_;
        bool accept_in_progress_;
        in_place_allocator<512> accept_allocator_;
      }; // class session_manager

    } // namespace server
  } // namespace echo
} // namespace ma

#endif // MA_ECHO_SERVER_SESSION_MANAGER_HPP
