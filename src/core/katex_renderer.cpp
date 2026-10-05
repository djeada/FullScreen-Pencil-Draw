/**
 * @file katex_renderer.cpp
 * @brief Implementation of KaTeX-based LaTeX renderer.
 */
#include "katex_renderer.h"

#ifdef HAVE_QT_WEBENGINE

#include <QApplication>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QPointer>
#include <QRegularExpression>
#include <QScreen>
#include <QTimer>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>

// Helper to escape string for JavaScript
static QString escapeJsString(const QString &str) {
  QString escaped;
  escaped.reserve(str.size() + 2);
  escaped.append('"');
  for (const QChar c : str) {
    switch (c.unicode()) {
    case '\\':
      escaped.append("\\\\");
      break;
    case '"':
      escaped.append("\\\"");
      break;
    case '\'':
      escaped.append("\\'");
      break;
    case '`':
      escaped.append("\\`");
      break;
    case '\n':
      escaped.append("\\n");
      break;
    case '\r':
      escaped.append("\\r");
      break;
    case '\t':
      escaped.append("\\t");
      break;
    case '\b':
      escaped.append("\\b");
      break;
    case '\f':
      escaped.append("\\f");
      break;
    // U+2028/U+2029 terminate a JS string literal even though they are
    // not \n; other control characters are unsafe unescaped too.
    default:
      if (c.unicode() < 0x20 || c.unicode() == 0x2028 || c.unicode() == 0x2029)
        escaped.append(QString::asprintf("\\u%04x", c.unicode()));
      else
        escaped.append(c);
      break;
    }
  }
  escaped.append('"');
  return escaped;
}

KatexRenderer &KatexRenderer::instance() {
  static KatexRenderer instance;
  return instance;
}

KatexRenderer::KatexRenderer(QObject *parent)
    : QObject(parent), webView_(nullptr), initialized_(false),
      rendering_(false), shuttingDown_(false), cache_(CACHE_SIZE) {}

KatexRenderer::~KatexRenderer() {
  shuttingDown_ = true;
  pendingRequests_.clear();
  rendering_ = false;

  if (webView_) {
    disconnect(webView_, nullptr, this, nullptr);
    webView_->close();
    if (QCoreApplication::instance() && !QCoreApplication::closingDown()) {
      delete webView_;
    }
    webView_ = nullptr;
  }
}

bool KatexRenderer::isAvailable() const { return initialized_; }

void KatexRenderer::initializeWebEngine() {
  if (webView_ || shuttingDown_) {
    return;
  }

  webView_ = new QWebEngineView();

  // Rendered entirely offscreen: the view counts as shown (Chromium needs
  // that to paint) but is never mapped. A real hidden top-level window can't
  // be kept off-screen reliably (window managers clamp negative positions,
  // opacity needs a compositor), so it flashed in the top-left corner.
  webView_->setAttribute(Qt::WA_DontShowOnScreen);
  webView_->setAttribute(Qt::WA_TranslucentBackground);
  webView_->setStyleSheet("background: transparent;");
  // Generous fixed viewport; results are grabbed as a sub-rect, so the view
  // only has to grow (never shrink) for unusually large formulas.
  webView_->resize(VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

  // Configure settings for optimal rendering
  auto *settings = webView_->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                         false);
  settings->setAttribute(QWebEngineSettings::ShowScrollBars, false);

  // Transparent background so only the text is captured
  webView_->page()->setBackgroundColor(Qt::transparent);

  // Load the KaTeX HTML template from resources
  QFile htmlFile(":/katex/katex.html");
  if (htmlFile.open(QIODevice::ReadOnly)) {
    QString html = QString::fromUtf8(htmlFile.readAll());
    webView_->setHtml(html, QUrl("qrc:/katex/"));
    htmlFile.close();
  } else {
    qWarning() << "Failed to load KaTeX HTML template";
  }

  // Show only after the page exists: showing first lets WebEngine's render
  // widget map a real window despite WA_DontShowOnScreen.
  webView_->show();

  // Wait for page to load
  connect(webView_, &QWebEngineView::loadFinished, this, [this](bool ok) {
    initialized_ = ok;
    if (ok && !pendingRequests_.isEmpty()) {
      processNextRequest();
    }
  });
}

void KatexRenderer::processNextRequest() {
  if (pendingRequests_.isEmpty() || rendering_ || shuttingDown_) {
    return;
  }

  currentRequest_ = pendingRequests_.takeFirst();
  rendering_ = true;

  // Apply font size via CSS, render and measure in one round trip.
  // Build the JS in a single .arg() pass: LaTeX regularly contains '%'
  // sequences which would otherwise hijack sequential multi-arg() markers.
  QString js =
      QString("document.getElementById('math').style.fontSize = '%1px';"
              "renderLatex(%2, %3, %4);"
              "getSize();")
          .arg(QString::number(currentRequest_.fontSize),
               escapeJsString(currentRequest_.latex),
               escapeJsString(currentRequest_.color.name()),
               currentRequest_.displayMode ? QStringLiteral("true")
                                           : QStringLiteral("false"));

  QPointer<KatexRenderer> self(this);
  const quintptr requestId = currentRequest_.requestId;
  webView_->page()->runJavaScript(
      js, [self, requestId](const QVariant &result) {
        if (!self || self->shuttingDown_ || !self->webView_) {
          return;
        }
        self->captureResult(requestId, result.toString());
      });
}

QString KatexRenderer::cacheKey(const QString &latex, const QColor &color,
                                int fontSize, bool displayMode) const {
  // Single-pass substitution so '%' inside user LaTeX can't corrupt the key.
  return QString("%1|%2|%3|%4")
      .arg(latex, color.name(), QString::number(fontSize),
           displayMode ? QStringLiteral("d") : QStringLiteral("i"));
}

QPixmap KatexRenderer::getCached(const QString &latex, const QColor &color,
                                 int fontSize, bool displayMode) const {
  QString key = cacheKey(latex, color, fontSize, displayMode);
  if (QPixmap *cached = cache_.object(key)) {
    return *cached;
  }
  return QPixmap();
}

void KatexRenderer::clearCache() { cache_.clear(); }

void KatexRenderer::render(const QString &latex, const QColor &color,
                           int fontSize, bool displayMode, quintptr requestId) {
  if (shuttingDown_) {
    emit renderComplete(requestId, QPixmap(), false);
    return;
  }

  // Check cache first
  QString key = cacheKey(latex, color, fontSize, displayMode);
  if (QPixmap *cached = cache_.object(key)) {
    emit renderComplete(requestId, *cached, true);
    return;
  }

  // Initialize WebEngine if needed (lazy init)
  if (!webView_) {
    initializeWebEngine();
  }

  RenderRequest request{latex, color, fontSize, displayMode, requestId};
  pendingRequests_.append(request);

  if (initialized_ && !rendering_) {
    processNextRequest();
  }
}

void KatexRenderer::cancel(quintptr requestId) {
  if (!requestId) {
    return;
  }
  pendingRequests_.removeIf(
      [requestId](const RenderRequest &r) { return r.requestId == requestId; });
}

void KatexRenderer::finishRequest(quintptr requestId, const QPixmap &pixmap) {
  if (pixmap.isNull() || pixmap.size().isEmpty()) {
    emit renderComplete(requestId, QPixmap(), false);
  } else {
    cache_.insert(cacheKey(currentRequest_.latex, currentRequest_.color,
                           currentRequest_.fontSize,
                           currentRequest_.displayMode),
                  new QPixmap(pixmap));
    emit renderComplete(requestId, pixmap, true);
  }
  rendering_ = false;
  processNextRequest();
}

void KatexRenderer::captureResult(quintptr requestId, const QString &sizeJson) {
  if (shuttingDown_ || !webView_ || !webView_->page()) {
    finishRequest(requestId, QPixmap());
    return;
  }

  // Parse size (simple JSON parsing)
  int width = 100, height = 30;
  static const QRegularExpression widthRe("\"width\"\\s*:\\s*(\\d+)");
  static const QRegularExpression heightRe("\"height\"\\s*:\\s*(\\d+)");
  const auto widthMatch = widthRe.match(sizeJson);
  const auto heightMatch = heightRe.match(sizeJson);
  if (widthMatch.hasMatch()) {
    width = widthMatch.captured(1).toInt();
  }
  if (heightMatch.hasMatch()) {
    height = heightMatch.captured(1).toInt();
  }

  // Add padding and ensure minimum size
  width = qMax(width + 16, 50);
  height = qMax(height + 8, 20);

  // Grow the viewport if the formula doesn't fit (it is never shown, so
  // this is invisible; it just triggers a relayout before the next frame).
  const QSize viewSize = webView_->size();
  if (width > viewSize.width() || height > viewSize.height()) {
    webView_->resize(qMax(width, viewSize.width()),
                     qMax(height, viewSize.height()));
  }

  // Wait until Chromium has actually produced a frame with the new content
  // (two animation frames), instead of guessing with fixed delays.
  QPointer<KatexRenderer> self(this);
  const QRect area(0, 0, width, height);
  webView_->page()->runJavaScript(
      "armFrameReady();", [self, requestId, area](const QVariant &) {
        if (self && !self->shuttingDown_) {
          self->pollFrameReady(requestId, area, 0);
        }
      });
}

void KatexRenderer::pollFrameReady(quintptr requestId, const QRect &area,
                                   int attempt) {
  if (shuttingDown_ || !webView_ || !webView_->page()) {
    finishRequest(requestId, QPixmap());
    return;
  }

  QPointer<KatexRenderer> self(this);
  webView_->page()->runJavaScript(
      "window.frameReady === true;",
      [self, requestId, area, attempt](const QVariant &ready) {
        if (!self || self->shuttingDown_ || !self->webView_) {
          return;
        }
        // Give up waiting after ~1s (e.g. a throttled page) and grab anyway.
        if (!ready.toBool() && attempt < 120) {
          QTimer::singleShot(8, self, [self, requestId, area, attempt]() {
            if (self) {
              self->pollFrameReady(requestId, area, attempt + 1);
            }
          });
          return;
        }
        // One more frame interval for the compositor to hand it to Qt.
        QTimer::singleShot(16, self, [self, requestId, area]() {
          if (!self || self->shuttingDown_ || !self->webView_) {
            return;
          }
          self->finishRequest(requestId, self->webView_->grab(area));
        });
      });
}

#else // !HAVE_QT_WEBENGINE

// Stub implementations when WebEngine is not available

KatexRenderer &KatexRenderer::instance() {
  static KatexRenderer instance;
  return instance;
}

KatexRenderer::KatexRenderer(QObject *parent)
    : QObject(parent), cache_(CACHE_SIZE) {}

KatexRenderer::~KatexRenderer() = default;

bool KatexRenderer::isAvailable() const { return false; }

QString KatexRenderer::cacheKey(const QString &latex, const QColor &color,
                                int fontSize, bool displayMode) const {
  // Single-pass substitution so '%' inside user LaTeX can't corrupt the key.
  return QString("%1|%2|%3|%4")
      .arg(latex, color.name(), QString::number(fontSize),
           displayMode ? QStringLiteral("d") : QStringLiteral("i"));
}

QPixmap KatexRenderer::getCached(const QString & /*latex*/,
                                 const QColor & /*color*/, int /*fontSize*/,
                                 bool /*displayMode*/) const {
  return QPixmap(); // Always empty - no WebEngine
}

void KatexRenderer::clearCache() { cache_.clear(); }

void KatexRenderer::render(const QString & /*latex*/, const QColor & /*color*/,
                           int /*fontSize*/, bool /*displayMode*/,
                           quintptr requestId) {
  // Immediately signal failure - WebEngine not available
  emit renderComplete(requestId, QPixmap(), false);
}

void KatexRenderer::cancel(quintptr /*requestId*/) {}

#endif // HAVE_QT_WEBENGINE
