#pragma once

#include "RotationFrameLoader.h"
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

class VizTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Colormap { Plasma, Magma, Viridis, Jet, Inferno, Greyscale };
  Q_ENUM(Colormap)

  explicit VizTabWidget(QWidget *parent = nullptr);
  ~VizTabWidget();

  /// Set the title of the visualization tab
  void setTitle(const QString &title);

public slots:
  /// Watch a directory of files named ".../image_<N>.hdf5"
  void watchImageDirectory(const QString &directory);

  /// Manually switch dataset key (1–4)
  void setDatasetKey(const QString &key);

  /// Instruct loader to jump to a given file index (0…latest)
  void setCurrentFileNumber(int index);

  /// Increment the current file number and load the next file but without
  /// recomputing percentiles
  void setCurrentFileNumberKnob(int index);

  /// Adjust low/high percentile
  void setPercentileRange(float low, float high);

  /// Query the current percentile range
  void percentileRange(float &low, float &high) const;

  /// Rewind time by a delta
  void rewindTime(int delta);

  /// Fast forward time by a delta
  void fastForwardTime(int delta);

  /**
   * @brief Restart the idle timer (e.g., on any user interaction).
   *        Exposed as a public slot so it can be triggered externally.
   */
  void resetIdleTimer();

protected:
  void resizeEvent(QResizeEvent *ev) override;
  void keyPressEvent(QKeyEvent *evt) override;

signals:
  /// Internal: start or restart the loader thread
  void startLoader(const QString &imageDirectory, int fileNumber,
                   const QString &datasetKey, float percentileLow,
                   float percentileHigh, int colormapIdx, int fps,
                   bool keepPercentiles);

private slots:
  void handleFrameReady(const QImage &img, int fileNumber, int frameIndex,
                        int totalFrames);
  void onImageDirectoryChanged(const QString &path);

  /**
   * @brief Applies accumulated delta after debounce interval.
   */
  void applyPendingDelta();

  /**
   * @brief Resets visualization to the latest frame after idle period.
   */
  void resetToLatest();

private:
  void scanImageDirectory();

  ScaledPixmapLabel *m_imageLabel;
  QLabel *m_logoLabel;
  QPixmap m_logoOrig;
  QLabel *m_flamingoLabel;
  QPixmap m_flamingoOrig;
  QLabel *m_sussexLabel;
  QPixmap m_sussexOrig;
  QLabel *m_esaLabel;
  QPixmap m_esaOrig;
  QLabel *m_titleLabel;
  QFileSystemWatcher m_dirWatcher;

  // Logo offsets for centering
  int m_swiftXMargin = 15;
  int m_swiftYMargin = 15;
  int m_flamingoXMargin = 10;
  int m_flamingoYMargin = 10;
  int m_sussexXMargin = 15;
  int m_sussexYMargin = 15;
  int m_esaXMargin = 15;
  int m_esaYMargin = 15;
  double m_swiftSizePercent = 7;
  double m_flamingoSizePercent = 12;
  double m_sussexSizePercent = 7.5;
  double m_esaSizePercent = 6;

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
  int m_fps = 25;

  // percentile normalization
  float m_percentileLow = 5.0f;
  float m_percentileHigh = 99.99f;

  // colormap
  Colormap m_colormap = Colormap::Plasma;

  // pending time delta for rewinding/fast-forwarding and debouncing timer
  int m_pendingDelta = 0;
  QTimer m_debounceTimer;

  // idle reset timer
  QTimer m_idleTimer;
};
