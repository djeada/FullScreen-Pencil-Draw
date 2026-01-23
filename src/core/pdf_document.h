/**
 * @file pdf_document.h
 * @brief PDF document loading and management using Qt PDF module.
 *
 * This file defines the PdfDocument class for loading PDF files and
 * the PdfPageCache class for caching rendered pages.
 */
#ifndef PDF_DOCUMENT_H
#define PDF_DOCUMENT_H

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QPdfDocument>
#include <QSize>
#include <QString>
#include <memory>
#include <unordered_map>

/**
 * @brief Cache for rendered PDF pages.
 *
 * Provides caching of rendered PDF pages by page index and DPI.
 * Implements a simple LRU-like eviction strategy to bound memory usage.
 */
class PdfPageCache {
public:
  /**
   * @brief Construct a new PdfPageCache
   * @param maxPages Maximum number of pages to cache (default: 10)
   */
  explicit PdfPageCache(int maxPages = 10);
  ~PdfPageCache();

  /**
   * @brief Get a cached page image
   * @param pageIndex The page index
   * @param dpi The rendering DPI
   * @return The cached image, or null QImage if not cached
   */
  QImage getPage(int pageIndex, int dpi) const;

  /**
   * @brief Store a page image in the cache
   * @param pageIndex The page index
   * @param dpi The rendering DPI
   * @param image The rendered image
   */
  void setPage(int pageIndex, int dpi, const QImage &image);

  /**
   * @brief Check if a page is cached
   * @param pageIndex The page index
   * @param dpi The rendering DPI
   * @return true if the page is cached
   */
  bool hasPage(int pageIndex, int dpi) const;

  /**
   * @brief Clear the entire cache
   */
  void clear();

  /**
   * @brief Remove a specific page from cache
   * @param pageIndex The page index to remove
   */
  void removePage(int pageIndex);

private:
  struct CacheKey {
    int pageIndex;
    int dpi;

    bool operator==(const CacheKey &other) const {
      return pageIndex == other.pageIndex && dpi == other.dpi;
    }
  };

  struct CacheKeyHash {
    std::size_t operator()(const CacheKey &key) const {
      return std::hash<int>()(key.pageIndex) ^
             (std::hash<int>()(key.dpi) << 1);
    }
  };

  struct CacheEntry {
    QImage image;
    mutable int accessCount;
  };

  int maxPages_;
  mutable QMutex mutex_;
  std::unordered_map<CacheKey, CacheEntry, CacheKeyHash> cache_;

  void evictIfNeeded();
};

/**
 * @brief Manages a PDF document using Qt's QPdfDocument.
 *
 * Provides functionality to load PDF files, retrieve page count and sizes,
 * render individual pages to QImage, and handle errors.
 */
class PdfDocument : public QObject {
  Q_OBJECT

public:
  /**
   * @brief Document load status
   */
  enum class Status {
    NotLoaded,
    Loading,
    Ready,
    Error
  };
  Q_ENUM(Status)

  /**
   * @brief Construct a new PdfDocument
   * @param parent Parent QObject
   */
  explicit PdfDocument(QObject *parent = nullptr);
  ~PdfDocument() override;

  /**
   * @brief Load a PDF file
   * @param filePath Path to the PDF file
   * @return true if loading started successfully
   */
  bool load(const QString &filePath);

  /**
   * @brief Close the current document
   */
  void close();

  /**
   * @brief Get the current document status
   * @return The status
   */
  Status status() const;

  /**
   * @brief Get the number of pages in the document
   * @return Page count, or 0 if not loaded
   */
  int pageCount() const;

  /**
   * @brief Get the size of a specific page in points
   * @param pageIndex The page index (0-based)
   * @return The page size, or empty QSizeF if invalid
   */
  QSizeF pageSize(int pageIndex) const;

  /**
   * @brief Get the file path of the loaded document
   * @return The file path, or empty string if not loaded
   */
  QString filePath() const;

  /**
   * @brief Get the last error message
   * @return Error message, or empty string if no error
   */
  QString errorMessage() const;

  /**
   * @brief Render a page to a QImage
   * @param pageIndex The page index (0-based)
   * @param dpi Rendering DPI (default: 96)
   * @param inverted Apply color inversion for dark mode
   * @return The rendered image, or null QImage on failure
   */
  QImage renderPage(int pageIndex, int dpi = 96, bool inverted = false);

  /**
   * @brief Check if a page is cached
   * @param pageIndex The page index
   * @param dpi The rendering DPI
   * @return true if cached
   */
  bool isPageCached(int pageIndex, int dpi) const;

  /**
   * @brief Clear the page cache
   */
  void clearCache();

  /**
   * @brief Get the underlying QPdfDocument (for advanced use)
   * @return Pointer to the QPdfDocument
   */
  QPdfDocument *document() const { return document_.get(); }

signals:
  /**
   * @brief Emitted when document status changes
   * @param status The new status
   */
  void statusChanged(PdfDocument::Status status);

  /**
   * @brief Emitted when the document is loaded successfully
   */
  void documentLoaded();

  /**
   * @brief Emitted when an error occurs
   * @param message The error message
   */
  void errorOccurred(const QString &message);

private slots:
  void onDocumentStatusChanged();

private:
  std::unique_ptr<QPdfDocument> document_;
  std::unique_ptr<PdfPageCache> cache_;
  QString filePath_;
  QString errorMessage_;
  Status status_;

  void setStatus(Status status);
  QImage invertImage(const QImage &image) const;
};

#endif // PDF_DOCUMENT_H
