#include "VizTabWidget.h"
#include "SimulationController.h"
#include "VizTabWidget.h"
#include "colormaps.h"
#include <QDir>
#include <QFileInfo>
#include <QFileInfoList>
#include <QImage>
#include <QLabel>
#include <QPalette>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QtMath>

VizTabWidget::VizTabWidget(SimulationController *simCtrl, QWidget *parent)
    : QWidget(parent), m_imageLabel(new QLabel(this)) // ← Create label first
      ,
      m_overlayLabel(new QLabel(this)) {
  // -- Setup the image label --
  m_imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  m_imageLabel->setAlignment(Qt::AlignCenter);

  // -- Layout: now add the fully-constructed label --
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->addWidget(m_imageLabel);
  setLayout(layout);

  // -- Overlay label setup --
  m_overlayLabel->setStyleSheet(R"(
      QLabel {
        background: rgba(0,0,0,128);
        color: #E50000;
        padding: 2px 4px;
        border-radius: 3px;
        font: 12pt "Hack Nerd Font Mono";
      }
    )");
  m_overlayLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
  m_overlayLabel->move(10, 10);
  m_overlayLabel->show();

  // -- File system watcher --
  connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this,
          &VizTabWidget::updateImage);

  // -- Respond to simulationDir changes --
  connect(simCtrl, &SimulationController::simulationDirectoryChanged, this,
          &VizTabWidget::setSimulationDirectory);

  // Initialize
  setSimulationDirectory(simCtrl->simulationDirectory());

  // Find all sub-directories under <simDir>/images
  QDir imagesRoot(simCtrl->simulationDirectory() + "/images");
  QStringList entries =
      imagesRoot.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

  // Set the image directory to the first found, or default to "images"
  setImageDirectory(entries.isEmpty() ? "images" : "images/" + entries.first());
}

void VizTabWidget::setSimulationDirectory(const QString &simDir) {
  m_simulationDir = simDir;
}

void VizTabWidget::setImageDirectory(const QString &imageDir) {
  m_imagesDir = QDir(m_simulationDir).filePath(imageDir);
  m_watcher.removePaths(m_watcher.directories());
  if (QDir(m_imagesDir).exists())
    m_watcher.addPath(m_imagesDir);

  updateImage();
}

void VizTabWidget::loadLatestImage() {
  QDir dir(m_imagesDir);
  if (!dir.exists()) {
    m_originalPixmap = QPixmap();
    m_currentImagePath.clear();
    return;
  }

  QStringList patterns = {"*.png", "*.jpg", "*.jpeg", "*.bmp"};
  QFileInfoList infos = dir.entryInfoList(patterns, QDir::Files, QDir::Time);

  if (infos.isEmpty()) {
    m_originalPixmap = QPixmap();
    m_currentImagePath.clear();
  } else {
    m_currentImagePath = infos.first().absoluteFilePath();
    m_originalPixmap.load(m_currentImagePath);
  }
}

void VizTabWidget::resizeEvent(QResizeEvent *ev) {
  QWidget::resizeEvent(ev);
  // Re-apply overlay position if needed
  m_overlayLabel->move(10, 10);
  // Rescale image
  updateImage();
}

void VizTabWidget::setScaleMode(ScaleMode mode) {
  if (m_scaleMode == mode)
    return;
  m_scaleMode = mode;
  updateImage();
}

void VizTabWidget::updateImage() {
  loadLatestImage();

  if (m_originalPixmap.isNull()) {
    m_imageLabel->clear();
    m_overlayLabel->setText(QStringLiteral("No Image"));
    m_overlayLabel->adjustSize();
    return;
  }

  // 1) Overlay the filename
  QFileInfo fi(m_currentImagePath);
  m_overlayLabel->setText(fi.fileName());
  m_overlayLabel->adjustSize();
  m_overlayLabel->raise();

  // 2) Resample the raw pixmap once, keeping aspect ratio
  //    and using SmoothTransformation for best quality.
  //    We compute the target in *device* pixels so it uses full DPI.
  qreal dpr = devicePixelRatioF();
  QSize target = m_imageLabel->size() * dpr;

  QPixmap scaled =
      m_originalPixmap.scaled(target,
                              Qt::KeepAspectRatio,     // letterbox if necessary
                              Qt::SmoothTransformation // high-quality filter
      );
  scaled.setDevicePixelRatio(dpr);

  // 3) Display it
  m_imageLabel->setPixmap(scaled);
}
