#include "LogTabWidget.h"
#include <QApplication>
#include <QFile>
#include <QFontMetrics>
#include <QPlainTextEdit>
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

void LogTabWidget::updateLogView() {
  // Determine how many lines fit
  QFontMetrics fm(m_font);
  int lineHeight = fm.lineSpacing();
  if (lineHeight <= 0)
    return;

  int rows = height() / lineHeight;
  if (rows < 1)
    rows = 1;

  // Read all lines (could optimize for very large files)
  QFile file(m_filePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    m_textEdit->setPlainText(tr("Failed to open %1").arg(m_filePath));
    return;
  }
  QTextStream in(&file);
  QStringList allLines;
  while (!in.atEnd()) {
    allLines << in.readLine();
  }
  file.close();

  // Take only the last 'rows' lines
  int start = qMax(0, allLines.size() - rows);
  QStringList tail = allLines.mid(start, rows);

  m_textEdit->setPlainText(tail.join('\n'));
  // Scroll to bottom
  m_textEdit->verticalScrollBar()->setValue(
      m_textEdit->verticalScrollBar()->maximum());
}
