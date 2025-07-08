#pragma once

#include "SerialHandler.h"
#include <QFileSystemWatcher>
#include <QFont>
#include <QString>
#include <QTimer>
#include <QWidget>

class QPlainTextEdit;

// LogTabWidget provides a tailing view of a log file, auto-updated when the
// file changes. It now reads only appended data (not entire file) and
// debounces updates via a single-shot timer.
class LogTabWidget : public QWidget {
  Q_OBJECT

public:
  // Construct with the path to the log file.
  explicit LogTabWidget(const QString &filePath, QWidget *parent = nullptr);

  // Call this when the user picks a new font size.
  void setFontSize(int pointSize);

  // Set the serial handler to allow scrolling via serial commands.
  void setSerialHandler(SerialHandler *serialHandler) {
    m_serialHandler = serialHandler;
  }

public slots:
  void scrollLogUp(int steps);
  void scrollLogDown(int steps);

private slots:
  // Invoked when the watched file is modified or replaced.
  void onFileChanged(const QString &path);

  // Reloads new data into the text edit.
  void updateLogView();

protected:
  void paintEvent(QPaintEvent *event) override;
  void showEvent(QShowEvent *ev) override;
  void hideEvent(QHideEvent *ev) override;

private:
  QPlainTextEdit *m_textEdit;    // Read-only display area.
  QFileSystemWatcher *m_watcher; // Watches the file for updates.
  QString m_filePath;            // Path to the log file.
  QFont m_font;                  // Current font for display.

  QTimer *m_reloadTimer;     // Single-shot debounce timer
  qint64 m_lastPosition = 0; // Last read position in file
  QPixmap m_background;

  SerialHandler *m_serialHandler = nullptr; // Serial handler for scrolling
};
