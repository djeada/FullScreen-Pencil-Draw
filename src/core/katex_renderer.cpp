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
#include <QRegularExpression>
#include <QScreen>
#include <QTimer>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>

// Helper to escape string for JavaScript
static QString escapeJsString(const QString &str) {
  QString escaped = str;
  escaped.replace('\\', "\\\\");
  escaped.replace('\'', "\\'");
  escaped.replace('\"', "\\\"");
  escaped.replace('\n', "\\n");
  escaped.replace('\r', "\\r");
  return "\"" + escaped + "\"";
}

KatexRenderer &KatexRenderer::instance() {
  static KatexRenderer instance;
  return instance;
}

KatexRenderer::KatexRenderer(QObject *parent)
    : QObject(parent), webView_(nullptr), initialized_(false),
      rendering_(false), cache_(CACHE_SIZE) {}

KatexRenderer::~KatexRenderer() {
  if (webView_) {
    webView_->close();
    delete webView_;
    webView_ = nullptr;
  }
}

bool KatexRenderer::isAvailable() const { return initialized_; }

void KatexRenderer::initializeWebEngine() {
  if (webView_) {
    return;
  }

  webView_ = new QWebEngineView();
  
  // Tool window, no taskbar, positioned off-screen
  webView_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowDoesNotAcceptFocus);
  webView_->setAttribute(Qt::WA_TranslucentBackground);
  webView_->setAttribute(Qt::WA_ShowWithoutActivating);
  webView_->setStyleSheet("background: transparent;");
  webView_->setFixedSize(400, 200);
  webView_->move(-2000, -2000);  // Off-screen but not too far for some WMs
  webView_->setWindowOpacity(0.0);  // Invisible even if somehow on screen
  
  // Show without activating - required for rendering to work
  webView_->show();

  // Configure settings for optimal rendering
  auto *settings = webView_->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
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

  // Wait for page to load
  connect(webView_, &QWebEngineView::loadFinished, this,
          [this](bool ok) {
            qDebug() << "KaTeX page load finished:" << ok;
            initialized_ = ok;
            if (ok && !pendingRequests_.isEmpty()) {
              processNextRequest();
            }
          });
}

void KatexRenderer::processNextRequest() {
  if (pendingRequests_.isEmpty() || rendering_) {
    return;
  }
  
  currentRequest_ = pendingRequests_.takeFirst();
  rendering_ = true;
  
  qDebug() << "Processing LaTeX:" << currentRequest_.latex << "color:" << currentRequest_.color.name();
  
  // Apply font size via CSS and render
  QString js = QString(
      "document.getElementById('math').style.fontSize = '%1px';"
      "renderLatex(%2, %3, %4);")
      .arg(currentRequest_.fontSize)
      .arg(escapeJsString(currentRequest_.latex))
      .arg(escapeJsString(currentRequest_.color.name()))
      .arg(currentRequest_.displayMode ? "true" : "false");

  webView_->page()->runJavaScript(js, [this](const QVariant &result) {
    qDebug() << "JavaScript result:" << result;
    // Give KaTeX time to render, then capture
    QTimer::singleShot(200, this, [this]() {
      captureResult(currentRequest_.requestId);
    });
  });
}

QString KatexRenderer::cacheKey(const QString &latex, const QColor &color,
                                 int fontSize, bool displayMode) const {
  return QString("%1|%2|%3|%4")
      .arg(latex)
      .arg(color.name())
      .arg(fontSize)
      .arg(displayMode ? "d" : "i");
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

void KatexRenderer::captureResult(quintptr requestId) {
  if (!webView_ || !webView_->page()) {
    qDebug() << "captureResult: no webView or page";
    emit renderComplete(requestId, QPixmap(), false);
    rendering_ = false;
    processNextRequest();
    return;
  }

  // Get the size of the rendered math element
  webView_->page()->runJavaScript("getSize();", [this, requestId](const QVariant &result) {
    QString sizeJson = result.toString();
    qDebug() << "getSize result:" << sizeJson;
    
    // Parse size (simple JSON parsing)
    int width = 100, height = 30;
    QRegularExpression widthRe("\"width\"\\s*:\\s*(\\d+)");
    QRegularExpression heightRe("\"height\"\\s*:\\s*(\\d+)");
    
    auto widthMatch = widthRe.match(sizeJson);
    auto heightMatch = heightRe.match(sizeJson);
    
    if (widthMatch.hasMatch()) {
      width = widthMatch.captured(1).toInt();
    }
    if (heightMatch.hasMatch()) {
      height = heightMatch.captured(1).toInt();
    }
    
    // Add padding and ensure minimum size
    width = qMax(width + 16, 50);
    height = qMax(height + 8, 20);
    
    qDebug() << "Resizing webView to:" << width << "x" << height;
    
    // Resize view to match content, keep off-screen
    webView_->setFixedSize(width, height);
    webView_->move(-2000, -2000);
    
    // Grab after a short delay for content to render
    QTimer::singleShot(50, this, [this, requestId, width, height]() {
      QPixmap pixmap = webView_->grab();
      
      qDebug() << "Grabbed pixmap:" << pixmap.size() << "isNull:" << pixmap.isNull();
      
      if (pixmap.isNull() || pixmap.size().isEmpty()) {
        qDebug() << "Pixmap capture failed";
        emit renderComplete(requestId, QPixmap(), false);
      } else {
        // Cache the result
        QString key = cacheKey(currentRequest_.latex, currentRequest_.color,
                               currentRequest_.fontSize, currentRequest_.displayMode);
        cache_.insert(key, new QPixmap(pixmap));
        
        qDebug() << "Emitting renderComplete with pixmap";
        emit renderComplete(requestId, pixmap, true);
      }
      
      rendering_ = false;
      processNextRequest();
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
  return QString("%1|%2|%3|%4")
      .arg(latex)
      .arg(color.name())
      .arg(fontSize)
      .arg(displayMode ? "d" : "i");
}

QPixmap KatexRenderer::getCached(const QString & /*latex*/, const QColor & /*color*/,
                                  int /*fontSize*/, bool /*displayMode*/) const {
  return QPixmap(); // Always empty - no WebEngine
}

void KatexRenderer::clearCache() { cache_.clear(); }

void KatexRenderer::render(const QString & /*latex*/, const QColor & /*color*/,
                            int /*fontSize*/, bool /*displayMode*/, quintptr requestId) {
  // Immediately signal failure - WebEngine not available
  emit renderComplete(requestId, QPixmap(), false);
}

#endif // HAVE_QT_WEBENGINE
