#include "LogTabWidget.h"
#include <QApplication>
#include <QFile>
#include <QFontMetrics>
#include <QPlainTextEdit>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextStream>
#include <QTimer>
#include <QVBoxLayout>

LogTabWidget::LogTabWidget(QWidget *parent)
    : QWidget(parent), m_textEdit(new QPlainTextEdit(this)),
      m_filePath("mock_sim/log.txt"),
      m_font(QApplication::font()) // start with app font
{
  m_textEdit->setReadOnly(true);
  m_textEdit->setFont(m_font);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_textEdit);
  setLayout(layout);

  // Initial load of the log file
  updateLogView();
}

void LogTabWidget::setFontSize(int pointSize) {
  m_font.setPointSize(pointSize);
  m_textEdit->setFont(m_font);
  updateLogView(); // reflow lines for new metrics
}

void LogTabWidget::setFilePath(const QString &filePath) {
  m_filePath = filePath;
  updateLogView(); // reload the file
}

void LogTabWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  updateLogView();
}

double LogTabWidget::parseTimeFromLine(const QString &line) {
  // split on whitespace
  const auto parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
  // if cosmology on, time is 5th (index 4), else 3rd (index 2)
  int idx = 1;
  bool ok = false;
  double t = parts.value(idx).toDouble(&ok);
  return ok ? t : 0.0;
}

void LogTabWidget::updateLogView() {
  // 1) Open and read the whole file
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_textEdit->setPlainText(tr("Failed to open %1").arg(m_filePath));
    emit currentTimeChanged(0.0);
    return;
  }
  QString content = QTextStream(&file).readAll();
  file.close();

  // 2) Decide if we should stay at bottom after reloading
  QScrollBar *sb = m_textEdit->verticalScrollBar();
  bool wasAtBottom = (sb->value() == sb->maximum());

  // 3) Replace the text
  m_textEdit->setPlainText(content);

  // 4) If we were following the tail, scroll to bottom again
  if (wasAtBottom) {
    sb->setValue(sb->maximum());
  }

  // 5) Parse the last non-empty line for current time
  // Split on any newline, skip empty
  const QStringList lines =
      content.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
  if (!lines.isEmpty()) {
    double t = parseTimeFromLine(lines.last());
    emit currentTimeChanged(t);
  } else {
    emit currentTimeChanged(0.0);
  }
}
