// HomeTabWidget.cpp
#include "HomeTabWidget.h"
#include "ScaledPixmapLabel.h"
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QResizeEvent>
#include <QVBoxLayout>

HomeTabWidget::HomeTabWidget(QWidget *parent)
    : QWidget(parent), m_logoLabel(new ScaledPixmapLabel(this)),
      m_background(":/images/cluster_bkg.png"),
      m_flamingoLabel(new QLabel(this)), m_sussexLabel(new QLabel(this)) {
  // Logo setup (as before)
  QPixmap logo(":/images/swift-logo-white.png");
  if (!logo.isNull())
    m_logoLabel->setPixmapKeepingAspect(logo);
  else
    m_logoLabel->setText(tr("Logo not found"));

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_logoLabel);

  // ——— Credits overlay ———
  m_creditsLabel = new QLabel(
      tr("Credit: Dr Will Roper; The SWIFT Team; The FLAMINGO Team"), this);
  // don’t interfere with mouse events below
  m_creditsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  // bring it to front
  m_creditsLabel->raise();

  qInfo() << "HomeTabWidget: setting up credits label";

  // ——— Flamingo (bottom-left) ———
  m_flamingoOrig = QPixmap(":/images/flamingo-logo.png");
  m_flamingoLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_flamingoLabel->setStyleSheet("background:transparent;");
  m_flamingoLabel->show();
  auto *flamingoOpacity = new QGraphicsOpacityEffect(this);
  flamingoOpacity->setOpacity(0.75);
  m_flamingoLabel->setGraphicsEffect(flamingoOpacity);

  qInfo() << "HomeTabWidget: setting up flamingo label";

  // ——— Sussex (bottom-right) ———
  m_sussexOrig = QPixmap(":/images/sussex-logo.png");
  m_sussexLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_sussexLabel->setStyleSheet("background:transparent;");
  m_sussexLabel->show();
  auto *sussexOpacity = new QGraphicsOpacityEffect(this);
  sussexOpacity->setOpacity(0.75);
  m_sussexLabel->setGraphicsEffect(sussexOpacity);

  qInfo() << "HomeTabWidget: setting up sussex label";

  // Initial scale
  m_scaledBackground = m_background;
}

void HomeTabWidget::resizeEvent(QResizeEvent *event) {
  // Scale background to cover the new size, keeping aspect ratio,
  // expanding so the shortest side fits (i.e. cropping if necessary)
  m_scaledBackground = m_background.scaled(
      event->size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
  QWidget::resizeEvent(event);

  // 2) let layout reposition logo/etc.
  QWidget::resizeEvent(event);
  // 3) now overlay credits at bottom-center
  m_creditsLabel->adjustSize();
  const int x = (width() - m_creditsLabel->width()) / 2;
  const int y =
      (height() - m_creditsLabel->height()) - 10; // 10px bottom margin
  m_creditsLabel->move(x, y);

  {
    int maxH = height() * m_flamingoSizePercent / 100.0;
    QSize tgt(maxH * m_flamingoOrig.width() / double(m_flamingoOrig.height()),
              maxH);
    QPixmap scaled = m_flamingoOrig.scaled(tgt, Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
    m_flamingoLabel->setPixmap(scaled);
    m_flamingoLabel->setFixedSize(scaled.size());
    int x = m_flamingoXMargin;
    int y = height() - scaled.height() - m_flamingoYMargin;
    m_flamingoLabel->move(x, y);
    m_flamingoLabel->raise();
  }

  // ——— Sussex bottom-right (~m_sussexSizePercent% height) ———
  {
    int maxH = height() * m_sussexSizePercent / 100.0;
    QSize tgt(maxH * m_sussexOrig.width() / double(m_sussexOrig.height()),
              maxH);
    QPixmap scaled =
        m_sussexOrig.scaled(tgt, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_sussexLabel->setPixmap(scaled);
    m_sussexLabel->setFixedSize(scaled.size());
    int x = width() - scaled.width() - m_sussexXMargin;
    int y = height() - scaled.height() - m_sussexYMargin;
    m_sussexLabel->move(x, y);
    m_sussexLabel->raise();
  }
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
