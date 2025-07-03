#include "StepCounter.h"
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLCDNumber>
#include <QLabel>
#include <QSizePolicy>
#include <QVBoxLayout>

StepCounterWidget::StepCounterWidget(const QString &title, QWidget *parent,
                                     int digitCount)
    : QWidget(parent), m_titleLabel(new QLabel(title, this)),
      m_lcd(new QLCDNumber(this)), m_digitCount(digitCount) {
  // Title: centered, minimal vertical space
  m_titleLabel->setAlignment(Qt::AlignCenter);
  m_titleLabel->setSizePolicy(
      QSizePolicy::Preferred, // horizontal: follow its text
      QSizePolicy::Fixed      // vertical: only as tall as it needs
  );

  // Configure LCD display
  m_lcd->setDigitCount(m_digitCount);
  m_lcd->setSegmentStyle(QLCDNumber::Flat);
  m_lcd->setMode(QLCDNumber::Dec);
  m_lcd->setSizePolicy(QSizePolicy::Expanding, // horizontal: expand
                       QSizePolicy::Expanding  // vertical: expand
  );

  // Glow effect
  auto *glow = new QGraphicsDropShadowEffect(m_lcd);
  glow->setBlurRadius(8);
  glow->setOffset(0, 0);
  glow->setColor(QColor(229, 0, 0, 120));
  m_lcd->setGraphicsEffect(glow);

  // Layouts:
  // 1) Horizontal just for the LCD
  auto *hl = new QHBoxLayout();
  hl->setContentsMargins(0, 0, 0, 0);
  hl->addWidget(m_lcd);

  // 2) Vertical: title (no stretch) + LCD (stretches to fill)
  auto *vl = new QVBoxLayout(this);
  vl->setContentsMargins(0, 60, 0, 0);
  vl->addWidget(m_titleLabel, /*stretch=*/0);
  vl->addLayout(hl, /*stretch=*/1);
  setLayout(vl);

  // Initialize display
  setStep(0);
}

void StepCounterWidget::setStep(int step) {
  // How many digits do we need to display this step?
  int stepDigits = QString::number(step).length();

  // If we need to add another to the display, there shoudl always be a
  // leading zero.
  if (stepDigits + 1 > m_digitCount) {
    expandDigits(stepDigits + 1);
  }

  QString txt = QString("%1").arg(step, m_lcd->digitCount(), 10, QChar('0'));

  m_lcd->display(txt);
}

void StepCounterWidget::expandDigits(int newDigitCount) {
  if (newDigitCount < 4 || newDigitCount > 40) {
    qWarning("StepCounterWidget: Invalid digit count %d; must be [4, 40].",
             newDigitCount);
    if (newDigitCount < 4)
      newDigitCount = 4; // enforce minimum
    else if (newDigitCount > 40)
      newDigitCount = 40; // enforce maximum
  }
  m_digitCount = newDigitCount;
  m_lcd->setDigitCount(newDigitCount);
}
