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
#include <stdexcept>
#include <boost/optional.hpp>
#include <boost/thread/thread.hpp>
#include <boost/throw_exception.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <QtGlobal>
#include <QtCore/QTime>
#include <QtGui/QTextCursor>
#include <QtGui/QMessageBox>
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

  boost::optional<int> readOptionalValue(const QCheckBox& checkBox, const QSpinBox& spinBox)
  {
    if (Qt::Checked == checkBox.checkState())
    {
      return spinBox.value();
    }
    return boost::optional<int>();
  }

  class config_read_error : public std::exception
  {
  public:
    explicit config_read_error(const QString& message = QString())
      : std::exception()
      , message_(message)
    {
    }

#if defined(__GNUC__)
    ~config_read_error() throw ()
    {
    }
#endif

    const QString& message() const
    {
      return message_;
    }
  private:
    QString message_;
  }; // class config_read_error

  class widget_based_config_read_error : public config_read_error
  {
  public:
    explicit widget_based_config_read_error(QWidget* widget, const QString& message = QString())
      : config_read_error(message)
      , widget_(widget)
    {
    }

#if defined(__GNUC__)
    ~widget_based_config_read_error() throw ()
    {
    }
#endif

    QWidget* widget() const
    {
      return widget_;
    }
    
  private:    
    QWidget* widget_;
  }; //class widget_based_config_read_error

} // anonymous namespace

  MainForm::MainForm(Service& service, QWidget* parent, Qt::WFlags flags)
    : QWidget(parent, flags)
    , configWidgets_()
    , prevServiceState_(service.currentState())
    , service_(service)
  {
    ui_.setupUi(this);

    configWidgets_.push_back(boost::make_tuple(0, ui_.sessionManagerThreadsSpinBox));
    configWidgets_.push_back(boost::make_tuple(0, ui_.sessionThreadsSpinBox));
    configWidgets_.push_back(boost::make_tuple(1, ui_.maxSessionCountSpinBox));
    configWidgets_.push_back(boost::make_tuple(1, ui_.recycledSessionCountSpinBox));
    configWidgets_.push_back(boost::make_tuple(1, ui_.listenBacklogSpinBox));
    configWidgets_.push_back(boost::make_tuple(1, ui_.portNumberSpinBox));
    configWidgets_.push_back(boost::make_tuple(1, ui_.addressEdit));
    configWidgets_.push_back(boost::make_tuple(2, ui_.sessionBufferSizeSpinBox));
    configWidgets_.push_back(boost::make_tuple(2, ui_.sockRecvBufferSizeCheckBox));
    configWidgets_.push_back(boost::make_tuple(2, ui_.sockRecvBufferSizeSpinBox));
    configWidgets_.push_back(boost::make_tuple(2, ui_.sockSendBufferSizeCheckBox));
    configWidgets_.push_back(boost::make_tuple(2, ui_.sockSendBufferSizeSpinBox));
    configWidgets_.push_back(boost::make_tuple(2, ui_.tcpNoDelayComboBox));

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
    catch (const widget_based_config_read_error& e)
    {
      showError(e.message(), e.widget());
    }
    catch (const config_read_error& e)
    {
      showError(e.message());
    }
    catch (const std::exception&)
    {
      showError(tr("Unexpected error reading configuration."));
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

  execution_config MainForm::readExecutionConfig() const
  {   
    std::size_t sessionManagerThreadCount;
    std::size_t sessionThreadCount;
    QWidget*    currentWidget = 0;
    try
    {
      currentWidget = ui_.sessionManagerThreadsSpinBox;
      sessionManagerThreadCount = boost::numeric_cast<std::size_t>(ui_.sessionManagerThreadsSpinBox->value());

      currentWidget = ui_.sessionThreadsSpinBox;
      sessionThreadCount = boost::numeric_cast<std::size_t>(ui_.sessionThreadsSpinBox->value());
    }
    catch (const boost::numeric::bad_numeric_cast&)
    {
      boost::throw_exception(widget_based_config_read_error(
        currentWidget, tr("Invalid value.")));
    }

    return execution_config(sessionManagerThreadCount, sessionThreadCount);
  }

  session_config MainForm::readSessionConfig() const
  {
    boost::optional<int> socketRecvBufferSize = 
      readOptionalValue(*ui_.sockRecvBufferSizeCheckBox, *ui_.sockRecvBufferSizeSpinBox);

    boost::optional<int> socketSendBufferSize = 
      readOptionalValue(*ui_.sockSendBufferSizeCheckBox, *ui_.sockSendBufferSizeSpinBox);

    boost::optional<bool> tcpNoDelay;
    switch (ui_.tcpNoDelayComboBox->currentIndex())
    {
    case 1:
      tcpNoDelay = true;
      break;
    case 2:
      tcpNoDelay = false;
      break;
    }

    return session_config(
      boost::numeric_cast<std::size_t>(ui_.sessionBufferSizeSpinBox->value()),
      socketRecvBufferSize, socketSendBufferSize, tcpNoDelay);
  }

  session_manager_config MainForm::readSessionManagerConfig() const
  {
    unsigned short port;
    std::size_t    maxSessions;
    std::size_t    recycledSessions;
    int            listenBacklog;
    QWidget*       currentWidget = 0;
    try 
    {
      currentWidget = ui_.portNumberSpinBox;
      port = boost::numeric_cast<unsigned short>(ui_.portNumberSpinBox->value());

      currentWidget = ui_.maxSessionCountSpinBox;
      maxSessions = boost::numeric_cast<std::size_t>(ui_.maxSessionCountSpinBox->value());

      currentWidget = ui_.recycledSessionCountSpinBox;
      recycledSessions = boost::numeric_cast<std::size_t>(ui_.recycledSessionCountSpinBox->value());

      currentWidget = ui_.listenBacklogSpinBox;
      listenBacklog = ui_.listenBacklogSpinBox->value();
    }
    catch (const boost::numeric::bad_numeric_cast&)
    {
      boost::throw_exception(widget_based_config_read_error(
        currentWidget, tr("Invalid value.")));
    }
    
    boost::asio::ip::address listenAddress;
    try 
    {
      listenAddress = boost::asio::ip::address::from_string(
        ui_.addressEdit->text().toStdString());
    } 
    catch (const std::exception&)
    {
      boost::throw_exception(widget_based_config_read_error(
        ui_.addressEdit, tr("Failed to parse numeric IP address.")));
    }

    return session_manager_config(
      boost::asio::ip::tcp::endpoint(listenAddress, port), 
      maxSessions, recycledSessions, listenBacklog,
      readSessionConfig());
  }  

  MainForm::ServiceConfiguration MainForm::readServiceConfig() const
  {
    //todo: read and validate configuration
    execution_config executionConfig = readExecutionConfig();
    session_manager_config sessionManagerConfig = readSessionManagerConfig();
    return boost::make_tuple(executionConfig, sessionManagerConfig);
  }

  void MainForm::showError(const QString& message, QWidget* widget)
  {
    if (widget)
    {
      typedef std::vector<ConfigWidget>::iterator iterator_type;
      for (iterator_type i = configWidgets_.begin(), end = configWidgets_.end(); i != end; ++i)
      {
        if (widget == i->get<1>())
        {
          ui_.tabWidget->setCurrentIndex(i->get<0>());
          break;
        }
      }
      widget->setFocus(Qt::TabFocusReason);
    }
    QMessageBox::critical(this, tr("Invalid configuration"), message);
  }

  QString MainForm::getServiceStateWindowTitle(ServiceState::State serviceState)
  {
    switch (serviceState)
    {
    case ServiceState::Stopped:
      return QString();
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

      typedef std::vector<ConfigWidget>::iterator iterator_type;
      for (iterator_type i = configWidgets_.begin(), end = configWidgets_.end(); i != end; ++i)
      {
        i->get<1>()->setEnabled(serviceStopped);
      }
      ui_.sockRecvBufferSizeSpinBox->setEnabled(serviceStopped && 
        Qt::Checked == ui_.sockRecvBufferSizeCheckBox->checkState());
      ui_.sockSendBufferSizeSpinBox->setEnabled(serviceStopped && 
        Qt::Checked == ui_.sockSendBufferSizeCheckBox->checkState());

      QString windowTitle = tr("Qt Echo Server");
      QString statedWindowTitlePart = getServiceStateWindowTitle(serviceState);
      if (!statedWindowTitlePart.isNull())
      {
        windowTitle = tr("%1 (%2)").arg(windowTitle).arg(statedWindowTitlePart);
      }      
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
