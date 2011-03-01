/*
TRANSLATOR ma::echo::server::qt::Service
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <stdexcept>
#include <boost/asio.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/thread.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/base_from_member.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/qt/sessionmanagerwrapper.h>
#include <ma/echo/server/qt/service.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{    
  execution_config::execution_config(std::size_t the_session_manager_thread_count, 
    std::size_t the_session_thread_count, long the_stop_sec_timeout)
    : session_manager_thread_count(the_session_manager_thread_count)
    , session_thread_count(the_session_thread_count)
    , stop_sec_timeout(the_stop_sec_timeout)
  {
    if (the_session_manager_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument("the_session_manager_thread_count must be >= 1"));
    }
    if (the_session_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument("the_session_thread_count must be >= 1"));
    }
    if (the_stop_sec_timeout < 0)
    {
      boost::throw_exception(std::invalid_argument("the_stop_sec_timeout must be >= 0"));
    }
  }

namespace 
{
  class io_service_chain
    : private boost::noncopyable
    , public boost::base_from_member<boost::asio::io_service>
  {
  private:
    typedef boost::base_from_member<boost::asio::io_service> session_io_service_base;

  public:
    explicit io_service_chain(const execution_config& /*the_execution_config*/)
      : session_io_service_base()
      , session_manager_io_service_()
    {        
    }

    boost::asio::io_service& session_manager_io_service()
    {
      return session_manager_io_service_;
    }

    boost::asio::io_service& session_io_service()
    {
      return session_io_service_base::member;
    }

  private:
    boost::asio::io_service session_manager_io_service_;
  }; // class io_service_chain

  class execution_system
    : public boost::base_from_member<io_service_chain>
  {   
  private:
    typedef boost::base_from_member<io_service_chain> io_service_chain_base;    

  public:
    execution_system(const execution_config& the_execution_config)
      : io_service_chain_base(the_execution_config)
      , session_work_(io_service_chain_base::member.session_io_service())
      , session_manager_work_(io_service_chain_base::member.session_manager_io_service())
      , threads_()
    {
    }

    ~execution_system()
    {
      io_service_chain_base::member.session_manager_io_service().stop();
      io_service_chain_base::member.session_io_service().stop();
      threads_.join_all();
    }

    boost::asio::io_service& session_manager_io_service()
    {
      return io_service_chain_base::member.session_manager_io_service();
    }

    boost::asio::io_service& session_io_service()
    {
      return io_service_chain_base::member.session_io_service();
    }

    boost::thread_group& threads()
    {
      return threads_;
    }

  private:
    boost::asio::io_service::work session_work_;
    boost::asio::io_service::work session_manager_work_;
    boost::thread_group threads_; 
  }; // class execution_system

} // anonymous namespace

  class Service::implementation 
    : public boost::base_from_member<execution_system>
  {   
  private:
    typedef boost::base_from_member<execution_system> execution_system_base;
    typedef implementation this_type;

  public:
    implementation(const execution_config& the_execution_config, 
      const session_manager_config& /*the_session_manager_config*/)
      : execution_system_base(the_execution_config)      
    {
    }

    ~implementation()
    {      
    }

  private:     
  }; // class Service::implementation

  Service::Service(QObject* parent)
    : QObject(parent)
    , impl_()
  {
  } // Service::Service

  Service::~Service()
  {
    
  } // Service::~Service

  void Service::asyncStart(const execution_config&, const session_manager_config&)
  {
    boost::system::error_code error;
    if (impl_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit startComplete(error);
      return;
    }
    //todo: continue implementation of method
    //temporary
    {
      error = ma::echo::server::server_error::invalid_state;
      emit startComplete(error);
    }    
  } // Service::asyncStart

  void Service::asyncWait()
  {
    boost::system::error_code error;
    if (!impl_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit waitComplete(error);
      return;
    }
    //todo: continue implementation of method
    //temporary
    {
      error = ma::echo::server::server_error::invalid_state;
      emit waitComplete(error);
    }    
  } // Service::asyncWait
  
  void Service::asyncStop()
  {
    boost::system::error_code error;
    if (!impl_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit stopComplete(error);
      return;
    }
    //todo: continue implementation of method
    //temporary
    {
      error = ma::echo::server::server_error::invalid_state;
      emit stopComplete(error);
    }    
  } // Service::asyncStop

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
