#pragma once

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

private slots:
  // Invoked when the watched file is modified or replaced.
  void onFileChanged(const QString &path);

  // Reloads new data into the text edit.
  void updateLogView();

private:
  QPlainTextEdit *m_textEdit;    // Read-only display area.
  QFileSystemWatcher *m_watcher; // Watches the file for updates.
  QString m_filePath;            // Path to the log file.
  QFont m_font;                  // Current font for display.

  QTimer *m_reloadTimer;     // Single-shot debounce timer
  qint64 m_lastPosition = 0; // Last read position in file
};
