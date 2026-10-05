/**
 * @file katex_renderer.h
 * @brief KaTeX-based LaTeX renderer using QtWebEngine.
 *
 * Provides high-quality LaTeX math rendering by leveraging the KaTeX
 * JavaScript library through a hidden QWebEngineView.
 */
#ifndef KATEX_RENDERER_H
#define KATEX_RENDERER_H

#include <QCache>
#include <QColor>
#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QRect>
#include <QString>

#ifdef HAVE_QT_WEBENGINE
class QWebEngineView;
class QWebEnginePage;
#endif

/**
 * @brief Singleton class for rendering LaTeX expressions using KaTeX.
 *
 * Uses a hidden QWebEngineView to render LaTeX via the bundled KaTeX library.
 * Rendered results are cached for performance.
 *
 * When HAVE_QT_WEBENGINE is not defined, this class provides stub
 * implementations that always report unavailable.
 */
class KatexRenderer : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Get the singleton instance.
   * @return Reference to the KatexRenderer instance
   */
  static KatexRenderer &instance();

  /**
   * @brief Check if the renderer is available and ready.
   * @return true if WebEngine is initialized and ready
   */
  bool isAvailable() const;

  /**
   * @brief Request rendering of a LaTeX expression.
   * @param latex The LaTeX expression (without $ delimiters)
   * @param color Text color for the rendered output
   * @param fontSize Font size in points
   * @param displayMode true for display math, false for inline
   * @param requestId Unique identifier for this render request
   *
   * Rendering is asynchronous. When complete, renderComplete() is emitted.
   */
  void render(const QString &latex, const QColor &color, int fontSize,
              bool displayMode, quintptr requestId);

  /**
   * @brief Get a cached render if available.
   * @param latex The LaTeX expression
   * @param color Text color
   * @param fontSize Font size
   * @param displayMode Display mode flag
   * @return Cached pixmap, or null pixmap if not cached
   */
  QPixmap getCached(const QString &latex, const QColor &color, int fontSize,
                    bool displayMode) const;

  /**
   * @brief Drop a queued render request that is no longer wanted.
   *
   * A request that is already rendering still completes (and is cached),
   * but one waiting in the queue is discarded without being rendered.
   */
  void cancel(quintptr requestId);

  /**
   * @brief Clear the render cache.
   */
  void clearCache();

signals:
  /**
   * @brief Emitted when a render request completes.
   * @param requestId The request identifier passed to render()
   * @param pixmap The rendered LaTeX as a pixmap (null if failed)
   * @param success true if rendering succeeded
   */
  void renderComplete(quintptr requestId, const QPixmap &pixmap, bool success);

private:
  explicit KatexRenderer(QObject *parent = nullptr);
  ~KatexRenderer() override;

  // Non-copyable
  KatexRenderer(const KatexRenderer &) = delete;
  KatexRenderer &operator=(const KatexRenderer &) = delete;

  /**
   * @brief Generate cache key from render parameters.
   */
  QString cacheKey(const QString &latex, const QColor &color, int fontSize,
                   bool displayMode) const;

#ifdef HAVE_QT_WEBENGINE
  /**
   * @brief Initialize the WebEngine view.
   */
  void initializeWebEngine();

  /**
   * @brief Process the next pending render request.
   */
  void processNextRequest();

  /**
   * @brief Size the capture area from the measured content, then wait for a
   * fresh frame before grabbing.
   */
  void captureResult(quintptr requestId, const QString &sizeJson);

  /**
   * @brief Poll until the page has painted the new content, then grab it.
   */
  void pollFrameReady(quintptr requestId, const QRect &area, int attempt);

  /**
   * @brief Cache and emit a capture, then start the next request.
   */
  void finishRequest(quintptr requestId, const QPixmap &pixmap);

  static constexpr int VIEWPORT_WIDTH = 1280;
  static constexpr int VIEWPORT_HEIGHT = 640;

  QWebEngineView *webView_;
  bool initialized_;
  bool rendering_;
  bool shuttingDown_;

  // Pending render request
  struct RenderRequest {
    QString latex;
    QColor color;
    int fontSize;
    bool displayMode;
    quintptr requestId;
  };
  QList<RenderRequest> pendingRequests_;
  RenderRequest currentRequest_;
#endif

  // LRU cache for rendered pixmaps
  mutable QCache<QString, QPixmap> cache_;
  static constexpr int CACHE_SIZE = 100;
};

#endif // KATEX_RENDERER_H
