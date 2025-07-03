// VizTabWidget.h
#pragma once

#include "ScaledPixmapLabel.h"
#include "colormaps.h"

#include <QFileSystemWatcher>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QTimer>
#include <QWidget>
#include <hdf5.h>

/**
 * @class VizTabWidget
 * @brief Displays rotating‐cube frames from an HDF5 file in real time,
 *        using GPU‐accelerated colormapping via OpenGL.
 *
 * - Delegates all HDF5 I/O and percentile computation to RotationFrameLoader.
 * - Uploads each float frame to a GL texture and renders with a shader.
 * - Supports four datasets, key-switching (1–4), percentile range tuning,
 *   directory watching for new files, and a watermark logo.
 */
class VizTabWidget : public QOpenGLWidget, protected QOpenGLFunctions {
  Q_OBJECT

public:
  enum class Colormap { Plasma, Magma, Viridis, Jet, Inferno, Greyscale };
  Q_ENUM(Colormap)

  explicit VizTabWidget(QWidget *parent = nullptr);
  ~VizTabWidget() override;

  /// Begin watching this directory for new image_N.hdf5 files
  void watchImageDirectory(const QString &directory);

public slots:
  /// Set active dataset key ("dark_matter", "gas", "stars", "gas_temperature")
  void setDatasetKey(const QString &key);

  /// Adjust the low/high percentile (0–100)
  void setPercentileRange(float low, float high);

  /// Retrieve the current percentile range
  void percentileRange(float &low, float &high) const;

protected:
  void initializeGL() override;
  void resizeGL(int w, int h) override;
  void paintGL() override;
  void resizeEvent(QResizeEvent *ev) override;
  void keyPressEvent(QKeyEvent *evt) override;

private slots:
  /// Advance to the next rotation frame
  void advanceRotationFrame();

  /// Directory watcher callback
  void onImageDirectoryChanged(const QString &);

private:
  /// Load & display the current frame (via GPU upload + draw)
  void loadCurrentFrame();

  /// Scan directory for new files and update file index
  void scanImageDirectory();

  /// Switch colormap arrays
  void setColormap(Colormap map);

  /// GPU shader setup
  void initShader();

  QLabel *m_overlayLabel; ///< top‐left overlay text
  QLabel *m_logoLabel;    ///< bottom‐right watermark
  QTimer *m_timer;        ///< drives frame advance
  QFileSystemWatcher m_dirWatcher;
  QString m_imageDirectory;     ///< watched folder
  int m_currentFileNumber = -1; ///< index of current file
  int m_latestFileNumber = -1;  ///< highest index seen
  int m_currentRotationFrame = 0;
  int m_fps = 20;

  // Percentile settings
  float m_percentileLow = 1.0f;
  float m_percentileHigh = 99.0f;

  // Colormap data
  Colormap m_colormap = Colormap::Plasma;
  const uint8_t (*m_cmap)[3] = nullptr;
  size_t m_cmap_size = 0;

  // OpenGL resources
  GLuint m_texture = 0;
  GLuint m_program = 0;
  int m_locCmap = -1;
  int m_locRange = -1;
  int m_texUnit = 0;

  /// Helper to load raw frames from HDF5 & compute percentiles
  struct RotationFrameLoader {
    RotationFrameLoader();
    ~RotationFrameLoader();

    /// Open a new HDF5 file, query dims, compute percentile cutoffs
    bool openFile(const QString &path, const QString &datasetKey, float lowPct,
                  float highPct);

    /// Read a single rotation slice into the internal float buffer
    bool fetchFrame(int rotationIndex);

    float *data() { return m_buf.data(); }
    int width() { return m_xres; }
    int height() { return m_yres; }

  private:
    hid_t m_fileId = -1;
    QString m_datasetKey;
    int m_nFrames = 0, m_xres = 0, m_yres = 0;
    float m_lowerValue = 0.0f, m_upperValue = 1.0f;
    std::vector<float> m_buf;

    void computePercentiles(float lowPct, float highPct);
  } m_loader;
};
