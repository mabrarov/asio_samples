/*
TRANSLATOR ma::echo::server::qt::Service
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/utility/base_from_member.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/qt/execution_config.h>
#include <ma/echo/server/qt/signal_connect_error.h>
#include <ma/echo/server/qt/serviceforwardsignal.h>
#include <ma/echo/server/qt/serviceservantsignal.h>
#include <ma/echo/server/qt/service.h>

namespace ma
{    
namespace echo
{
namespace server
{    
namespace qt 
{
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

  class Service::servant : public boost::base_from_member<execution_system>
  {   
  private:
    typedef boost::base_from_member<execution_system> execution_system_base;
    typedef servant this_type;

  public:
    servant(const execution_config& the_execution_config, 
      const session_manager_config& the_session_manager_config)
      : execution_system_base(the_execution_config)      
      , session_manager_(boost::make_shared<session_manager>(
          boost::ref(execution_system_base::member.session_manager_io_service()),
          boost::ref(execution_system_base::member.session_io_service()), 
          the_session_manager_config))
    {
    }

    ~servant()
    {      
    }

    template <typename Handler>
    void create_threads(const Handler& handler)
    {
      execution_system_base::member.create_threads(handler);
    }
    
    session_manager_ptr get_session_manager() const
    {
      return session_manager_;
    }    

  private:     
    session_manager_ptr session_manager_;
  }; // class Service::servant

  Service::Service(QObject* parent)
    : QObject(parent)
    , currentState_(ServiceState::stopped)
    , servant_() 
    , servantSignal_()
  {
    forwardSignal_ = new ServiceForwardSignal(this);
    checkConnect(QObject::connect(forwardSignal_, 
      SIGNAL(startCompleted(const boost::system::error_code&)), 
      SIGNAL(startCompleted(const boost::system::error_code&)), 
      Qt::QueuedConnection));
    checkConnect(QObject::connect(forwardSignal_, 
      SIGNAL(stopCompleted(const boost::system::error_code&)), 
      SIGNAL(stopCompleted(const boost::system::error_code&)), 
      Qt::QueuedConnection));
    checkConnect(QObject::connect(forwardSignal_, 
      SIGNAL(workCompleted(const boost::system::error_code&)), 
      SIGNAL(workCompleted(const boost::system::error_code&)), 
      Qt::QueuedConnection));
  }

  Service::~Service()
  {    
  }

  void Service::asyncStart(const execution_config& the_execution_config, 
    const session_manager_config& the_session_manager_config)
  {
    if (ServiceState::stopped != currentState_)
    {      
      forwardSignal_->emitStartCompleted(server_error::invalid_state);
      return;
    }
    createServant(the_execution_config, the_session_manager_config);
    servant_->create_threads(
      boost::bind(&ServiceServantSignal::emitWorkThreadExceptionHappened, servantSignal_));
    servant_->get_session_manager()->async_start(
      boost::bind(&ServiceServantSignal::emitSessionManagerStartCompleted, servantSignal_, _1));
    currentState_ = ServiceState::startInProgress;
  }

  void Service::onSessionManagerStartCompleted(const boost::system::error_code& error)
  {    
    if (ServiceState::startInProgress == currentState_)
    {
      if (error)
      {
        destroyServant();
        currentState_ = ServiceState::stopped;
      }
      else
      {
        servant_->get_session_manager()->async_wait(
          boost::bind(&ServiceServantSignal::emitSessionManagerWaitCompleted, servantSignal_, _1));
        currentState_ = ServiceState::started;
      }
      emit startCompleted(error);
    }    
  }
  
  void Service::asyncStop()
  {    
    if (ServiceState::stopped == currentState_ || ServiceState::stopInProgress == currentState_)
    {
      forwardSignal_->emitStopCompleted(server_error::invalid_state);
      return;
    }
    if (ServiceState::startInProgress == currentState_)
    {
      forwardSignal_->emitStartCompleted(server_error::operation_aborted);
    }
    else if (ServiceState::started == currentState_)
    {
      forwardSignal_->emitWorkCompleted(server_error::operation_aborted);
    }
    servant_->get_session_manager()->async_stop(
      boost::bind(&ServiceServantSignal::emitSessionManagerStopCompleted, servantSignal_, _1));
    currentState_ = ServiceState::stopInProgress;
  }

  void Service::onSessionManagerStopCompleted(const boost::system::error_code& error)
  {
    if (ServiceState::stopInProgress == currentState_)
    {
      destroyServant();
      currentState_ = ServiceState::stopped;
      emit stopCompleted(error);
    }        
  }

  void Service::terminate()
  {    
    destroyServant();
    if (ServiceState::startInProgress == currentState_)
    {
      forwardSignal_->emitStartCompleted(server_error::operation_aborted);
    }
    else if (ServiceState::started == currentState_)
    {
      forwardSignal_->emitWorkCompleted(server_error::operation_aborted);
    }
    else if (ServiceState::stopInProgress == currentState_)
    {
      forwardSignal_->emitStopCompleted(server_error::operation_aborted);
    }
    currentState_ = ServiceState::stopped;
  }

  void Service::onSessionManagerWaitCompleted(const boost::system::error_code& error)
  {
    if (ServiceState::started == currentState_)
    {
      emit workCompleted(error);
    }
  }  

  void Service::onWorkThreadExceptionHappened()
  {
    if (ServiceState::stopped != currentState_)
    {
      emit exceptionHappened();
    }
  }

  void Service::createServant(const execution_config& the_execution_config, 
    const session_manager_config& the_session_manager_config)
  {
    servant_.reset(new servant(the_execution_config, the_session_manager_config));
    servantSignal_ = boost::make_shared<ServiceServantSignal>();
        
    checkConnect(QObject::connect(servantSignal_.get(), 
      SIGNAL(workThreadExceptionHappened()), 
      SLOT(onWorkThreadExceptionHappened()), 
      Qt::QueuedConnection));
    checkConnect(QObject::connect(servantSignal_.get(), 
      SIGNAL(sessionManagerStartCompleted(const boost::system::error_code&)), 
      SLOT(onSessionManagerStartCompleted(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(servantSignal_.get(), 
      SIGNAL(sessionManagerWaitCompleted(const boost::system::error_code&)), 
      SLOT(onSessionManagerWaitCompleted(const boost::system::error_code&)),
      Qt::QueuedConnection));
    checkConnect(QObject::connect(servantSignal_.get(), 
      SIGNAL(sessionManagerStopCompleted(const boost::system::error_code&)), 
      SLOT(onSessionManagerStopCompleted(const boost::system::error_code&)),
      Qt::QueuedConnection));
  }

  void Service::destroyServant()
  {
    if (servantSignal_)
    {
      servantSignal_->disconnect();
      servantSignal_.reset();
    }    
    servant_.reset();    
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
