// RotationFrameLoader.h
#pragma once

#include <QImage>
#include <QObject>
#include <QString>
#include <QTimer>
#include <hdf5.h>
#include <vector>

/**
 * @brief Loads and rotates through frames stored in an HDF5 volume.
 *
 * Emits frameReady() at a fixed fps.  You can:
 *   • startLoading(...) to open a new file (with optional percentile recompute)
 *   • jumpToFile(...)   to switch files under the same rotation clock
 *
 * Internally we cache the HDF5 dataset, file‐space, and mem‐space, and reuse
 * a pair of float buffers + QImage to avoid heap churn.
 */
class RotationFrameLoader : public QObject {
  Q_OBJECT
public:
  explicit RotationFrameLoader(QObject *parent = nullptr);
  ~RotationFrameLoader();

  double minValue() const;
  double maxValue() const;

  /// Set the latest available file number.
  void setLatestFileNumber(int fileNumber) { m_latestFileNumber = fileNumber; }

public slots:
  void startLoading(const QString &imageDirectory, int fileNumber,
                    const QString &datasetKey, int colormapIdx, int fps,
                    bool keepPercentiles = false);

  /**
   * @brief Jump to a new file without altering rotation phase.
   */
  void jumpToFile(int fileNumber, bool keepPercentiles);

signals:
  void frameReady(const QImage &img, int fileNumber, int frameIndex,
                  int totalFrames);
  void percentChanged(int step);
  void ageChanged(long long age); // in Gyrs

private:
  void computePercentiles();
  void setColormap(int colormapIdx);
  void nextRotationFrame();
  void loadNextFrame();

  // HDF5 handles
  hid_t m_fileId = -1;
  hid_t m_dsetId = -1;
  hid_t m_fileSpace = -1;
  hid_t m_memSpace = -1;

  // directory + dataset
  QString m_imageDirectory;
  QString m_currentDatasetKey;
  int m_currentFileNumber = -1;
  int m_latestFileNumber = -1;

  // normalization
  float m_percentileLowDM = 5.0f;       // Dark matter default
  float m_percentileHighDM = 99.99f;    // Dark matter default
  float m_percentileLowGas = 5.0f;      // Gas default
  float m_percentileHighGas = 99.99f;   // Gas default
  float m_percentileLowStars = 5.0f;    // Stars default
  float m_percentileHighStars = 99.99f; // Stars default
  float m_percentileLowTemp = 5.0f;     // Temperature default
  float m_percentileHighTemp = 99.99f;  // Temperature default
  float m_lowerValueDM = 0.0f;          // Dark matter default
  float m_upperValueDM = 1.0f;          // Dark matter default
  float m_lowerValueGas = 0.0f;         // Gas default
  float m_upperValueGas = 1.0f;         // Gas default
  float m_lowerValueStars = 0.0f;       // Stars default
  float m_upperValueStars = 1.0f;       // Stars default
  float m_lowerValueTemp = 0.0f;        // Temperature default
  float m_upperValueTemp = 1.0f;        // Temperature default

  // volume dims
  int m_nFrames = 0, m_xres = 0, m_yres = 0;

  // colormap
  const uint8_t (*m_cmap)[3] = nullptr;
  size_t m_cmap_size = 0;
  int m_colormapIdx = 0;

  // rotation state
  int m_currentRotationFrame = 0;
  int m_fps = 25;
  QTimer *m_timer = nullptr;

  // buffers
  std::vector<float> m_buf;
  QImage m_img;

  // Current step and age values
  int m_currentStep = 0;
  double m_currentAge = 0.0; // in Gyrs
};
