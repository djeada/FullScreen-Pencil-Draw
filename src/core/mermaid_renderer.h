/**
 * @file mermaid_renderer.h
 * @brief Mermaid diagram renderer using QtWebEngine.
 *
 * Provides high-quality Mermaid diagram rendering by leveraging the Mermaid
 * JavaScript library through a hidden QWebEngineView.
 */
#ifndef MERMAID_RENDERER_H
#define MERMAID_RENDERER_H

#include <QCache>
#include <QColor>
#include <QHash>
#include <QObject>
#include <QPixmap>
#include <QString>

#ifdef HAVE_QT_WEBENGINE
class QWebEngineView;
class QWebEnginePage;
#endif

/**
 * @brief Singleton class for rendering Mermaid diagrams.
 *
 * Uses a hidden QWebEngineView to render Mermaid diagrams via the bundled
 * Mermaid library. Rendered results are cached for performance.
 *
 * When HAVE_QT_WEBENGINE is not defined, this class provides stub
 * implementations that always report unavailable.
 */
class MermaidRenderer : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Get the singleton instance.
   * @return Reference to the MermaidRenderer instance
   */
  static MermaidRenderer &instance();

  /**
   * @brief Check if the renderer is available and ready.
   * @return true if WebEngine is initialized and ready
   */
  bool isAvailable() const;

  /**
   * @brief Request rendering of a Mermaid diagram.
   * @param mermaidCode The Mermaid diagram code
   * @param theme The theme to use (default, dark, forest, neutral)
   * @param requestId Unique identifier for this render request
   *
   * Rendering is asynchronous. When complete, renderComplete() is emitted.
   */
  void render(const QString &mermaidCode, const QString &theme,
              quintptr requestId);

  /**
   * @brief Get a cached render if available.
   * @param mermaidCode The Mermaid diagram code
   * @param theme The theme
   * @return Cached pixmap, or null pixmap if not cached
   */
  QPixmap getCached(const QString &mermaidCode, const QString &theme) const;

  /**
   * @brief Clear the render cache.
   */
  void clearCache();

signals:
  /**
   * @brief Emitted when a render request completes.
   * @param requestId The request identifier passed to render()
   * @param pixmap The rendered diagram as a pixmap (null if failed)
   * @param success true if rendering succeeded
   */
  void renderComplete(quintptr requestId, const QPixmap &pixmap, bool success);

private:
  explicit MermaidRenderer(QObject *parent = nullptr);
  ~MermaidRenderer() override;

  // Non-copyable
  MermaidRenderer(const MermaidRenderer &) = delete;
  MermaidRenderer &operator=(const MermaidRenderer &) = delete;

  /**
   * @brief Generate cache key from render parameters.
   */
  QString cacheKey(const QString &mermaidCode, const QString &theme) const;

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
   * @brief Capture the rendered content as a pixmap.
   */
  void captureResult(quintptr requestId);

  QWebEngineView *webView_;
  bool initialized_;
  bool rendering_;

  // Pending render request
  struct RenderRequest {
    QString mermaidCode;
    QString theme;
    quintptr requestId;
  };
  QList<RenderRequest> pendingRequests_;
  RenderRequest currentRequest_;
#endif

  // LRU cache for rendered pixmaps
  mutable QCache<QString, QPixmap> cache_;
  static constexpr int CACHE_SIZE = 50;
};

#endif // MERMAID_RENDERER_H
