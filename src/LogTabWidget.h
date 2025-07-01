#pragma once

#include <QFileSystemWatcher>
#include <QFont>
#include <QString>
#include <QWidget>

class QPlainTextEdit;

// LogTabWidget provides a tailing view of a log file, auto-updated when the
// file changes.
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

private:
  // Reloads the entire log into the text edit.
  void updateLogView();

  QPlainTextEdit *m_textEdit;    // Read-only display area.
  QFileSystemWatcher *m_watcher; // Watches the file for updates.
  QString m_filePath;            // Path to the log file.
  QFont m_font;                  // Current font for display.
};
