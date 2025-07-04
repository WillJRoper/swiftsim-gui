#pragma once

#include <QObject>
#include <QRegularExpression>
#include <QSerialPort>
#include <QThread>

/**
 * @brief SerialHandler handles asynchronous communication with
 *        a Pico 2W encoder/button interface over a serial port.
 *
 * On construction, it opens the given serial port (in its own QThread)
 * and emits signals when buttons are pressed, the dial is rotated,
 * or its absolute position is updated.
 */
class SerialHandler : public QObject {
  Q_OBJECT
public:
  /**
   * @brief Constructs and starts the SerialHandler.
   * @param portName The serial device path (e.g. "/dev/cu.usbmodem3301").
   * @param baudRate Baud rate for serial communication (default 115200).
   * @param parent   Parent QObject (default nullptr).
   *
   * The handler is moved to a dedicated QThread, which is started
   * immediately, and the serial port is opened automatically.
   */
  explicit SerialHandler(const QString &portName, int baudRate = 115200,
                         QObject *parent = nullptr);

  /**
   * @brief Destructor closes the serial port and stops the thread.
   */
  ~SerialHandler() override;

signals:
  /**
   * @brief Emitted when button SW## is pressed.
   * @param id Numeric button ID (01–06).
   */
  void buttonPressed(int id);

  /**
   * @brief Emitted when the encoder is rotated clockwise.
   * @param steps Number of steps rotated.
   */
  void rotatedCW(int steps);

  /**
   * @brief Emitted when the encoder is rotated anti-clockwise.
   * @param steps Number of steps rotated.
   */
  void rotatedCCW(int steps);

  /**
   * @brief Emitted when an absolute position update is received.
   * @param pos Position value (0–63).
   */
  void positionChanged(int pos);

  /**
   * @brief Emitted on serial error.
   * @param msg Error message.
   */
  void errorOccurred(const QString &msg);

private slots:
  /**
   * @brief Initializes and opens the serial port (runs in its own thread).
   */
  void start();

  /**
   * @brief Reads incoming data, buffers lines, and parses commands.
   */
  void handleReadyRead();

private:
  QString m_portName;             ///< Serial device path.
  int m_baudRate;                 ///< Baud rate.
  QSerialPort *m_serial{nullptr}; ///< QSerialPort instance (owned).
  QThread *m_thread{nullptr};     ///< Worker thread.
  QByteArray m_buffer;            ///< Buffer for incoming bytes.

  const QRegularExpression m_reButton;   ///< Pattern \[SW(\d{2})\]
  const QRegularExpression m_reCW;       ///< Pattern \[CW(\d{2})\]
  const QRegularExpression m_reCCW;      ///< Pattern \[AC(\d{2})\]
  const QRegularExpression m_rePosition; ///< Pattern \[DP(\d{2})\]
};
