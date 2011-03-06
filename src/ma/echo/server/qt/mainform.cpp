/*
TRANSLATOR ma::echo::server::qt::MainForm
*/

//
// Copyright (c) 2010-2011 Marat Abrarov (abrarov@mail.ru)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <limits>
#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <QtGlobal>
#include <QtCore/QTime>
#include <QtGui/QTextCursor>
#include <ma/echo/server/error.hpp>
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
  std::size_t calcSessionManagerThreadCount(std::size_t hardwareConcurrency)
  {
    if (hardwareConcurrency < 2)
    {
      return 1;
    }
    return 2;
  }

  std::size_t calcSessionThreadCount(std::size_t hardwareConcurrency)
  {
    if (hardwareConcurrency)
    {
      if ((std::numeric_limits<std::size_t>::max)() == hardwareConcurrency)
      {
        return hardwareConcurrency;
      }
      return hardwareConcurrency + 1;
    }
    return 2;
  }

  execution_config createTestExecutionConfig()
  {
    return execution_config(2, 2);
  }

  session_config createTestSessionConfig()
  {
    return session_config(4096);
  }

  session_manager_config createTestSessionManagerConfig(const session_config& sessionConfig)
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

  MainForm::MainForm(Service& service, QWidget* parent, Qt::WFlags flags)
    : QDialog(parent, flags | Qt::WindowMinMaxButtonsHint)
    , configInputs_()
    , prevServiceState_(service.currentState())
    , service_(service)
  {
    ui_.setupUi(this);

    configInputs_.push_back(ui_.sessionManagerThreadsSpinBox);
    configInputs_.push_back(ui_.sessionThreadsSpinBox);
    configInputs_.push_back(ui_.maxSessionCountSpinBox);
    configInputs_.push_back(ui_.recycledSessionCountSpinBox);
    configInputs_.push_back(ui_.listenBacklogSpinBox);
    configInputs_.push_back(ui_.portNumberSpinBox);
    configInputs_.push_back(ui_.addressEdit);
    configInputs_.push_back(ui_.sessionBufferSizeSpinBox);
    configInputs_.push_back(ui_.sockRecvBufferSizeCheckBox);
    configInputs_.push_back(ui_.sockRecvBufferSizeSpinBox);
    configInputs_.push_back(ui_.sockSendBufferSizeCheckBox);
    configInputs_.push_back(ui_.sockSendBufferSizeSpinBox);
    configInputs_.push_back(ui_.tcpNoDelayComboBox);

    checkConnect(QObject::connect(&service, 
      SIGNAL(exceptionHappened()), 
      SLOT(on_service_exceptionHappened())));
    checkConnect(QObject::connect(&service, 
      SIGNAL(startCompleted(const boost::system::error_code&)), 
      SLOT(on_service_startCompleted(const boost::system::error_code&))));
    checkConnect(QObject::connect(&service, 
      SIGNAL(stopCompleted(const boost::system::error_code&)), 
      SLOT(on_service_stopCompleted(const boost::system::error_code&))));
    checkConnect(QObject::connect(&service, 
      SIGNAL(workCompleted(const boost::system::error_code&)), 
      SLOT(on_service_workCompleted(const boost::system::error_code&))));

    unsigned hardwareConcurrency = boost::thread::hardware_concurrency();
    ui_.sessionManagerThreadsSpinBox->setValue(
      boost::numeric_cast<int>(calcSessionManagerThreadCount(hardwareConcurrency)));
    ui_.sessionThreadsSpinBox->setValue(
      boost::numeric_cast<int>(calcSessionThreadCount(hardwareConcurrency)));    
    
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
       serviceConfiguration = readServiceConfig();
    }
    catch (...)
    {
      //todo
    }
    if (serviceConfiguration)
    {
      service_.asyncStart(serviceConfiguration->get<0>(), serviceConfiguration->get<1>());
      writeLog(tr("Starting echo service..."));
    }
    updateWidgetsStates();
  }

  void MainForm::on_stopButton_clicked()
  {        
    service_.asyncStop();
    writeLog(tr("Stopping echo service..."));
    updateWidgetsStates();
  }

  void MainForm::on_terminateButton_clicked()
  {
    writeLog(tr("Terminating echo service..."));    
    service_.terminate();
    writeLog(tr("Echo service terminated."));
    updateWidgetsStates();
  }

  void MainForm::on_service_startCompleted(const boost::system::error_code& error)
  {   
    if (server_error::invalid_state == error) 
    {
      writeLog(tr("Echo service start completed with error due to invalid state for start operation."));
    }
    else if (server_error::operation_aborted == error) 
    {
      writeLog(tr("Echo service start was canceled."));
    }
    else if (error)
    {      
      writeLog(tr("Echo service start completed with error."));
    }
    else 
    {
      writeLog(tr("Echo service start completed successfully."));
    } 
    updateWidgetsStates();
  }
          
  void MainForm::on_service_stopCompleted(const boost::system::error_code& error)
  {    
    if (server_error::invalid_state == error) 
    {
      writeLog(tr("Echo service stop completed with error due to invalid state for stop operation."));
    }
    else if (server_error::operation_aborted == error) 
    {
      writeLog(tr("Echo service stop was canceled."));
    }
    else if (error)
    {      
      writeLog(tr("Echo service stop completed with error."));
    }    
    else 
    {
      writeLog(tr("Echo service stop completed successfully."));
    }
    updateWidgetsStates();
  }

  void MainForm::on_service_workCompleted(const boost::system::error_code& error)
  {
    if (server_error::invalid_state == error) 
    {
      writeLog(tr("Echo service work completed with error due to invalid state."));
    }
    else if (server_error::operation_aborted == error) 
    {
      writeLog(tr("Echo service work was canceled."));
    }
    else if (error)
    {      
      writeLog(tr("Echo service work completed with error."));
    }    
    else 
    {
      writeLog(tr("Echo service work completed successfully."));
    }
    if (ServiceState::Started == service_.currentState())
    {
      service_.asyncStop(); 
      writeLog(tr("Stopping echo service (because of its work was completed)..."));
    }
    updateWidgetsStates();
  }

  void MainForm::on_service_exceptionHappened()
  {
    writeLog(tr("Unexpected error during echo service work. Terminating echo service..."));
    service_.terminate(); 
    writeLog(tr("Echo service terminated."));
    updateWidgetsStates();
  } 

  execution_config MainForm::readExecutionConfig()
  {
    //todo
    return createTestExecutionConfig();
  }

  session_config MainForm::readSessionConfig()
  {
    //todo
    return createTestSessionConfig();
  }

  session_manager_config MainForm::readSessionManagerConfig()
  {
    //todo
    return createTestSessionManagerConfig(readSessionConfig());
  }  

  MainForm::ServiceConfiguration MainForm::readServiceConfig()
  {
    //todo: read and validate configuration
    execution_config executionConfig = readExecutionConfig();
    session_manager_config sessionManagerConfig = readSessionManagerConfig();
    return boost::make_tuple(executionConfig, sessionManagerConfig);
  }

  QString MainForm::getStateDescription(ServiceState::State serviceState)
  {
    switch (serviceState)
    {
    case ServiceState::Stopped:
      return tr("stopped");
    case ServiceState::Starting:
      return tr("starting");
    case ServiceState::Started:
      return tr("started");
    case ServiceState::Stopping:
      return tr("stopping");    
    default:
      return tr("unknown state");
    }    
  }

  void MainForm::updateWidgetsStates(bool ignorePrevServiceState)
  {
    ServiceState::State serviceState = service_.currentState();
    if (serviceState != prevServiceState_ || ignorePrevServiceState)
    {
      bool serviceStopped = ServiceState::Stopped == serviceState;
      ui_.startButton->setEnabled(serviceStopped);
      ui_.stopButton->setEnabled(ServiceState::Starting == serviceState || ServiceState::Started == serviceState);
      ui_.terminateButton->setEnabled(!serviceStopped);

      typedef std::vector<QWidget*>::iterator iterator_type;
      for (iterator_type i = configInputs_.begin(), end = configInputs_.end(); i != end; ++i)
      {
        (*i)->setEnabled(serviceStopped);
      }
      ui_.sockRecvBufferSizeSpinBox->setEnabled(serviceStopped && 
        Qt::Checked == ui_.sockRecvBufferSizeCheckBox->checkState());
      ui_.sockSendBufferSizeSpinBox->setEnabled(serviceStopped && 
        Qt::Checked == ui_.sockSendBufferSizeCheckBox->checkState());
      
      QString windowTitle = tr("%1 (%2)").arg(tr("Qt Echo Server")).arg(getStateDescription(serviceState));
      setWindowTitle(windowTitle);
    }
    prevServiceState_ = serviceState;
  }

  void MainForm::writeLog(const QString& message)
  {    
    QString logMessage = tr("[%1] %2").arg(QTime::currentTime().toString(Qt::SystemLocaleLongDate)).arg(message);
    ui_.logTextEdit->appendPlainText(logMessage);
    ui_.logTextEdit->moveCursor(QTextCursor::End);
  }

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
