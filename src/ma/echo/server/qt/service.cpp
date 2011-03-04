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
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/base_from_member.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/qt/serviceworksignal.h>
#include <ma/echo/server/qt/forwardservicesignal.h>
#include <ma/echo/server/qt/signal_connect_error.h>
#include <ma/echo/server/qt/service.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{    
  execution_config::execution_config(
    std::size_t the_session_manager_thread_count, 
    std::size_t the_session_thread_count)
    : session_manager_thread_count(the_session_manager_thread_count)
    , session_thread_count(the_session_thread_count)    
  {
    if (the_session_manager_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument("the_session_manager_thread_count must be >= 1"));
    }
    if (the_session_thread_count < 1)
    {
      boost::throw_exception(std::invalid_argument("the_session_thread_count must be >= 1"));
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
    explicit io_service_chain(const execution_config& the_execution_config)
      : session_io_service_base(the_execution_config.session_thread_count)
      , session_manager_io_service_(the_execution_config.session_manager_thread_count)
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

  class execution_system : public boost::base_from_member<io_service_chain>
  {   
  private:
    typedef boost::base_from_member<io_service_chain> io_service_chain_base;    
    typedef execution_system this_type;

  public:
    execution_system(const execution_config& the_execution_config)
      : io_service_chain_base(the_execution_config)
      , session_work_(io_service_chain_base::member.session_io_service())
      , session_manager_work_(io_service_chain_base::member.session_manager_io_service())
      , threads_()
      , execution_config_(the_execution_config)
    {
    }

    ~execution_system()
    {
      io_service_chain_base::member.session_manager_io_service().stop();
      io_service_chain_base::member.session_io_service().stop();
      threads_.join_all();
    }

    template <typename Handler>
    void create_threads(const Handler& handler)
    { 
      typedef boost::tuple<Handler> wrapped_handler_type;
      typedef void (*thread_func_type)(boost::asio::io_service&, wrapped_handler_type);
                              
      boost::tuple<Handler> wrapped_handler = boost::make_tuple(handler);
      thread_func_type func = &this_type::thread_func<Handler>;

      for (std::size_t i = 0; i != execution_config_.session_thread_count; ++i)
      {        
        threads_.create_thread(
          boost::bind(func, boost::ref(this->session_io_service()), wrapped_handler));
      }
      for (std::size_t i = 0; i != execution_config_.session_manager_thread_count; ++i)
      {
        threads_.create_thread(
          boost::bind(func, boost::ref(this->session_manager_io_service()), wrapped_handler));
      }      
    }

    boost::asio::io_service& session_manager_io_service()
    {
      return io_service_chain_base::member.session_manager_io_service();
    }

    boost::asio::io_service& session_io_service()
    {
      return io_service_chain_base::member.session_io_service();
    }
    
  private:
    template <typename Handler> 
    static void thread_func(boost::asio::io_service& io_service, boost::tuple<Handler> handler)
    {
      try
      {
        io_service.run();
      }
      catch (...)
      {        
        boost::get<0>(handler)();
      }      
    }

    boost::asio::io_service::work session_work_;
    boost::asio::io_service::work session_manager_work_;
    boost::thread_group threads_;
    execution_config execution_config_;
  }; // class execution_system

} // anonymous namespace

  class Service::Work : public boost::base_from_member<execution_system>
  {   
  private:
    typedef boost::base_from_member<execution_system> execution_system_base;
    typedef Work this_type;

  public:
    Work(const execution_config& the_execution_config, 
      const session_manager_config& the_session_manager_config)
      : execution_system_base(the_execution_config)      
      , workSignal_(boost::make_shared<ServiceWorkSignal>())      
      , sessionManager_(boost::make_shared<session_manager>(
          boost::ref(execution_system_base::member.session_manager_io_service()),
          boost::ref(execution_system_base::member.session_io_service()), 
          the_session_manager_config))
    {
    }

    ~Work()
    {      
    }

    void createThreads()
    {
      execution_system_base::member.create_threads(
        boost::bind(&ServiceWorkSignal::emitWorkThreadExceptionHappened, workSignal_));
    }
    
    ServiceWorkSignalPtr workSignal() const
    {
      return workSignal_;
    }

    session_manager_ptr sessionManager() const
    {
      return sessionManager_;
    }

  private: 
    ServiceWorkSignalPtr workSignal_;
    session_manager_ptr sessionManager_;
  }; // class Service::Work

  Service::Service(QObject* parent)
    : QObject(parent)
    , work_()    
  {
    forwardSignal_ = new ForwardServiceSignal(this);
    checkConnect(QObject::connect(forwardSignal_, 
      SIGNAL(startCompleted(const boost::system::error_code&)), 
      SIGNAL(startCompleted(const boost::system::error_code&)), 
      Qt::QueuedConnection));
    checkConnect(QObject::connect(forwardSignal_, 
      SIGNAL(stopCompleted(const boost::system::error_code&)), 
      SIGNAL(stopCompleted(const boost::system::error_code&)), 
      Qt::QueuedConnection));
  }

  Service::~Service()
  {    
  }

  void Service::asyncStart(const execution_config& the_execution_config, 
    const session_manager_config& the_session_manager_config)
  {
    if (work_.get())
    {      
      forwardSignal_->emitStartCompleted(server_error::invalid_state);
      return;
    }
    work_.reset(new Work(the_execution_config, the_session_manager_config));
    
    ServiceWorkSignalPtr workSignal = work_->workSignal();
    checkConnect(QObject::connect(workSignal.get(), 
      SIGNAL(workThreadExceptionHappened()), 
      SLOT(onWorkThreadExceptionHappened()), 
      Qt::QueuedConnection));
    checkConnect(QObject::connect(workSignal.get(), 
      SIGNAL(sessionManagerStartCompleted(const boost::system::error_code&)), 
      SLOT(onSessionManagerStartCompleted(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(workSignal.get(), 
      SIGNAL(sessionManagerWaitCompleted(const boost::system::error_code&)), 
      SLOT(onSessionManagerWaitCompleted(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(workSignal.get(), 
      SIGNAL(sessionManagerStopCompleted(const boost::system::error_code&)), 
      SLOT(onSessionManagerStopCompleted(const boost::system::error_code&)),
      Qt::QueuedConnection));    

    work_->createThreads();
    work_->sessionManager()->async_start(
      boost::bind(&ServiceWorkSignal::emitSessionManagerStartCompleted, workSignal, _1));
  }

  void Service::onSessionManagerStartCompleted(const boost::system::error_code& error)
  {
    if (!isActualSignalSender(QObject::sender()))
    {
      return;
    }    
    if (error && error != server_error::invalid_state)
    {
      work_.reset();
    }
    else
    {
      work_->sessionManager()->async_wait(
        boost::bind(&ServiceWorkSignal::emitSessionManagerWaitCompleted, work_->workSignal(), _1));
    }
    emit startCompleted(error);
  }
  
  void Service::asyncStop()
  {    
    if (!work_.get())
    {
      forwardSignal_->emitStopCompleted(server_error::invalid_state);
      return;
    }
    work_->sessionManager()->async_stop(
      boost::bind(&ServiceWorkSignal::emitSessionManagerStopCompleted, work_->workSignal(), _1));
  }

  void Service::onSessionManagerStopCompleted(const boost::system::error_code& error)
  {
    if (!isActualSignalSender(QObject::sender()))
    {
      return;
    }
    if (error != server_error::invalid_state)
    {      
      work_.reset();
    }
    emit stopCompleted(error);
  }

  void Service::terminate()
  {
    work_.reset();
  }  

  void Service::onSessionManagerWaitCompleted(const boost::system::error_code& error)
  {
    if (!isActualSignalSender(QObject::sender()))
    {
      return;
    }    
    emit workCompleted(error);
  }  

  void Service::onWorkThreadExceptionHappened()
  {
    if (!isActualSignalSender(QObject::sender()))
    {
      return;
    }
    emit exceptionHappened();
  }

  bool Service::isActualSignalSender(QObject* sender) const
  {
    if (!work_.get())
    {
      return false;
    }
    return work_->workSignal().get() == sender;
  }  

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
