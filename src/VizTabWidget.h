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
                    float percentileHigh, int colormapIdx, int fps);

signals:
  /// Emitted whenever a new rotation frame is ready
  void frameReady(const QImage &img, int fileNumber, int frameIndex,
                  int totalFrames);

private slots:
  void loadNextFrame();

private:
  hid_t m_fileId = -1;
  QString m_imageDirectory;
  QString m_currentDatasetKey;
  int m_currentFileNumber = -1;
  float m_percentileLow = 0.0f;
  float m_percentileHigh = 100.0f;
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

class VizTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Colormap { Plasma, Magma, Viridis, Jet, Inferno, Greyscale };
  Q_ENUM(Colormap)

  explicit VizTabWidget(QWidget *parent = nullptr);
  ~VizTabWidget();

public slots:
  /// Watch a directory of files named ".../image_<N>.hdf5"
  void watchImageDirectory(const QString &directory);

  /// Manually switch dataset key (1–4)
  void setDatasetKey(const QString &key);

  /// Instruct loader to jump to a given file index (0…latest)
  void setCurrentFileNumber(int index);

  /// Adjust low/high percentile
  void setPercentileRange(float low, float high);

  /// Query the current percentile range
  void percentileRange(float &low, float &high) const;

protected:
  void resizeEvent(QResizeEvent *ev) override;
  void keyPressEvent(QKeyEvent *evt) override;

signals:
  /// Internal: start or restart the loader thread
  void startLoader(const QString &imageDirectory, int fileNumber,
                   const QString &datasetKey, float percentileLow,
                   float percentileHigh, int colormapIdx, int fps);

private slots:
  void handleFrameReady(const QImage &img, int fileNumber, int frameIndex,
                        int totalFrames);
  void onImageDirectoryChanged(const QString &path);

private:
  void scanImageDirectory();

  ScaledPixmapLabel *m_imageLabel;
  QLabel *m_overlayLabel;
  QLabel *m_logoLabel;
  QPixmap m_logoOrig;
  QFileSystemWatcher m_dirWatcher;

  // loader + thread
  RotationFrameLoader *m_loader;
  QThread *m_loaderThread;

  // state
  QString m_imageDirectory;
  QString m_currentDatasetKey = "dark_matter";
  int m_currentFileNumber = -1;
  int m_latestFileNumber = -1;

  int m_nFrames = 0;
  int m_currentRotationFrame = 0;
  int m_fps = 20;

  // percentile normalization
  float m_percentileLow = 1.0f;
  float m_percentileHigh = 99.0f;

  // colormap
  Colormap m_colormap = Colormap::Plasma;
};
