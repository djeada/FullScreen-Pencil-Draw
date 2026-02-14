/**
 * @file mermaid_renderer.cpp
 * @brief Implementation of Mermaid diagram renderer.
 */
#include "mermaid_renderer.h"

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
  QString escaped = str;
  escaped.replace('\\', "\\\\");
  escaped.replace('\'', "\\'");
  escaped.replace('\"', "\\\"");
  escaped.replace('\n', "\\n");
  escaped.replace('\r', "\\r");
  escaped.replace('\t', "\\t");
  escaped.replace(
      '`', "\\`"); // Escape backticks to prevent template literal injection
  return "\"" + escaped + "\"";
}

MermaidRenderer &MermaidRenderer::instance() {
  static MermaidRenderer instance;
  return instance;
}

MermaidRenderer::MermaidRenderer(QObject *parent)
    : QObject(parent), webView_(nullptr), initialized_(false),
      rendering_(false), shuttingDown_(false), cache_(CACHE_SIZE) {}

MermaidRenderer::~MermaidRenderer() {
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

bool MermaidRenderer::isAvailable() const { return initialized_; }

void MermaidRenderer::initializeWebEngine() {
  if (webView_ || shuttingDown_) {
    return;
  }

  webView_ = new QWebEngineView();

  // Tool window, no taskbar, positioned off-screen
  webView_->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                           Qt::WindowDoesNotAcceptFocus);
  webView_->setAttribute(Qt::WA_TranslucentBackground);
  webView_->setAttribute(Qt::WA_ShowWithoutActivating);
  webView_->setStyleSheet("background: transparent;");
  webView_->setFixedSize(800, 600);
  webView_->move(-2000, -2000);    // Off-screen but not too far for some WMs
  webView_->setWindowOpacity(0.0); // Invisible even if somehow on screen

  // Show without activating - required for rendering to work
  webView_->show();

  // Configure settings for optimal rendering
  auto *settings = webView_->settings();
  settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls,
                         true);
  settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls,
                         false);
  settings->setAttribute(QWebEngineSettings::ShowScrollBars, false);

  // White background for diagrams
  webView_->page()->setBackgroundColor(Qt::white);

  // Load the Mermaid HTML template from resources
  QFile htmlFile(":/mermaid/mermaid.html");
  if (htmlFile.open(QIODevice::ReadOnly)) {
    QString html = QString::fromUtf8(htmlFile.readAll());
    webView_->setHtml(html, QUrl("qrc:/mermaid/"));
    htmlFile.close();
  } else {
    qWarning() << "Failed to load Mermaid HTML template";
  }

  // Wait for page to load (use Qt::UniqueConnection to prevent duplicates)
  connect(
      webView_, &QWebEngineView::loadFinished, this,
      [this](bool ok) {
        qDebug() << "Mermaid page load finished:" << ok;
        initialized_ = ok;
        if (ok && !pendingRequests_.isEmpty()) {
          processNextRequest();
        }
      },
      Qt::UniqueConnection);
}

void MermaidRenderer::processNextRequest() {
  if (pendingRequests_.isEmpty() || rendering_ || shuttingDown_) {
    return;
  }

  currentRequest_ = pendingRequests_.takeFirst();
  rendering_ = true;

  qDebug() << "Processing Mermaid diagram:"
           << currentRequest_.mermaidCode.left(50)
           << "theme:" << currentRequest_.theme;

  // Render the diagram
  QString js = QString("renderMermaid(%1, %2);")
                   .arg(escapeJsString(currentRequest_.mermaidCode))
                   .arg(escapeJsString(currentRequest_.theme));

  QPointer<MermaidRenderer> self(this);
  webView_->page()->runJavaScript(js, [self](const QVariant &result) {
    if (!self || self->shuttingDown_) {
      return;
    }
    qDebug() << "JavaScript result:" << result;
    const quintptr requestId = self->currentRequest_.requestId;
    // Give Mermaid time to render, then capture
    QTimer::singleShot(500, self, [self, requestId]() {
      if (!self || self->shuttingDown_) {
        return;
      }
      self->captureResult(requestId);
    });
  });
}

QString MermaidRenderer::cacheKey(const QString &mermaidCode,
                                  const QString &theme) const {
  return QString("%1|%2").arg(mermaidCode).arg(theme);
}

QPixmap MermaidRenderer::getCached(const QString &mermaidCode,
                                   const QString &theme) const {
  QString key = cacheKey(mermaidCode, theme);
  if (QPixmap *cached = cache_.object(key)) {
    return *cached;
  }
  return QPixmap();
}

void MermaidRenderer::clearCache() { cache_.clear(); }

void MermaidRenderer::render(const QString &mermaidCode, const QString &theme,
                             quintptr requestId) {
  if (shuttingDown_) {
    emit renderComplete(requestId, QPixmap(), false);
    return;
  }

  // Check cache first
  QString key = cacheKey(mermaidCode, theme);
  if (QPixmap *cached = cache_.object(key)) {
    emit renderComplete(requestId, *cached, true);
    return;
  }

  // Initialize WebEngine if needed (lazy init)
  if (!webView_) {
    initializeWebEngine();
  }

  RenderRequest request{mermaidCode, theme, requestId};
  pendingRequests_.append(request);

  if (initialized_ && !rendering_) {
    processNextRequest();
  }
}

void MermaidRenderer::captureResult(quintptr requestId) {
  if (shuttingDown_ || !webView_ || !webView_->page()) {
    qDebug() << "captureResult: no webView or page";
    emit renderComplete(requestId, QPixmap(), false);
    rendering_ = false;
    processNextRequest();
    return;
  }

  // Get the size of the rendered diagram
  QPointer<MermaidRenderer> self(this);
  webView_->page()->runJavaScript(
      "getSize();", [self, requestId](const QVariant &result) {
        if (!self || self->shuttingDown_ || !self->webView_) {
          return;
        }

        QString sizeJson = result.toString();
        qDebug() << "getSize result:" << sizeJson;

        // Parse size (simple JSON parsing)
        int width = 400, height = 300;
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
        width = qMax(width + 32, 100);
        height = qMax(height + 32, 100);

        qDebug() << "Resizing webView to:" << width << "x" << height;

        // Resize view to match content, keep off-screen
        self->webView_->setFixedSize(width, height);
        self->webView_->move(-2000, -2000);

        // Grab after a short delay for content to render
        QTimer::singleShot(100, self, [self, requestId]() {
          if (!self || self->shuttingDown_ || !self->webView_) {
            return;
          }

          QPixmap pixmap = self->webView_->grab();

          qDebug() << "Grabbed pixmap:" << pixmap.size()
                   << "isNull:" << pixmap.isNull();

          if (pixmap.isNull() || pixmap.size().isEmpty()) {
            qDebug() << "Pixmap capture failed";
            emit self->renderComplete(requestId, QPixmap(), false);
          } else {
            // Cache the result
            QString key = self->cacheKey(self->currentRequest_.mermaidCode,
                                         self->currentRequest_.theme);
            self->cache_.insert(key, new QPixmap(pixmap));

            qDebug() << "Emitting renderComplete with pixmap";
            emit self->renderComplete(requestId, pixmap, true);
          }

          self->rendering_ = false;
          self->processNextRequest();
        });
      });
}

#else // !HAVE_QT_WEBENGINE

// Stub implementations when WebEngine is not available

MermaidRenderer &MermaidRenderer::instance() {
  static MermaidRenderer instance;
  return instance;
}

MermaidRenderer::MermaidRenderer(QObject *parent)
    : QObject(parent), cache_(CACHE_SIZE) {}

MermaidRenderer::~MermaidRenderer() = default;

bool MermaidRenderer::isAvailable() const { return false; }

QString MermaidRenderer::cacheKey(const QString &mermaidCode,
                                  const QString &theme) const {
  return QString("%1|%2").arg(mermaidCode).arg(theme);
}

QPixmap MermaidRenderer::getCached(const QString & /*mermaidCode*/,
                                   const QString & /*theme*/) const {
  return QPixmap(); // Always empty - no WebEngine
}

void MermaidRenderer::clearCache() { cache_.clear(); }

void MermaidRenderer::render(const QString & /*mermaidCode*/,
                             const QString & /*theme*/, quintptr requestId) {
  // Immediately signal failure - WebEngine not available
  emit renderComplete(requestId, QPixmap(), false);
}

#endif // HAVE_QT_WEBENGINE
