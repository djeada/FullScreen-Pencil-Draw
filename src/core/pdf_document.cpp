/**
 * @file pdf_document.cpp
 * @brief Implementation of PDF document loading and management.
 */
#include "pdf_document.h"

#ifdef HAVE_QT_PDF

#include <QDebug>

// --- PdfPageCache Implementation ---

PdfPageCache::PdfPageCache(int maxPages) : maxPages_(maxPages) {}

PdfPageCache::~PdfPageCache() { clear(); }

QImage PdfPageCache::getPage(int pageIndex, int dpi) const {
  QMutexLocker locker(&mutex_);
  CacheKey key{pageIndex, dpi};
  auto it = cache_.find(key);
  if (it != cache_.end()) {
    it->second.accessCount++;
    return it->second.image;
  }
  return QImage();
}

void PdfPageCache::setPage(int pageIndex, int dpi, const QImage &image) {
  QMutexLocker locker(&mutex_);
  evictIfNeeded();
  CacheKey key{pageIndex, dpi};
  cache_[key] = CacheEntry{image, 1};
}

bool PdfPageCache::hasPage(int pageIndex, int dpi) const {
  QMutexLocker locker(&mutex_);
  CacheKey key{pageIndex, dpi};
  return cache_.find(key) != cache_.end();
}

void PdfPageCache::clear() {
  QMutexLocker locker(&mutex_);
  cache_.clear();
}

void PdfPageCache::removePage(int pageIndex) {
  QMutexLocker locker(&mutex_);
  // Remove all DPI variants for this page
  for (auto it = cache_.begin(); it != cache_.end();) {
    if (it->first.pageIndex == pageIndex) {
      it = cache_.erase(it);
    } else {
      ++it;
    }
  }
}

void PdfPageCache::evictIfNeeded() {
  // Evict least accessed pages if cache is full
  while (static_cast<int>(cache_.size()) >= maxPages_ && !cache_.empty()) {
    auto minIt = cache_.begin();
    for (auto it = cache_.begin(); it != cache_.end(); ++it) {
      if (it->second.accessCount < minIt->second.accessCount) {
        minIt = it;
      }
    }
    cache_.erase(minIt);
  }
}

// --- PdfDocument Implementation ---

#ifdef HAVE_QT_PDF

PdfDocument::PdfDocument(QObject *parent)
    : QObject(parent), document_(std::make_unique<QPdfDocument>()),
      cache_(std::make_unique<PdfPageCache>(20)),
      status_(Status::NotLoaded) {

  connect(document_.get(), &QPdfDocument::statusChanged, this,
          &PdfDocument::onDocumentStatusChanged);
}

PdfDocument::~PdfDocument() { close(); }

bool PdfDocument::load(const QString &filePath) {
  close();
  filePath_ = filePath;
  setStatus(Status::Loading);

  auto error = document_->load(filePath);
  if (error != QPdfDocument::Error::None) {
    // Make sure document is closed on error
    document_->close();
    
    switch (error) {
    case QPdfDocument::Error::FileNotFound:
      errorMessage_ = tr("File not found: %1").arg(filePath);
      break;
    case QPdfDocument::Error::InvalidFileFormat:
      errorMessage_ = tr("Invalid PDF file format");
      break;
    case QPdfDocument::Error::UnsupportedSecurityScheme:
      errorMessage_ = tr("Unsupported security scheme (encrypted PDF)");
      break;
    default:
      errorMessage_ = tr("Unknown error loading PDF");
      break;
    }
    setStatus(Status::Error);
    emit errorOccurred(errorMessage_);
    return false;
  }

  return true;
}

void PdfDocument::close() {
  if (document_) {
    document_->close();
  }
  cache_->clear();
  filePath_.clear();
  errorMessage_.clear();
  setStatus(Status::NotLoaded);
}

PdfDocument::Status PdfDocument::status() const { return status_; }

int PdfDocument::pageCount() const {
  if (status_ != Status::Ready) {
    return 0;
  }
  return document_->pageCount();
}

QSizeF PdfDocument::pageSize(int pageIndex) const {
  if (status_ != Status::Ready || pageIndex < 0 ||
      pageIndex >= document_->pageCount()) {
    return QSizeF();
  }
  return document_->pagePointSize(pageIndex);
}

QString PdfDocument::filePath() const { return filePath_; }

QString PdfDocument::errorMessage() const { return errorMessage_; }

QImage PdfDocument::renderPage(int pageIndex, int dpi, bool inverted) {
  if (status_ != Status::Ready || pageIndex < 0 ||
      pageIndex >= document_->pageCount()) {
    return QImage();
  }

  // Check cache first (only for non-inverted images)
  if (!inverted && cache_->hasPage(pageIndex, dpi)) {
    return cache_->getPage(pageIndex, dpi);
  }

  // Calculate image size based on page size and DPI
  QSizeF pageSizePoints = document_->pagePointSize(pageIndex);
  qreal scale = dpi / 72.0; // 72 points per inch
  QSize imageSize(static_cast<int>(pageSizePoints.width() * scale),
                  static_cast<int>(pageSizePoints.height() * scale));

  // Render the page
  QImage image = document_->render(pageIndex, imageSize);

  if (image.isNull()) {
    return QImage();
  }

  // Cache the rendered image (only non-inverted)
  if (!inverted) {
    cache_->setPage(pageIndex, dpi, image);
  }

  // Apply color inversion if requested
  if (inverted) {
    image = invertImage(image);
  }

  return image;
}

bool PdfDocument::isPageCached(int pageIndex, int dpi) const {
  return cache_->hasPage(pageIndex, dpi);
}

void PdfDocument::clearCache() { cache_->clear(); }

void PdfDocument::onDocumentStatusChanged() {
  switch (document_->status()) {
  case QPdfDocument::Status::Ready:
    setStatus(Status::Ready);
    emit documentLoaded();
    break;
  case QPdfDocument::Status::Loading:
    setStatus(Status::Loading);
    break;
  case QPdfDocument::Status::Error:
    setStatus(Status::Error);
    break;
  case QPdfDocument::Status::Null:
  case QPdfDocument::Status::Unloading:
    setStatus(Status::NotLoaded);
    break;
  }
}

#else

PdfDocument::PdfDocument(QObject *parent)
    : QObject(parent), cache_(std::make_unique<PdfPageCache>(0)),
      status_(Status::NotLoaded) {}

PdfDocument::~PdfDocument() { close(); }

bool PdfDocument::load(const QString &filePath) {
  close();
  filePath_ = filePath;
  errorMessage_ = tr("Qt PDF module is not available.");
  setStatus(Status::Error);
  emit errorOccurred(errorMessage_);
  return false;
}

void PdfDocument::close() {
  if (cache_) {
    cache_->clear();
  }
  filePath_.clear();
  errorMessage_.clear();
  setStatus(Status::NotLoaded);
}

PdfDocument::Status PdfDocument::status() const { return status_; }

int PdfDocument::pageCount() const { return 0; }

QSizeF PdfDocument::pageSize(int pageIndex) const {
  Q_UNUSED(pageIndex);
  return QSizeF();
}

QString PdfDocument::filePath() const { return filePath_; }

QString PdfDocument::errorMessage() const { return errorMessage_; }

QImage PdfDocument::renderPage(int pageIndex, int dpi, bool inverted) {
  Q_UNUSED(pageIndex);
  Q_UNUSED(dpi);
  Q_UNUSED(inverted);
  return QImage();
}

bool PdfDocument::isPageCached(int pageIndex, int dpi) const {
  Q_UNUSED(pageIndex);
  Q_UNUSED(dpi);
  return false;
}

void PdfDocument::clearCache() {
  if (cache_) {
    cache_->clear();
  }
}

void PdfDocument::onDocumentStatusChanged() {}

#endif

void PdfDocument::setStatus(Status status) {
  if (status_ != status) {
    status_ = status;
    emit statusChanged(status);
  }
}

QImage PdfDocument::invertImage(const QImage &image) const {
  QImage inverted = image.copy();
  inverted.invertPixels(QImage::InvertRgb);
  return inverted;
}

#endif // HAVE_QT_PDF
