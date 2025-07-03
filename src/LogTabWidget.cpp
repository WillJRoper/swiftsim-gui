#include "LogTabWidget.h"
#include <QApplication>
#include <QFile>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextStream>
#include <QVBoxLayout>

LogTabWidget::LogTabWidget(const QString &filePath, QWidget *parent)
    : QWidget(parent), m_textEdit(new QPlainTextEdit(this)),
      m_watcher(new QFileSystemWatcher(this)), m_filePath(filePath),
      m_font(QApplication::font()), m_reloadTimer(new QTimer(this)) {
  // Configure text edit
  m_textEdit->setReadOnly(true);
  m_textEdit->setObjectName("logEditor");
  m_textEdit->setFont(m_font);

  // Layout
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_textEdit);
  setLayout(layout);

  // Set up debounce timer (1000 ms)
  m_reloadTimer->setSingleShot(true);
  m_reloadTimer->setInterval(1000);
  connect(m_reloadTimer, &QTimer::timeout, this, &LogTabWidget::updateLogView);

  // Watch file
  m_watcher->addPath(m_filePath);
  connect(m_watcher, &QFileSystemWatcher::fileChanged, this,
          &LogTabWidget::onFileChanged);

  // Initial load
  updateLogView();
}

void LogTabWidget::setFontSize(int pointSize) {
  m_font.setPointSize(pointSize);
  m_textEdit->setFont(m_font);
}

void LogTabWidget::onFileChanged(const QString &path) {
  // Re-add path if rotated/replaced
  if (!m_watcher->files().contains(path)) {
    m_watcher->addPath(path);
    // Reset position so we reload whole file
    m_lastPosition = 0;
  }

  // Debounce further changes
  if (!m_reloadTimer->isActive()) {
    m_reloadTimer->start();
  }
}

void LogTabWidget::updateLogView() {
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_textEdit->setPlainText(tr("Failed to open %1").arg(m_filePath));
    return;
  }

  // Check file size and handle truncation
  qint64 fileSize = file.size();
  if (fileSize < m_lastPosition) {
    // File was truncated or rotated: clear and reset
    m_textEdit->clear();
    m_lastPosition = 0;
  }

  // Seek to last read position
  file.seek(m_lastPosition);

  // Read appended data
  QTextStream in(&file);
  QString newText = in.readAll();
  m_lastPosition = file.pos();
  file.close();

  // Append or initial load
  QScrollBar *sb = m_textEdit->verticalScrollBar();
  bool atBottom = (sb->value() == sb->maximum());

  if (m_lastPosition == (qint64)newText.size()) {
    // First load (empty before): set full text
    m_textEdit->setPlainText(newText);
  } else if (!newText.isEmpty()) {
    m_textEdit->moveCursor(QTextCursor::End);
    m_textEdit->insertPlainText(newText);
  }

  // Maintain scroll position
  if (atBottom) {
    sb->setValue(sb->maximum());
  }
}
