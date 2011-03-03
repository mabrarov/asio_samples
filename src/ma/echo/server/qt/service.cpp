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
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/throw_exception.hpp>
#include <boost/utility/base_from_member.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/qt/workthreadsignal.h>
#include <ma/echo/server/qt/sessionmanagersignal.h>
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
      , workThreadSignal_(boost::make_shared<WorkThreadSignal>())
      , startSessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
      , waitSessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
      , stopSessionManagerSignal_(boost::make_shared<SessionManagerSignal>())
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
        boost::bind(&WorkThreadSignal::emitWorkException, workThreadSignal_));
    }
    
    WorkThreadSignalPtr workThreadSignal() const
    {
      return workThreadSignal_;
    }

    SessionManagerSignalPtr startSessionManagerSignal() const
    {
      return startSessionManagerSignal_;
    }

    SessionManagerSignalPtr waitSessionManagerSignal() const
    {
      return waitSessionManagerSignal_;
    }

    SessionManagerSignalPtr stopSessionManagerSignal() const
    {
      return stopSessionManagerSignal_;
    }

    session_manager_ptr sessionManager() const
    {
      return sessionManager_;
    }

  private: 
    WorkThreadSignalPtr workThreadSignal_;
    SessionManagerSignalPtr startSessionManagerSignal_;
    SessionManagerSignalPtr waitSessionManagerSignal_;
    SessionManagerSignalPtr stopSessionManagerSignal_;    
    session_manager_ptr sessionManager_;    
  }; // class Service::Work

  Service::Service(QObject* parent)
    : QObject(parent)
    , work_()
  {
  }

  Service::~Service()
  {    
  }

  void Service::asyncStart(const execution_config& the_execution_config, 
    const session_manager_config& the_session_manager_config)
  {
    if (work_.get())
    {      
      emit startComplete(boost::system::error_code(server_error::invalid_state));
      return;
    }
    work_.reset(new Work(the_execution_config, the_session_manager_config));
    
    checkConnect(QObject::connect(work_->workThreadSignal().get(), 
      SIGNAL(workException()), SLOT(workThreadException()), 
      Qt::QueuedConnection));
    checkConnect(QObject::connect(work_->startSessionManagerSignal().get() , 
      SIGNAL(operationComplete(const boost::system::error_code&)), 
      SLOT(sessionManagerStartComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(work_->waitSessionManagerSignal().get(), 
      SIGNAL(operationComplete(const boost::system::error_code&)), 
      SLOT(sessionManagerWaitComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(work_->stopSessionManagerSignal().get(), 
      SIGNAL(operationComplete(const boost::system::error_code&)), 
      SLOT(sessionManagerStopComplete(const boost::system::error_code&)),
      Qt::QueuedConnection));    

    work_->createThreads();
    work_->sessionManager()->async_start(boost::bind(
      &SessionManagerSignal::emitOperationComplete, work_->startSessionManagerSignal(), _1));
  }

  void Service::sessionManagerStartComplete(const boost::system::error_code& error)
  {
    if (!sessionManagerStartSignalActual(QObject::sender()))
    {
      return;
    }    
    if (error && error != server_error::invalid_state)
    {
      work_.reset();      
    }
    else
    {
      work_->sessionManager()->async_wait(boost::bind(
        &SessionManagerSignal::emitOperationComplete, work_->waitSessionManagerSignal(), _1));
    }
    emit startComplete(error);
  }
  
  void Service::asyncStop()
  {    
    if (!work_.get())
    {      
      emit stopComplete(boost::system::error_code(server_error::invalid_state));
      return;
    }    
    work_->sessionManager()->async_wait(boost::bind(
      &SessionManagerSignal::emitOperationComplete, work_->stopSessionManagerSignal(), _1));
  }

  void Service::sessionManagerStopComplete(const boost::system::error_code& error)
  {
    if (!sessionManagerStopSignalActual(QObject::sender()))
    {
      return;
    }
    if (error != server_error::invalid_state)
    {
      work_.reset();
    }
    emit stopComplete(error);
  }

  void Service::terminateWork()
  {
    work_.reset();
  }  

  void Service::sessionManagerWaitComplete(const boost::system::error_code& error)
  {
    if (!sessionManagerWaitSignalActual(QObject::sender()))
    {
      return;
    }    
    emit workComplete(error);
  }  

  void Service::workThreadException()
  {
    if (!workThreadSignalActual(QObject::sender()))
    {
      return;
    }
    emit workException();
  }

  bool Service::sessionManagerStartSignalActual(QObject* sender) const
  {
    if (!work_.get())
    {
      return false;
    }
    return work_->startSessionManagerSignal().get() == sender;
  }

  bool Service::sessionManagerWaitSignalActual(QObject* sender) const
  {
    if (!work_.get())
    {
      return false;
    }
    return work_->waitSessionManagerSignal().get() == sender;
  }

  bool Service::sessionManagerStopSignalActual(QObject* sender) const
  {
    if (!work_.get())
    {
      return false;
    }
    return work_->stopSessionManagerSignal().get() == sender;
  }

  bool Service::workThreadSignalActual(QObject* sender) const
  {
    if (!work_.get())
    {
      return false;
    }
    return work_->workThreadSignal().get() == sender;
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
