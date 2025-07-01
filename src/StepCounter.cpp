// StepCounterWidget.cpp
#include "StepCounter.h"
#include "DataWatcher.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>

StepCounterWidget::StepCounterWidget(QWidget *parent)
    : QWidget(parent), m_lcd(new QLCDNumber(this)) {
  // Reserve, say, 6 digits
  m_lcd->setDigitCount(7);
  m_lcd->setSegmentStyle(QLCDNumber::Flat); // or Outline, Flat, etc.
  m_lcd->setMode(QLCDNumber::Dec);

  // Layout: “Step:” then the LCD
  auto *hl = new QHBoxLayout(this);
  hl->setContentsMargins(0, 0, 0, 0);
  hl->addWidget(m_lcd);
  setLayout(hl);

  auto *glow = new QGraphicsDropShadowEffect(m_lcd);
  glow->setBlurRadius(8);
  glow->setOffset(0, 0);
  glow->setColor(QColor(229, 0, 0, 120)); // faint red glow
  m_lcd->setGraphicsEffect(glow);

  setStep(0); // Initialize to 0
}

void StepCounterWidget::setStep(int step) {
  // Format the step number with leading zeros and display it
  QString txt = QString("%1").arg(step, m_lcd->digitCount(), 10, QChar('0'));
  m_lcd->display(txt);
}
