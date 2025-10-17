#include "serialhandler.h"
#include <QDebug>
#include <QSerialPort>

SerialHandler::SerialHandler(const QString &portName, int baudRate,
                             QObject *parent)
    : QObject(parent), m_portName(portName), m_baudRate(baudRate),
      m_reButton(R"(\[SW(\d{2})\])"), m_reCW(R"(\[CW(\d{2})\])"),
      m_reCCW(R"(\[AC(\d{2})\])"), m_rePosition(R"(\[DP(\d{2})\])") {
  // Create and start a dedicated thread
  m_thread = new QThread(this);

  // Move this object to its dedicated thread
  moveToThread(m_thread);

  // When the thread starts, open the serial port
  connect(m_thread, &QThread::started, this, &SerialHandler::start);

  // Ensure the thread stops when this object is destroyed
  connect(this, &SerialHandler::destroyed, m_thread, &QThread::quit);

  // Start the thread (and thus invoke start())
  m_thread->start();
}

SerialHandler::~SerialHandler() {
  // Close the serial port if open
  if (m_serial && m_serial->isOpen()) {
    m_serial->close();
  }

  // Stop and clean up the thread
  m_thread->quit();
  m_thread->wait();
}

void SerialHandler::start() {
  // Create and configure QSerialPort in this thread
  m_serial = new QSerialPort(this);
  m_serial->setPortName(m_portName);
  m_serial->setBaudRate(m_baudRate);

  if (!m_serial->open(QIODevice::ReadOnly)) {
    emit errorOccurred(m_serial->errorString());
    return;
  }

  // Connect readyRead to our handler
  connect(m_serial, &QSerialPort::readyRead, this,
          &SerialHandler::handleReadyRead);
}

void SerialHandler::handleReadyRead() {
  m_buffer.append(m_serial->readAll());

  // Process each complete line
  while (true) {
    int idx = m_buffer.indexOf('\n');
    if (idx < 0)
      break;

    QByteArray line = m_buffer.left(idx).trimmed();
    m_buffer.remove(0, idx + 1);

    QRegularExpressionMatch match;

    if ((match = m_reButton.match(line)).hasMatch()) {
      int id = match.captured(1).toInt();
      if (id >= 1 && id <= 6) {
        emit buttonPressed(id);
      }
    } else if ((match = m_reCW.match(line)).hasMatch()) {
      int steps = match.captured(1).toInt();
      emit rotatedCW(steps);
    } else if ((match = m_reCCW.match(line)).hasMatch()) {
      int steps = match.captured(1).toInt();
      emit rotatedCCW(steps);
    } else if ((match = m_rePosition.match(line)).hasMatch()) {
      int pos = match.captured(1).toInt();
      emit positionChanged(pos);
    }
    // Unrecognized lines are ignored
  }
}
