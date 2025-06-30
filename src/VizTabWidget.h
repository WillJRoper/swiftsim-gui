#pragma once

#include "ScaledPixmapLabel.h"
#include <QFileSystemWatcher>
#include <QWidget>

class SimulationController;

class VizTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class ScaleMode { Linear, Logarithmic, AutoMinMax };
  Q_ENUM(ScaleMode)

  explicit VizTabWidget(SimulationController *simCtrl,
                        QWidget *parent = nullptr);

public slots:
  void setSimulationDirectory(const QString &simDir);
  void setImageDirectory(const QString &imageDir);
  void setScaleMode(ScaleMode mode);

private slots:
  void updateImage();

protected:
  void resizeEvent(QResizeEvent *ev) override;

private:
  void loadLatestImage();
  QImage applyScale(const QImage &src);

  QLabel *m_overlayLabel;
  QLabel *m_imageLabel;
  QPixmap m_originalPixmap;
  QFileSystemWatcher m_watcher;
  QString m_simulationDir;
  QString m_imagesDir;
  QString m_currentImagePath;

  ScaleMode m_scaleMode = ScaleMode::Linear;
};
