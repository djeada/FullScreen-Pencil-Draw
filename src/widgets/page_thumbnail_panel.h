/**
 * @file page_thumbnail_panel.h
 * @brief Collapsible page thumbnail panel for PDF navigation.
 */
#ifndef PAGE_THUMBNAIL_PANEL_H
#define PAGE_THUMBNAIL_PANEL_H

#ifdef HAVE_QT_PDF

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

class PdfViewer;

/**
 * @brief A collapsible panel showing page thumbnails for PDF navigation.
 *
 * Displays vertical thumbnails of PDF pages with page numbers.
 * Clicking a thumbnail navigates to that page.
 */
class PageThumbnailPanel : public QWidget {
  Q_OBJECT

public:
  explicit PageThumbnailPanel(PdfViewer *viewer, QWidget *parent = nullptr);
  ~PageThumbnailPanel() override;

  /**
   * @brief Toggle panel visibility
   */
  void toggleVisibility();

  /**
   * @brief Check if panel is visible
   * @return true if visible
   */
  bool isPanelVisible() const { return isVisible(); }

  /**
   * @brief Refresh all thumbnails
   */
  void refreshThumbnails();

  /**
   * @brief Update the current page selection
   * @param pageIndex The page to select (0-based)
   */
  void setCurrentPage(int pageIndex);

signals:
  /**
   * @brief Emitted when a page is selected
   * @param pageIndex The selected page (0-based)
   */
  void pageSelected(int pageIndex);

  /**
   * @brief Emitted when visibility changes
   * @param visible New visibility state
   */
  void visibilityChanged(bool visible);

private slots:
  void onItemClicked(QListWidgetItem *item);
  void onPdfLoaded();
  void onPdfClosed();
  void onPageChanged(int pageIndex, int pageCount);

private:
  PdfViewer *pdfViewer_;
  QListWidget *thumbnailList_;
  QVBoxLayout *layout_;
  
  static constexpr int THUMBNAIL_WIDTH = 120;
  static constexpr int THUMBNAIL_HEIGHT = 160;
  
  void setupUI();
  void generateThumbnails();
  QPixmap renderThumbnail(int pageIndex);
};

#endif // HAVE_QT_PDF

#endif // PAGE_THUMBNAIL_PANEL_H
