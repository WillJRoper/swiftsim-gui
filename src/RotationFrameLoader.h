#pragma once

#include "ScaledPixmapLabel.h"
#include "colormaps.h"
#include <QFileSystemWatcher>
#include <QImage>
#include <QLabel>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWidget>
#include <hdf5.h>

class RotationFrameLoader : public QObject {
  Q_OBJECT
public slots:
  /// Kick off (or restart) loading for a given file/dataset/percentile/fps
  void startLoading(const QString &imageDirectory, int fileNumber,
                    const QString &datasetKey, float percentileLow,
                    float percentileHigh, int colormapIdx, int fps,
                    bool keepPercentiles = false);

signals:
  /// Emitted whenever a new rotation frame is ready
  void frameReady(const QImage &img, int fileNumber, int frameIndex,
                  int totalFrames);

private slots:
  void loadNextFrame();
  void nextRotationFrame();

private:
  hid_t m_fileId = -1;
  QString m_imageDirectory;
  QString m_currentDatasetKey;
  int m_currentFileNumber = -1;
  float m_percentileLow = 5.0f;
  float m_percentileHigh = 99.99f;
  float m_lowerValue = 0.0f;
  float m_upperValue = 1.0f;
  int m_nFrames = 0;
  int m_xres = 0;
  int m_yres = 0;
  const uint8_t (*m_cmap)[3] = nullptr;
  size_t m_cmap_size = 0;
  int m_currentRotationFrame = 0;
  QTimer *m_timer = nullptr;

  void computePercentiles();
  void setColormap(int colormapIdx);
};
