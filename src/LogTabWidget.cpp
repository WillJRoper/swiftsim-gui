#include "LogTabWidget.h"
#include <QApplication>
#include <QFile>
#include <QFileSystemWatcher>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

// 1) Construct with the log file path and optional parent.
LogTabWidget::LogTabWidget(const QString &filePath, QWidget *parent)
    : QWidget(parent), m_textEdit(new QPlainTextEdit(this)),
      m_watcher(new QFileSystemWatcher(this)), m_filePath(filePath),
      m_font(QApplication::font()) {
  // 2) Configure the text edit: read-only and default application font.
  m_textEdit->setReadOnly(true);
  m_textEdit->setFont(m_font);

  // 3) Layout the text edit to fill this widget without margins.
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_textEdit);
  setLayout(layout);

  // 4) Start watching the log file for changes.
  m_watcher->addPath(m_filePath);
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
          &LogTabWidget::onFileChanged);

  // 5) Initial load of existing log content.
  updateLogView();
}

// 6) Update font size on demand.
void LogTabWidget::setFontSize(int pointSize) {
  m_font.setPointSize(pointSize);
  m_textEdit->setFont(m_font);
}

// 7) Slot triggered when the file changes: schedule a reload, and re-add
// watcher if rotated.
// At the top of LogTabWidget.cpp, ensure you have:
// …other existing includes…

// 7) Slot triggered when the file changes: schedule a delayed reload via a new
// QTimer.
void LogTabWidget::onFileChanged(const QString &path) {
  // 7.1) Create a new single‐shot timer (parented so it auto‐deletes on
  // timeout).
  QTimer *timer = new QTimer(this);
  timer->setSingleShot(true);

  // 7.2) When the timer fires, call updateLogView().
  connect(timer, &QTimer::timeout, this, &LogTabWidget::updateLogView);

  // 7.3) Start the timer with a 100 ms delay.
  timer->start(100);

  // 7.4) QFileSystemWatcher removes watches when files are replaced/rotated;
  //      re-add it so we keep tailing across rotations.
  if (!m_watcher->files().contains(path)) {
    m_watcher->addPath(path);
  }
}

// 8) Read the entire file and display it, preserving scroll position.
void LogTabWidget::updateLogView() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    // Show an error in the text edit if opening fails.
    m_textEdit->setPlainText(tr("Failed to open %1").arg(m_filePath));
    return;
  }

  // Read all content from the file.
  QString content = QTextStream(&file).readAll();
  file.close();

  // Check if we were scrolled to the bottom.
  QScrollBar *sb = m_textEdit->verticalScrollBar();
  bool atBottom = (sb->value() == sb->maximum());

  // Display the new content.
  m_textEdit->setPlainText(content);

  // If at bottom before, keep scrolling to bottom.
  if (atBottom) {
    sb->setValue(sb->maximum());
  }
}
