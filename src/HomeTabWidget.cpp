// HomeTabWidget.cpp
#include "HomeTabWidget.h"
#include "ScaledPixmapLabel.h"
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>

HomeTabWidget::HomeTabWidget(QWidget *parent)
    : QWidget(parent), m_logoLabel(new ScaledPixmapLabel(this)),
      m_background(":/images/cluster_bkg.png") {
  // Logo setup (as before)
  QPixmap logo(":/images/swift-logo-white.png");
  if (!logo.isNull())
    m_logoLabel->setPixmapKeepingAspect(logo);
  else
    m_logoLabel->setText(tr("Logo not found"));

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_logoLabel);

  // Initial scale
  m_scaledBackground = m_background;
}

void HomeTabWidget::resizeEvent(QResizeEvent *event) {
  // Scale background to cover the new size, keeping aspect ratio,
  // expanding so the shortest side fits (i.e. cropping if necessary)
  m_scaledBackground = m_background.scaled(
      event->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
  QWidget::resizeEvent(event);
}

void HomeTabWidget::paintEvent(QPaintEvent *event) {
  QPainter p(this);

  if (!m_scaledBackground.isNull()) {
    // center the pixmap
    QPoint topLeft{(width() - m_scaledBackground.width()) / 2,
                   (height() - m_scaledBackground.height()) / 2};
    p.drawPixmap(topLeft, m_scaledBackground);
  }

  // draw a semi-transparent black overlay
  constexpr int overlayAlpha = 150; // tweak 0–255
  p.fillRect(rect(), QColor(0, 0, 0, overlayAlpha));

  // now paint child widgets
  QWidget::paintEvent(event);
}
