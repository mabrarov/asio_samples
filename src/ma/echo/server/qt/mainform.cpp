/*
TRANSLATOR ma::echo::server::qt::MainForm
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <QtGlobal>
#include <QtCore/QTime>
#include <QtGui/QTextCursor>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_config.hpp>
#include <ma/echo/server/session_manager_config.hpp>
#include <ma/echo/server/qt/service.h>
#include <ma/echo/server/qt/signal_connect_error.h>
#include <ma/echo/server/qt/mainform.h>

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
  execution_config createExecutionConfig()
  {
    return execution_config(2, 2);
  }

  session_config createSessionConfig()
  {
    return session_config(4096);
  }

  session_manager_config createSessionManagerConfig(const session_config& sessionConfig)
  {
    using boost::asio::ip::tcp;

    unsigned short port = 7;
    std::size_t maxSessions = 1000;
    std::size_t recycledSessions = 100;
    int listenBacklog = 6;
    return session_manager_config(tcp::endpoint(tcp::v4(), port),
      maxSessions, recycledSessions, listenBacklog, sessionConfig);
  }
  
} // anonymous namespace

  MainForm::MainForm(Service& echoService, QWidget* parent, Qt::WFlags flags)
    : QDialog(parent, flags | Qt::WindowMinMaxButtonsHint)
    , prevEchoServiceState_(echoService.currentState())
    , echoService_(echoService)
  {
    ui_.setupUi(this);

    checkConnect(QObject::connect(&echoService, 
      SIGNAL(exceptionHappened()), 
      SLOT(on_echoService_exceptionHappened())));
    checkConnect(QObject::connect(&echoService, 
      SIGNAL(startCompleted(const boost::system::error_code&)), 
      SLOT(on_echoService_startCompleted(const boost::system::error_code&))));
    checkConnect(QObject::connect(&echoService, 
      SIGNAL(stopCompleted(const boost::system::error_code&)), 
      SLOT(on_echoService_stopCompleted(const boost::system::error_code&))));
    checkConnect(QObject::connect(&echoService, 
      SIGNAL(workCompleted(const boost::system::error_code&)), 
      SLOT(on_echoService_workCompleted(const boost::system::error_code&))));

    unsigned hardwareConcurrency = boost::thread::hardware_concurrency();
    unsigned defaultSessionManagerThreadCount = hardwareConcurrency < 2U ? 1U : 2U;
    unsigned defaultSessionThreadCount = hardwareConcurrency ? hardwareConcurrency + 1U : 2U;

    ui_.sessionManagementThreadsSpinBox->setValue(boost::numeric_cast<int>(defaultSessionManagerThreadCount));
    ui_.sessionThreadsSpinBox->setValue(boost::numeric_cast<int>(defaultSessionThreadCount));

    //todo: setup initial internal states    
    //todo: setup initial widgets' states
    updateWidgetsStates(true);
  }

  MainForm::~MainForm()
  {
  }

  void MainForm::on_startButton_clicked()
  {    
    //todo: read and validate configuration
    boost::optional<ServiceConfiguration> serviceConfiguration;
    try 
    {
       serviceConfiguration = readServiceConfiguration();
    }
    catch (...)
    {
      //todo
    }
    if (serviceConfiguration)
    {
      echoService_.asyncStart(serviceConfiguration->get<0>(), serviceConfiguration->get<1>());
      writeLog(QString::fromUtf8("Starting echo service..."));
    }
    updateWidgetsStates();
  }

  void MainForm::on_stopButton_clicked()
  {        
    echoService_.asyncStop();
    writeLog(QString::fromUtf8("Stopping echo service..."));
    updateWidgetsStates();
  }

  void MainForm::on_terminateButton_clicked()
  {
    writeLog(QString::fromUtf8("Terminating echo service..."));    
    echoService_.terminate();
    writeLog(QString::fromUtf8("Echo service terminated."));
    updateWidgetsStates();
  }

  void MainForm::on_echoService_startCompleted(const boost::system::error_code& error)
  {   
    if (server_error::invalid_state == error) 
    {
      writeLog(QString::fromUtf8("Echo service start completed with error due to invalid state for start operation."));
    }
    else if (server_error::operation_aborted == error) 
    {
      writeLog(QString::fromUtf8("Echo service start was cancelled."));
    }
    else if (error)
    {      
      writeLog(QString::fromUtf8("Echo service start completed with error."));
    }
    else 
    {
      writeLog(QString::fromUtf8("Echo service start completed successfully."));
    } 
    updateWidgetsStates();
  }
          
  void MainForm::on_echoService_stopCompleted(const boost::system::error_code& error)
  {    
    if (server_error::invalid_state == error) 
    {
      writeLog(QString::fromUtf8("Echo service stop completed with error due to invalid state for stop operation."));
    }
    else if (server_error::operation_aborted == error) 
    {
      writeLog(QString::fromUtf8("Echo service stop was cancelled."));
    }
    else if (error)
    {      
      writeLog(QString::fromUtf8("Echo service stop completed with error."));
    }    
    else 
    {
      writeLog(QString::fromUtf8("Echo service stop completed successfully."));
    }
    updateWidgetsStates();
  }

  void MainForm::on_echoService_workCompleted(const boost::system::error_code& error)
  {
    if (server_error::invalid_state == error) 
    {
      writeLog(QString::fromUtf8("Echo service work completed with error due to invalid state."));
    }
    else if (server_error::operation_aborted == error) 
    {
      writeLog(QString::fromUtf8("Echo service work was cancelled."));
    }
    else if (error)
    {      
      writeLog(QString::fromUtf8("Echo service work completed with error."));
    }    
    else 
    {
      writeLog(QString::fromUtf8("Echo service work completed successfully."));
    }
    if (ServiceState::started == echoService_.currentState())
    {
      echoService_.asyncStop(); 
      writeLog(QString::fromUtf8("Stopping echo service (because of its work was completed)..."));
    }
    updateWidgetsStates();
  }

  void MainForm::on_echoService_exceptionHappened()
  {
    writeLog(QString::fromUtf8("Unexpected error during echo service work. Terminating echo service..."));
    echoService_.terminate(); 
    writeLog(QString::fromUtf8("Echo service terminated."));
    updateWidgetsStates();
  }

  MainForm::ServiceConfiguration MainForm::readServiceConfiguration()
  {
    //todo: read and validate configuration
    execution_config executionConfig = createExecutionConfig();    
    session_manager_config sessionManagerConfig = createSessionManagerConfig(createSessionConfig());
    return boost::make_tuple(executionConfig, sessionManagerConfig);
  }

  void MainForm::updateWidgetsStates(bool ignorePrevEchoServiceState)
  {   
    static const char* windowTitleTr = QT_TR_NOOP("Qt Echo Server");
    static const char* stoppedTitleTr = QT_TR_NOOP("stopped");
    static const char* startingTitleTr = QT_TR_NOOP("starting");
    static const char* startedTitleTr = QT_TR_NOOP("started");
    static const char* stoppingTitleTr = QT_TR_NOOP("stopping");    
     
    ServiceState::State serviceState = echoService_.currentState();
    if (serviceState != prevEchoServiceState_ || ignorePrevEchoServiceState)
    {
      ui_.startButton->setEnabled(ServiceState::stopped == serviceState);
      ui_.stopButton->setEnabled(ServiceState::startInProgress == serviceState || ServiceState::started == serviceState);
      ui_.terminateButton->setEnabled(ServiceState::stopped != serviceState);      

      QString defWindowTitle = tr(windowTitleTr);
      QString statedWindowTitle = QString::fromUtf8("%1 - [%2]");
      switch (serviceState)
      {
      case ServiceState::stopped:
        statedWindowTitle = statedWindowTitle.arg(defWindowTitle).arg(tr(stoppedTitleTr));
        break;
      case ServiceState::startInProgress:
        statedWindowTitle = statedWindowTitle.arg(defWindowTitle).arg(tr(startingTitleTr));
        break;
      case ServiceState::started:
        statedWindowTitle = statedWindowTitle.arg(defWindowTitle).arg(tr(startedTitleTr));
        break;
      case ServiceState::stopInProgress:
        statedWindowTitle = statedWindowTitle.arg(defWindowTitle).arg(tr(stoppingTitleTr));
        break;
      default:
        statedWindowTitle = defWindowTitle;
      }
      setWindowTitle(statedWindowTitle);
    }
    prevEchoServiceState_ = serviceState;
  }

  void MainForm::writeLog(const QString& message)
  {    
    QString logMessage = QString::fromUtf8("[%1] %2")
      .arg(QTime::currentTime().toString(Qt::SystemLocaleLongDate))
      .arg(message);
    ui_.logTextEdit->appendPlainText(logMessage);
    ui_.logTextEdit->moveCursor(QTextCursor::End);
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
