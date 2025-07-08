#include "LogTabWidget.h"
#include <QApplication>
#include <QFile>
#include <QHideEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QShowEvent>
#include <QTextStream>
#include <QVBoxLayout>

LogTabWidget::LogTabWidget(const QString &filePath, QWidget *parent)
    : QWidget(parent), m_textEdit(new QPlainTextEdit(this)),
      m_watcher(new QFileSystemWatcher(this)), m_filePath(filePath),
      m_font(QApplication::font()), m_reloadTimer(new QTimer(this)),
      m_background(":/images/cluster_bkg.png") {
  // Configure text edit
  m_textEdit->setReadOnly(true);
  m_textEdit->setObjectName("logEditor");
  m_textEdit->setFont(m_font);
  m_textEdit->setAttribute(Qt::WA_TranslucentBackground);
  m_textEdit->viewport()->setAttribute(Qt::WA_TranslucentBackground);

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

void LogTabWidget::paintEvent(QPaintEvent *evt) {
  QPainter p(this);

  // 1) draw your background image to fill the whole widget
  p.drawPixmap(rect(), m_background);

  // 2) draw a semi-transparent black rect exactly where the textedit sits
  QRect editRect = m_textEdit->geometry();
  QColor overlayColor(0, 0, 0, 150); // alpha = 120/255 ~47%
  p.fillRect(editRect, overlayColor);

  // 3) now paint children (including your transparent textedit on top)
  QWidget::paintEvent(evt);
}

void LogTabWidget::scrollLogUp(int steps) {
  if (!isVisible())
    return;
  auto *sb = m_textEdit->verticalScrollBar();
  // e.g. each “step” is one singleStep()
  sb->setValue(sb->value() - steps * sb->singleStep());
}

void LogTabWidget::scrollLogDown(int steps) {
  if (!isVisible())
    return;
  auto *sb = m_textEdit->verticalScrollBar();
  sb->setValue(sb->value() + steps * sb->singleStep());
}

void LogTabWidget::showEvent(QShowEvent *ev) {
  QWidget::showEvent(ev);
  if (m_serialHandler) {
    // assume you can reach your serial handler somehow, e.g. via a setter
    connect(m_serialHandler, &SerialHandler::rotatedCCW, this,
            &LogTabWidget::scrollLogUp);
    connect(m_serialHandler, &SerialHandler::rotatedCW, this,
            &LogTabWidget::scrollLogDown);
  }
}

void LogTabWidget::hideEvent(QHideEvent *ev) {
  QWidget::hideEvent(ev);
  if (m_serialHandler) {
    disconnect(m_serialHandler, &SerialHandler::rotatedCCW, this,
               &LogTabWidget::scrollLogUp);
    disconnect(m_serialHandler, &SerialHandler::rotatedCW, this,
               &LogTabWidget::scrollLogDown);
  }
}
