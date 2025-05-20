#include "HomeTabWidget.h"
#include <QDir>
#include <QLabel>
#include <QResizeEvent>
#include <QVBoxLayout>

HomeTabWidget::HomeTabWidget(QWidget *parent)
    : QWidget(parent), m_logoLabel(new QLabel(this)) {
  // 1) Load the original pixmap
  const QString path = QDir(QCoreApplication::applicationDirPath())
                           .filePath("resources/images/swift-logo-white.png");
  m_logoPixmap = QPixmap(path);
  if (m_logoPixmap.isNull()) {
    // fallback: show text if the image failed to load
    m_logoLabel->setText(tr("Logo not found"));
    m_logoLabel->setAlignment(Qt::AlignCenter);
  } else {
    // center the label; we'll set its pixmap in resizeEvent
    m_logoLabel->setAlignment(Qt::AlignCenter);
  }

  // 2) A simple layout that makes the label fill the widget
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_logoLabel);
  setLayout(layout);
}

void HomeTabWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  if (!m_logoPixmap.isNull()) {
    // scale the original pixmap to the new size, keeping aspect ratio
    QSize targetSize = event->size();
    QPixmap scaled = m_logoPixmap.scaled(targetSize, Qt::KeepAspectRatio,
                                         Qt::SmoothTransformation);
    m_logoLabel->setPixmap(scaled);
  }
}
