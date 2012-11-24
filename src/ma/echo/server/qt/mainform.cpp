/*
TRANSLATOR ma::echo::server::qt::MainForm
*/

//
// Copyright (c) 2010-2012 Marat Abrarov (abrarov@mail.ru)
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
#include <QtCore/QTimer>
#include <QtGui/QTextCursor>
#include <QtGui/QMessageBox>
#include <ma/echo/server/error.hpp>
#include <ma/echo/server/session_manager_stats.hpp>
#include <ma/echo/server/qt/service.h>
#include <ma/echo/server/qt/signal_connect_error.h>
#include <ma/echo/server/qt/mainform.h>

namespace ma {
namespace echo {
namespace server {
namespace qt {

namespace {

const int statsTimerIntervalMillis = 1000;
const int singleDemuxModelIndex    = 0;
const int poolOfDemuxModelIndex    = 1;

QString format(std::size_t value)
{
  return QString("%1").arg(value);
}

QString format(const session_manager_stats::limited_counter& counter)
{
  if (counter.overflowed())
  {
    return QString(">%1").arg(counter.value());
  }
  else
  {
    return QString("%1").arg(counter.value());
  }
}

std::size_t calcSessionManagerThreadCount(std::size_t /*hardwareConcurrency*/)
{
  return 1;
}

std::size_t calcSessionThreadCount(std::size_t hardwareConcurrency)
{
  if (hardwareConcurrency)
  {
    return hardwareConcurrency;
  }
  return 2;
}

int getDefaultSessionThreadModelIndex()
{
#if defined(WIN32)
  return singleDemuxModelIndex;
#else
  return poolOfDemuxModelIndex;
#endif
}

boost::optional<int> readOptionalValue(const QCheckBox& checkBox,
    const QSpinBox& spinBox)
{
  if (Qt::Checked == checkBox.checkState())
  {
    return spinBox.value();
  }
  return boost::optional<int>();
}

class option_read_error : public std::runtime_error
{
public:
  explicit option_read_error(const QString& message = QString())
    : std::runtime_error(message.toLocal8Bit().data())
    , message_(message)
  {
  }

#if defined(__GNUC__)
  ~option_read_error() throw ()
  {
  }
#endif

  const QString& message() const
  {
    return message_;
  }

private:
  QString message_;
}; // class option_read_error

class widget_option_read_error : public option_read_error
{
public:
  explicit widget_option_read_error(QWidget* widget,
      const QString& message = QString())
    : option_read_error(message)
    , widget_(widget)
  {
  }

#if defined(__GNUC__)
  ~widget_option_read_error() throw ()
  {
  }
#endif

  QWidget* widget() const
  {
    return widget_;
  }

private:
  QWidget* widget_;
}; // class widget_option_read_error

} // anonymous namespace

MainForm::MainForm(Service& service, QWidget* parent, Qt::WFlags flags)
  : QWidget(parent, flags)
  , optionsWidgets_()
  , prevServiceState_(service.state())
  , service_(service)
{
  ui_.setupUi(this);

  statsTimer_ = new QTimer(this);
  checkConnect(QObject::connect(statsTimer_,
      SIGNAL(timeout()), SLOT(on_statsTimer_timeout())));

  optionsWidgets_.push_back(
      boost::make_tuple(0, ui_.sessionManagerThreadsSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(0, ui_.sessionThreadsSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(0, ui_.sessionThreadModelComboBox));
  optionsWidgets_.push_back(
      boost::make_tuple(1, ui_.maxSessionCountSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(1, ui_.recycledSessionCountSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(1, ui_.listenBacklogSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(1, ui_.portNumberSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(1, ui_.addressEdit));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.sessionBufferSizeSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.inactivityTimeoutCheckBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.inactivityTimeoutSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.maxTransferSizeSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.sockRecvBufferSizeCheckBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.sockRecvBufferSizeSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.sockSendBufferSizeCheckBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.sockSendBufferSizeSpinBox));
  optionsWidgets_.push_back(
      boost::make_tuple(2, ui_.tcpNoDelayComboBox));

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
  ui_.sessionManagerThreadsSpinBox->setValue(boost::numeric_cast<int>(
      calcSessionManagerThreadCount(hardwareConcurrency)));
  ui_.sessionThreadsSpinBox->setValue(boost::numeric_cast<int>(
      calcSessionThreadCount(hardwareConcurrency)));
  ui_.sessionThreadModelComboBox->setCurrentIndex(
      getDefaultSessionThreadModelIndex());
  ui_.listenBacklogSpinBox->setMaximum(std::numeric_limits<int>::max());

  updateWidgetsStates(true);
}

MainForm::~MainForm()
{
  stopStatsTimer();
}

void MainForm::on_startButton_clicked()
{
  boost::optional<ServiceConfig> serviceConfig;
  try
  {
    serviceConfig = buildServiceConfig();
  }
  catch (const widget_option_read_error& e)
  {
    showConfigError(e.message(), e.widget());
  }
  catch (const option_read_error& e)
  {
    showConfigError(e.message());
  }
  catch (const std::exception&)
  {
    showConfigError(tr("Unexpected error reading configuration."));
  }
  if (serviceConfig)
  {
    service_.asyncStart(serviceConfig->get<0>(), serviceConfig->get<1>());
    writeLog(tr("Starting echo service."));

    startStatsTimer();
    showStats(service_.stats());
  }
  updateWidgetsStates();
}

void MainForm::on_stopButton_clicked()
{
  service_.asyncStop();
  writeLog(tr("Stopping echo service."));

  showStats(service_.stats());
  updateWidgetsStates();
}

void MainForm::on_terminateButton_clicked()
{
  writeLog(tr("Terminating echo service."));
  service_.terminate();
  writeLog(tr("Echo service terminated."));

  showStats(service_.stats());
  conditionalStopStatsTimer(service_.state());
  updateWidgetsStates();
}

void MainForm::on_statsTimer_timeout()
{
  showStats(service_.stats());
}

void MainForm::on_service_startCompleted(
    const boost::system::error_code& error)
{
  if (server::error::invalid_state == error)
  {
    writeLog(tr("Echo service start completed" \
        " with error due to invalid state for start operation."));
  }
  else if (server::error::operation_aborted == error)
  {
    writeLog(tr("Echo service start was canceled."));
  }
  else if (error)
  {
    writeLog(tr("Echo service start completed with error: ")
        + QString::fromLocal8Bit(error.message().c_str()));
  }
  else
  {
    writeLog(tr("Echo service start completed successfully."));
  }

  showStats(service_.stats());
  conditionalStopStatsTimer(service_.state());
  updateWidgetsStates();
}

void MainForm::on_service_stopCompleted(const boost::system::error_code& error)
{
  if (server::error::invalid_state == error)
  {
    writeLog(tr("Echo service stop completed" \
        " with error due to invalid state for stop operation."));
  }
  else if (server::error::operation_aborted == error)
  {
    writeLog(tr("Echo service stop was canceled."));
  }
  else if (error)
  {
    writeLog(tr("Echo service stop completed with error: ")
        + QString::fromLocal8Bit(error.message().c_str()));
  }
  else
  {
    writeLog(tr("Echo service stop completed successfully."));
  }

  showStats(service_.stats());
  conditionalStopStatsTimer(service_.state());
  updateWidgetsStates();
}

void MainForm::on_service_workCompleted(const boost::system::error_code& error)
{
  if (server::error::invalid_state == error)
  {
    writeLog(tr("Echo service work completed" \
        " with error due to invalid state."));
  }
  else if (server::error::operation_aborted == error)
  {
    writeLog(tr("Echo service work was canceled."));
  }
  else if (error)
  {
    writeLog(tr("Echo service work completed with error: ")
        + QString::fromLocal8Bit(error.message().c_str()));
  }
  else
  {
    writeLog(tr("Echo service work completed successfully."));
  }
  if (ServiceState::Working == service_.state())
  {
    service_.asyncStop();
    writeLog(tr("Stopping echo service" \
        " (because of its work was completed)."));
  }

  showStats(service_.stats());
  conditionalStopStatsTimer(service_.state());
  updateWidgetsStates();
}

void MainForm::on_service_exceptionHappened()
{
  writeLog(tr("Unexpected error during echo service work." \
      " Terminating echo service."));
  service_.terminate();
  writeLog(tr("Echo service terminated."));

  showStats(service_.stats());
  conditionalStopStatsTimer(service_.state());
  updateWidgetsStates();
}

execution_config MainForm::buildExecutionConfig() const
{
  std::size_t sessionManagerThreadCount;
  std::size_t sessionThreadCount;
  QWidget*    currentWidget = 0;
  bool        ios_per_work_thread;
  try
  {
    currentWidget = ui_.sessionManagerThreadsSpinBox;
    sessionManagerThreadCount = boost::numeric_cast<std::size_t>(
        ui_.sessionManagerThreadsSpinBox->value());

    currentWidget = ui_.sessionThreadsSpinBox;
    sessionThreadCount = boost::numeric_cast<std::size_t>(
        ui_.sessionThreadsSpinBox->value());

    int sessionThreadModel = ui_.sessionThreadModelComboBox->currentIndex();
    ios_per_work_thread = poolOfDemuxModelIndex == sessionThreadModel;
  }
  catch (const boost::numeric::bad_numeric_cast&)
  {
    boost::throw_exception(widget_option_read_error(
      currentWidget, tr("Invalid value.")));
  }

  return execution_config(ios_per_work_thread,
      sessionManagerThreadCount, sessionThreadCount);
}

session_config MainForm::buildSessionConfig() const
{
  session_config::optional_time_duration inactivityTimeout;
  if (boost::optional<int> timeoutSeconds = readOptionalValue(
      *ui_.inactivityTimeoutCheckBox, *ui_.inactivityTimeoutSpinBox))
  {
    inactivityTimeout =
        boost::posix_time::seconds(boost::numeric_cast<long>(*timeoutSeconds));
  }

  boost::optional<int> socketRecvBufferSize = readOptionalValue(
      *ui_.sockRecvBufferSizeCheckBox, *ui_.sockRecvBufferSizeSpinBox);

  boost::optional<int> socketSendBufferSize = readOptionalValue(
      *ui_.sockSendBufferSizeCheckBox, *ui_.sockSendBufferSizeSpinBox);

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
      boost::numeric_cast<std::size_t>(ui_.maxTransferSizeSpinBox->value()),
      socketRecvBufferSize, socketSendBufferSize,
      tcpNoDelay, inactivityTimeout);
}

session_manager_config MainForm::buildSessionManagerConfig() const
{
  unsigned short port;
  std::size_t    maxSessions;
  std::size_t    recycledSessions;
  std::size_t    maxStoppingSessions;
  int            listenBacklog;
  QWidget*       currentWidget = 0;
  try
  {
    currentWidget = ui_.portNumberSpinBox;
    port = boost::numeric_cast<unsigned short>(ui_.portNumberSpinBox->value());

    currentWidget = ui_.maxSessionCountSpinBox;
    maxSessions = boost::numeric_cast<std::size_t>(
        ui_.maxSessionCountSpinBox->value());

    currentWidget = ui_.recycledSessionCountSpinBox;
    recycledSessions = boost::numeric_cast<std::size_t>(
        ui_.recycledSessionCountSpinBox->value());

    //todo: read from UI
    maxStoppingSessions = 1000;

    currentWidget = ui_.listenBacklogSpinBox;
    listenBacklog = ui_.listenBacklogSpinBox->value();
  }
  catch (const boost::numeric::bad_numeric_cast&)
  {
    boost::throw_exception(widget_option_read_error(
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
    boost::throw_exception(widget_option_read_error(
        ui_.addressEdit, tr("Failed to parse numeric IP address.")));
  }

  return session_manager_config(
      boost::asio::ip::tcp::endpoint(listenAddress, port),
      maxSessions, recycledSessions, maxStoppingSessions, listenBacklog, 
      buildSessionConfig());
}

MainForm::ServiceConfig MainForm::buildServiceConfig() const
{
  execution_config executionConfig = buildExecutionConfig();
  session_manager_config sessionManagerOptions = buildSessionManagerConfig();
  return boost::make_tuple(executionConfig, sessionManagerOptions);
}

void MainForm::startStatsTimer()
{
  statsTimer_->start(statsTimerIntervalMillis);
}

void MainForm::stopStatsTimer()
{
  statsTimer_->stop();
}

void MainForm::conditionalStopStatsTimer(ServiceState::State state)
{
  if (ServiceState::Stopped == state)
  {
    statsTimer_->stop();
  }
}

void MainForm::showConfigError(const QString& message, QWidget* widget)
{
  if (widget)
  {
    typedef std::vector<OptionWidget>::iterator iterator_type;
    for (iterator_type i = optionsWidgets_.begin(),
        end = optionsWidgets_.end(); i != end; ++i)
    {
      if (widget == i->get<1>())
      {
        ui_.configTabWidget->setCurrentIndex(i->get<0>());
        break;
      }
    }
    widget->setFocus(Qt::TabFocusReason);
  }
  QMessageBox::critical(this, tr("Invalid configuration"), message);
}

void MainForm::showStats(const session_manager_stats& stats)
{
  ui_.activeSessionsEdit->setText(format(stats.active));
  ui_.maxActiveSessionsEdit->setText(format(stats.max_active));
  ui_.recycledSessionsEdit->setText(format(stats.recycled));
  ui_.totalAcceptedSessionsEdit->setText(format(stats.total_accepted));
  ui_.activeShutdownedSessionsEdit->setText(
      format(stats.active_shutdowned));
  ui_.outOfWorkSessionsEdit->setText(format(stats.out_of_work));
  ui_.timedOutSessionsEdit->setText(format(stats.timed_out));
  ui_.errorStoppedSessionsEdit->setText(format(stats.error_stopped));
}

void MainForm::writeLog(const QString& message)
{
  QString logMessage = tr("[%1] %2").arg(
      QTime::currentTime().toString(Qt::SystemLocaleLongDate)).arg(message);

  ui_.logTextEdit->appendPlainText(logMessage);
  ui_.logTextEdit->moveCursor(QTextCursor::End);
}

void MainForm::updateWidgetsStates(bool ignorePrevServiceState)
{
  ServiceState::State serviceState = service_.state();
  if ((serviceState != prevServiceState_) || ignorePrevServiceState)
  {
    bool serviceStopped = ServiceState::Stopped == serviceState;

    ui_.startButton->setEnabled(serviceStopped);
    ui_.stopButton->setEnabled((ServiceState::Starting == serviceState)
        || (ServiceState::Working == serviceState));
    ui_.terminateButton->setEnabled(!serviceStopped);

    typedef std::vector<OptionWidget>::iterator iterator_type;
    for (iterator_type i = optionsWidgets_.begin(),
        end = optionsWidgets_.end(); i != end; ++i)
    {
      i->get<1>()->setEnabled(serviceStopped);
    }

    ui_.inactivityTimeoutSpinBox->setEnabled(serviceStopped &&
        (Qt::Checked == ui_.inactivityTimeoutCheckBox->checkState()));
    ui_.sockRecvBufferSizeSpinBox->setEnabled(serviceStopped &&
        (Qt::Checked == ui_.sockRecvBufferSizeCheckBox->checkState()));
    ui_.sockSendBufferSizeSpinBox->setEnabled(serviceStopped &&
        (Qt::Checked == ui_.sockSendBufferSizeCheckBox->checkState()));

    QString windowTitle = tr("Qt Echo Server");
    QString statedWindowTitlePart = buildServiceStateWindowTitle(serviceState);
    if (!statedWindowTitlePart.isNull())
    {
      windowTitle = tr("%1 (%2)").arg(windowTitle).arg(statedWindowTitlePart);
    }
    setWindowTitle(windowTitle);
  }
  prevServiceState_ = serviceState;
}

QString MainForm::buildServiceStateWindowTitle(ServiceState::State state)
{
  switch (state)
  {
  case ServiceState::Stopped:
    return QString();
  case ServiceState::Starting:
    return tr("starting");
  case ServiceState::Working:
    return tr("working");
  case ServiceState::Stopping:
    return tr("stopping");
  default:
    return tr("unknown state");
  }
}

} // namespace qt
} // namespace server
} // namespace echo
} // namespace ma
