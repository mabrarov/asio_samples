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
#include <ma/echo/server/qt/workthreadsignal.h>
#include <ma/echo/server/qt/signal_connect_error.h>
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
    void create_work_threads(const Handler& handler)
    { 
      typedef boost::tuple<Handler> wrapped_handler_type;
      typedef void (*thread_func_type)(boost::asio::io_service&, wrapped_handler_type);
                              
      boost::tuple<Handler> wrapped_handler = boost::make_tuple(handler);
      thread_func_type thread_func = &this_type::work_thread_func<Handler>;

      for (std::size_t i = 0; i != execution_config_.session_thread_count; ++i)
      {        
        threads_.create_thread(
          boost::bind(thread_func, boost::ref(this->session_io_service()), wrapped_handler));
      }
      for (std::size_t i = 0; i != execution_config_.session_manager_thread_count; ++i)
      {
        threads_.create_thread(
          boost::bind(thread_func, boost::ref(this->session_manager_io_service()), wrapped_handler));
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
    static void work_thread_func(boost::asio::io_service& io_service, boost::tuple<Handler> handler)
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

  class Service::Worker : public boost::base_from_member<execution_system>
  {   
  private:
    typedef boost::base_from_member<execution_system> execution_system_base;
    typedef Worker this_type;

  public:
    Worker(const execution_config& the_execution_config, 
      const session_manager_config& the_session_manager_config)
      : execution_system_base(the_execution_config)
      , sessionManager_(execution_system_base::member.session_manager_io_service(),
          execution_system_base::member.session_io_service(), the_session_manager_config)
      , workThreadSignal_(boost::make_shared<WorkThreadSignal>())
    {
    }

    void createWorkThreads()
    {
      execution_system_base::member.create_work_threads(
        boost::bind(&WorkThreadSignal::emitWorkException, workThreadSignal_));
    }

    SessionManagerWrapper& sessionManager()
    {
      return sessionManager_;
    }

    WorkThreadSignalPtr workThreadSignal() const
    {
      return workThreadSignal_;
    }

    ~Worker()
    {      
    }

  private: 
    SessionManagerWrapper sessionManager_;
    WorkThreadSignalPtr workThreadSignal_;
  }; // class Service::Worker

  Service::Service(QObject* parent)
    : QObject(parent)
    , worker_()
  {
  }

  Service::~Service()
  {    
  }

  void Service::asyncStart(const execution_config& the_execution_config, 
    const session_manager_config& the_session_manager_config)
  {
    worker_.reset(new Worker(the_execution_config, the_session_manager_config));

    SessionManagerWrapper& sessionManager = worker_->sessionManager();
    checkConnect(QObject::connect(&sessionManager, 
      SIGNAL(startComplete(const boost::system::error_code&)), 
      SLOT(sessionManagerStartComplete(const boost::system::error_code&))));
    checkConnect(QObject::connect(&sessionManager, 
      SIGNAL(waitComplete(const boost::system::error_code&)), 
      SLOT(sessionManagerWaitComplete(const boost::system::error_code&))));
    checkConnect(QObject::connect(&sessionManager, 
      SIGNAL(stopComplete(const boost::system::error_code&)), 
      SLOT(sessionManagerStopComplete(const boost::system::error_code&))));

    checkConnect(QObject::connect(worker_->workThreadSignal().get(), 
      SIGNAL(workException()), SLOT(workThreadExceptionHappen()), 
      Qt::QueuedConnection));

    worker_->createWorkThreads();
    sessionManager.asyncStart();

    //todo: update state
  }

  void Service::sessionManagerStartComplete(const boost::system::error_code& error)
  {
    if (!isSessionManagerSignalActual(QObject::sender()))
    {
      return;
    }
    //todo: check the state
    if (error)
    {
      worker_.reset();
      //todo: update state
    }
    else
    {
      worker_->sessionManager().asyncWait();
      //todo: update state      
    }    
    emit startComplete(error);
  }
  
  void Service::asyncStop()
  {
    boost::system::error_code error;
    if (!worker_.get())
    {
      error = ma::echo::server::server_error::invalid_state;
      emit stopComplete(error);
      return;
    }
    //todo: check the state
    worker_->sessionManager().asyncStop();
    //todo: update state
  }

  void Service::sessionManagerStopComplete(const boost::system::error_code& error)
  {
    if (!isSessionManagerSignalActual(QObject::sender()))
    {
      return;
    }
    //todo: update state
    emit stopComplete(error);
  }

  void Service::terminate()
  {
    worker_.reset();
    //todo: update the state
  }  

  void Service::sessionManagerWaitComplete(const boost::system::error_code& error)
  {
    if (!isSessionManagerSignalActual(QObject::sender()))
    {
      return;
    }    
    emit workComplete(error);
  }

  

  void Service::workThreadExceptionHappen()
  {
    if (!isWorkThreadSignalActual(QObject::sender()))
    {
      return;
    }
    emit workThreadException();
  }

  bool Service::isSessionManagerSignalActual(QObject* sender)
  {
    if (!worker_.get())
    {
      return false;
    }
    SessionManagerWrapper& sessionManager = worker_->sessionManager();
    return static_cast<QObject*>(&sessionManager) == sender;
  }

  bool Service::isWorkThreadSignalActual(QObject* sender)
  {
    if (!worker_.get())
    {
      return false;
    }
    return static_cast<QObject*>(worker_->workThreadSignal().get()) == sender;
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
