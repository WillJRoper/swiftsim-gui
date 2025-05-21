#include "HomeTabWidget.h"
#include "ScaledPixmapLabel.h"
#include <QVBoxLayout>

HomeTabWidget::HomeTabWidget(QWidget *parent)
    : QWidget(parent), m_logoLabel(new ScaledPixmapLabel(this)) {

  // load via Qt Resource (qrc:/images/swift-logo-white.png)
  QPixmap logo(":/images/swift-logo-white.png");
  if (!logo.isNull())
    m_logoLabel->setPixmapKeepingAspect(logo);
  else
    m_logoLabel->setText(tr("Logo not found"));

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_logoLabel);
}
