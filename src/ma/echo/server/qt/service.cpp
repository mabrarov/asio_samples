/*
TRANSLATOR ma::echo::server::qt::Service
*/

//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <vector>
#include <boost/ref.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/utility/base_from_member.hpp>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/simple_session_factory.hpp>
#include <ma/echo/server/pooled_session_factory.hpp>
#include <ma/echo/server/session_manager.hpp>
#include <ma/echo/server/qt/execution_config.h>
#include <ma/echo/server/qt/signal_connect_error.h>
#include <ma/echo/server/qt/serviceforwardsignal.h>
#include <ma/echo/server/qt/serviceservantsignal.h>
#include <ma/echo/server/qt/service.h>

namespace ma {
namespace echo {
namespace server {
namespace qt {

namespace {

typedef boost::shared_ptr<boost::asio::io_service> io_service_ptr;
typedef std::vector<io_service_ptr>                io_service_vector;
typedef boost::shared_ptr<ma::echo::server::session_factory>
    session_factory_ptr;
typedef boost::shared_ptr<boost::asio::io_service::work> io_service_work_ptr;
typedef std::vector<io_service_work_ptr> io_service_work_vector;

class pipeline_base_0 : private boost::noncopyable
{
public:
  explicit pipeline_base_0(const execution_config& config)
    : ios_per_work_thread_(config.ios_per_work_thread)
    , session_manager_thread_count_(config.session_manager_thread_count)
    , session_thread_count_(config.session_thread_count)
    , session_io_services_(create_session_io_services(config))
  {
  }

  io_service_vector session_io_services() const
  {
    return session_io_services_;
  }

protected:
  ~pipeline_base_0()
  {
  }

  const bool ios_per_work_thread_;
  const std::size_t session_manager_thread_count_;
  const std::size_t session_thread_count_;
  const io_service_vector session_io_services_;

private:
  static io_service_vector create_session_io_services(
      const execution_config& exec_config)
  {
    io_service_vector io_services;
    if (exec_config.ios_per_work_thread)
    {
      for (std::size_t i = 0; i != exec_config.session_thread_count; ++i)
      {
        io_services.push_back(boost::make_shared<boost::asio::io_service>(1));
      }
    }
    else
    {
      io_service_ptr io_service = boost::make_shared<boost::asio::io_service>(
          exec_config.session_thread_count);
      io_services.push_back(io_service);
    }
    return io_services;
  }
}; // pipeline_base_0

class pipeline_base_1 : public pipeline_base_0
{
public:
  pipeline_base_1(const execution_config& exec_config,
      const ma::echo::server::session_manager_config& session_manager_config)
    : pipeline_base_0(exec_config)
    , session_factory_(create_session_factory(exec_config,
          session_manager_config, session_io_services_))
  {
  }

  ma::echo::server::session_factory& session_factory() const
  {
    return *session_factory_;
  }

protected:
  ~pipeline_base_1()
  {
  }

  const session_factory_ptr session_factory_;

private:
  static session_factory_ptr create_session_factory(
      const execution_config& exec_config,
      const ma::echo::server::session_manager_config& session_manager_config,
      const io_service_vector& session_io_services)
  {
    if (exec_config.ios_per_work_thread)
    {
      return boost::make_shared<ma::echo::server::pooled_session_factory>(
          session_io_services, session_manager_config.recycled_session_count);
    }
    else
    {
      boost::asio::io_service& io_service = *session_io_services.front();
      return boost::make_shared<ma::echo::server::simple_session_factory>(
          boost::ref(io_service),
          session_manager_config.recycled_session_count);
    }
  }
}; // class pipeline_base_1

class pipeline_base_2 : public pipeline_base_1
{
public:
  explicit pipeline_base_2(const execution_config& exec_config,
      const ma::echo::server::session_manager_config& session_manager_config)
    : pipeline_base_1(exec_config, session_manager_config)
    , session_manager_io_service_(exec_config.session_manager_thread_count)
  {
  }

  boost::asio::io_service& session_manager_io_service()
  {
    return session_manager_io_service_;
  }

protected:
  ~pipeline_base_2()
  {
  }

  boost::asio::io_service session_manager_io_service_;
}; // class pipeline_base_2

class pipeline : public pipeline_base_2
{
private:
  typedef pipeline this_type;

public:
  explicit pipeline(const execution_config& exec_config,
      const ma::echo::server::session_manager_config& session_manager_config)
    : pipeline_base_2(exec_config, session_manager_config)
    , session_work_(create_works(session_io_services_))
    , session_manager_work_(session_manager_io_service_)
    , threads_()
  {
  }

  ~pipeline()
  {
    stop_threads();
  }

  template <typename Handler>
  void create_threads(const Handler& handler)
  {
    typedef boost::tuple<Handler> wrapped_handler_type;
    typedef void (*thread_func_type)(boost::asio::io_service&,
        wrapped_handler_type);

    wrapped_handler_type wrapped_handler = boost::make_tuple(handler);
    thread_func_type func = &this_type::thread_func<Handler>;

    if (ios_per_work_thread_)
    {
      for (io_service_vector::const_iterator i = session_io_services_.begin(),
          end = session_io_services_.end(); i != end; ++i)
      {
        threads_.create_thread(
            boost::bind(func, boost::ref(**i), wrapped_handler));
      }
    }
    else
    {
      boost::asio::io_service& io_service = *session_io_services_.front();
      for (std::size_t i = 0; i != session_thread_count_; ++i)
      {
        threads_.create_thread(
            boost::bind(func, boost::ref(io_service), wrapped_handler));
      }
    }

    for (std::size_t i = 0; i != session_manager_thread_count_; ++i)
    {
      threads_.create_thread(boost::bind(func,
          boost::ref(session_manager_io_service_), wrapped_handler));
    }
  }  

private:

  void stop_threads()
  {
    session_manager_io_service_.stop();
    stop(session_io_services_);
    threads_.join_all();
  }

  template <typename Handler>
  static void thread_func(boost::asio::io_service& io_service,
      boost::tuple<Handler> handler)
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

  static io_service_work_vector create_works(
      const io_service_vector& io_services)
  {
    io_service_work_vector works;
    for (io_service_vector::const_iterator i = io_services.begin(),
        end = io_services.end(); i != end; ++i)
    {
      works.push_back(
          boost::make_shared<boost::asio::io_service::work>(boost::ref(**i)));
    }
    return works;
  }

  static void stop(const io_service_vector& io_services)
  {
    for (io_service_vector::const_iterator i = io_services.begin(),
        end = io_services.end(); i != end; ++i)
    {
      (*i)->stop();
    }
  }

  const io_service_work_vector session_work_;
  const boost::asio::io_service::work session_manager_work_;
  boost::thread_group threads_;
}; // class pipeline

} // anonymous namespace

class Service::servant : public pipeline
{
private:
  typedef servant this_type;

public:
  servant(const execution_config& the_execution_config,
      const session_manager_config& the_session_manager_config)
    : pipeline(the_execution_config, the_session_manager_config)
    , session_manager_(session_manager::create(session_manager_io_service_,
          *session_factory_, the_session_manager_config))
  {
  }

  ~servant()
  {
  }

  template <typename Handler>
  void start_session_manager(const Handler& handler)
  {
    session_manager_->async_start(handler);
  }

  template <typename Handler>
  void wait_session_manager(const Handler& handler)
  {
    session_manager_->async_wait(handler);
  }

  template <typename Handler>
  void stop_session_manager(const Handler& handler)
  {
    session_manager_->async_stop(handler);
  }

  session_manager_stats stats() const
  {
    return session_manager_->stats();
  }

private:
  session_manager_ptr session_manager_;
}; // class Service::servant

Service::Service(QObject* parent)
  : QObject(parent)
  , state_(ServiceState::Stopped)
  , stats_()
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
  if (ServiceState::Stopped != state_)
  {
    forwardSignal_->emitStartCompleted(server::error::invalid_state);
    return;
  }

  createServant(the_execution_config, the_session_manager_config);

  servant_->create_threads(
      boost::bind(&ServiceServantSignal::emitWorkThreadExceptionHappened,
          servantSignal_));

  servant_->start_session_manager(
      boost::bind(&ServiceServantSignal::emitSessionManagerStartCompleted,
          servantSignal_, _1));

  state_ = ServiceState::Starting;
}

session_manager_stats Service::stats() const
{
  if (servant_)
  {
    return servant_->stats();
  }
  else
  {
    return stats_;
  }
}

void Service::onSessionManagerStartCompleted(
    const boost::system::error_code& error)
{
  if (ServiceState::Starting != state_)
  {
    return;
  }

  if (error)
  {
    destroyServant();
    state_ = ServiceState::Stopped;
  }
  else
  {
    servant_->wait_session_manager(
        boost::bind(&ServiceServantSignal::emitSessionManagerWaitCompleted,
            servantSignal_, _1));

    state_ = ServiceState::Working;
  }

  emit startCompleted(error);
}

void Service::asyncStop()
{
  if ((ServiceState::Stopped == state_) || (ServiceState::Stopping == state_))
  {
    forwardSignal_->emitStopCompleted(server::error::invalid_state);
    return;
  }

  if (ServiceState::Starting == state_)
  {
    forwardSignal_->emitStartCompleted(server::error::operation_aborted);
  }
  else if (ServiceState::Working == state_)
  {
    forwardSignal_->emitWorkCompleted(server::error::operation_aborted);
  }

  servant_->stop_session_manager(
      boost::bind(&ServiceServantSignal::emitSessionManagerStopCompleted,
          servantSignal_, _1));

  state_ = ServiceState::Stopping;
}

void Service::onSessionManagerStopCompleted(
    const boost::system::error_code& error)
{
  if (ServiceState::Stopping != state_)
  {
    return;
  }

  destroyServant();
  state_ = ServiceState::Stopped;
  emit stopCompleted(error);
}

void Service::terminate()
{
  destroyServant();

  if (ServiceState::Starting == state_)
  {
    forwardSignal_->emitStartCompleted(server::error::operation_aborted);
  }
  else if (ServiceState::Working == state_)
  {
    forwardSignal_->emitWorkCompleted(server::error::operation_aborted);
  }
  else if (ServiceState::Stopping == state_)
  {
    forwardSignal_->emitStopCompleted(server::error::operation_aborted);
  }

  state_ = ServiceState::Stopped;
}

void Service::onSessionManagerWaitCompleted(
    const boost::system::error_code& error)
{
  if (ServiceState::Working == state_)
  {
    emit workCompleted(error);
  }
}

void Service::onWorkThreadExceptionHappened()
{
  if (ServiceState::Stopped != state_)
  {
    emit exceptionHappened();
  }
}

void Service::createServant(const execution_config& the_execution_config,
    const session_manager_config& the_session_manager_config)
{
  servant_.reset(new servant(
      the_execution_config, the_session_manager_config));
  stats_ = servant_->stats();

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
  stats_ = servant_->stats();
  servant_.reset();
}

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
