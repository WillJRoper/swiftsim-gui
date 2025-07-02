// VizTabWidget.h
#pragma once

#include "ScaledPixmapLabel.h"
#include "colormaps.h"
#include <QFileSystemWatcher>
#include <QLabel>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <hdf5.h>
//
/**
 * @class VizTabWidget
 * @brief Displays rotating‐cube frames from an HDF5 file in real time.
 *
 * Uses only Qt types (QString) and the HDF5 C API (hid_t, H5Fopen, etc.).
 * Advances the “rotation frame” on a QTimer, lets you switch among
 * four datasets via keys 1–4, and tracks both a current/latest file index.
 */
class VizTabWidget : public QWidget {
  Q_OBJECT

public:
  enum class Colormap { Plasma, Magma, Viridis, Jet, Inferno, Greyscale };
  Q_ENUM(Colormap)

  explicit VizTabWidget(QWidget *parent = nullptr);
  ~VizTabWidget();

  /// Set the HDF5 file path
  void setBaseHDF5File(const QString &hdf5Path);

  // Get the current file path
  QString currentFilePath();

  // Open the current HDF5 file and query its dimensions
  void openHDF5File();

public slots:
  /// Choose among the built-in colormaps
  void setColormap(Colormap map);

  /// Choose one of: "dark_matter", "gas", "stars", or "gas_temperature"
  void setDatasetKey(const QString &key);

  /// Notify of a new file index (from your external watcher)
  void addNewFile(int fileNumber);

  /// Jump the displayed file index (0…latest)
  void setCurrentFileNumber(int index);

  /**
   * @brief Adjust the low/high percentile (between 0 and 100)
   */
  void setPercentileRange(float low, float high);

  /**
   * @brief Retrieve the current percentile range
   */
  void percentileRange(float &low, float &high) const;

  /// Begin watching this directory for new .hdf5 files
  void watchImageDirectory(const QString &directory);

protected:
  void resizeEvent(QResizeEvent *ev) override;
  void keyPressEvent(QKeyEvent *evt) override;

private slots:
  /// Fired by m_timer; advances rotation slice
  void advanceRotationFrame();

  /// Invoked when the directory’s contents change
  void onImageDirectoryChanged(const QString &path);

private:
  /// Read & display m_currentRotationFrame of m_currentDatasetKey
  void loadCurrentFrame();

  /**
   * @brief Scan all frames in the current dataset, collect every float,
   * sort, and pick out the m_percentileLow and m_percentileHigh cut-points.
   */
  void computePercentiles();

  /// Re-scan the watched folder and detect any newly added frame files
  void scanImageDirectory();

  ScaledPixmapLabel *m_imageLabel; ///< custom label that auto‐scales the pixmap
  QLabel *m_overlayLabel;          ///< displays “File X/Y  Rot A/B  [dataset]”
  QTimer *m_timer;                 ///< drives automatic rotation

  hid_t m_fileId = -1;             ///< HDF5 C handle (H5Fopen), –1 if none
  QString m_currentFilePath;       ///< full path to the current file
  QString m_hdf5Ext = ".hdf5";     ///< HDF5 file extension
  QString m_imageDirectory;        ///< directory to watch for new files
  QFileSystemWatcher m_dirWatcher; ///< watches m_imageDirectory for changes

  QString m_currentDatasetKey = "dark_matter";
  int m_currentFileNumber = -1;   ///< which file we’re showing now (0-based)
  int m_latestFileNumber = -1;    ///< highest file index seen so far
  int m_currentRotationFrame = 0; ///< index into [0…m_nFrames-1]

  int m_nFrames = 0; ///< number of rotation frames per file
  int m_xres = 0;    ///< width of each frame
  int m_yres = 0;    ///< height of each frame
  int m_fps = 25;    ///< how many rotation frames per second

  // for percentile-based normalization (computed once per file)
  float m_percentileLow = 1.0f;   ///< lower percentile (0–100)
  float m_percentileHigh = 99.0f; ///< upper percentile (0–100)
  float m_lowerValue = 0.0f;      ///< value at m_percentileLow
  float m_upperValue = 1.0f;      ///< value at m_percentileHigh

  Colormap m_colormap = Colormap::Plasma;
  const uint8_t (*m_cmap)[3] = nullptr;
  size_t m_cmap_size = 0;
};
