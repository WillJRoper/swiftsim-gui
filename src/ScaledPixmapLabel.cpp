#include "ScaledPixmapLabel.h"
#include <QPaintEvent>
#include <QPainter>

ScaledPixmapLabel::ScaledPixmapLabel(QWidget *parent) : QLabel(parent) {
  // Let layouts expand this to fill available space
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  setAlignment(Qt::AlignCenter);
  setAttribute(Qt::WA_TranslucentBackground);
}

void ScaledPixmapLabel::setPixmapKeepingAspect(const QPixmap &pixmap) {
  m_original = pixmap;
  update(); // schedule a repaint
}

void ScaledPixmapLabel::paintEvent(QPaintEvent *ev) {
  if (m_original.isNull()) {
    QLabel::paintEvent(ev);
    return;
  }

  QPainter painter(this);

  // perform the scale in *widget* coords only
  QPixmap scaled = m_original.scaled(
      size(), Qt::KeepAspectRatio,
      Qt::SmoothTransformation // or FastTransformation if you prefer
  );

  // center it in logical pixels
  QPoint c = rect().center() - QPoint(scaled.width() / 2, scaled.height() / 2);
  painter.drawPixmap(c, scaled);
}
