/**
 * @file page_thumbnail_panel.cpp
 * @brief Implementation of the page thumbnail panel.
 */
#include "page_thumbnail_panel.h"

#ifdef HAVE_QT_PDF

#include "../core/pdf_document.h"
#include "pdf_viewer.h"
#include <QLabel>
#include <QScrollBar>

PageThumbnailPanel::PageThumbnailPanel(PdfViewer *viewer, QWidget *parent)
    : QWidget(parent), pdfViewer_(viewer), thumbnailList_(nullptr),
      layout_(nullptr) {
  setupUI();

  if (pdfViewer_) {
    connect(pdfViewer_, &PdfViewer::pdfLoaded, this,
            &PageThumbnailPanel::onPdfLoaded);
    connect(pdfViewer_, &PdfViewer::pdfClosed, this,
            &PageThumbnailPanel::onPdfClosed);
    connect(pdfViewer_, &PdfViewer::pageChanged, this,
            &PageThumbnailPanel::onPageChanged);
  }
}

PageThumbnailPanel::~PageThumbnailPanel() = default;

void PageThumbnailPanel::setupUI() {
  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);

  // Header
  QLabel *header = new QLabel("Pages", this);
  header->setAlignment(Qt::AlignCenter);
  header->setStyleSheet(R"(
    QLabel {
      background-color: #2a2a30;
      color: #f8f8fc;
      padding: 10px;
      font-weight: 600;
      font-size: 12px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    }
  )");
  layout_->addWidget(header);

  // Thumbnail list
  thumbnailList_ = new QListWidget(this);
  thumbnailList_->setViewMode(QListView::IconMode);
  thumbnailList_->setIconSize(QSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT));
  thumbnailList_->setSpacing(8);
  thumbnailList_->setMovement(QListView::Static);
  thumbnailList_->setResizeMode(QListView::Adjust);
  thumbnailList_->setFlow(QListView::TopToBottom);
  thumbnailList_->setWrapping(false);
  thumbnailList_->setSelectionMode(QAbstractItemView::SingleSelection);
  thumbnailList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  thumbnailList_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

  connect(thumbnailList_, &QListWidget::itemClicked, this,
          &PageThumbnailPanel::onItemClicked);

  layout_->addWidget(thumbnailList_);

  setMinimumWidth(THUMBNAIL_WIDTH + 40);
  setMaximumWidth(THUMBNAIL_WIDTH + 60);

  // Styling
  setStyleSheet(R"(
    PageThumbnailPanel {
      background-color: #1a1a1e;
      border-right: 1px solid rgba(255, 255, 255, 0.06);
    }
    QListWidget {
      background-color: #1a1a1e;
      border: none;
      outline: none;
    }
    QListWidget::item {
      background-color: #242428;
      border: 2px solid transparent;
      border-radius: 6px;
      padding: 4px;
      margin: 4px 8px;
    }
    QListWidget::item:hover {
      background-color: #2a2a30;
      border: 2px solid rgba(59, 130, 246, 0.3);
    }
    QListWidget::item:selected {
      background-color: #2a2a30;
      border: 2px solid #3b82f6;
    }
    QScrollBar:vertical {
      background-color: #1a1a1e;
      width: 10px;
      border: none;
    }
    QScrollBar::handle:vertical {
      background-color: #3a3a40;
      border-radius: 5px;
      min-height: 30px;
    }
    QScrollBar::handle:vertical:hover {
      background-color: #4a4a50;
    }
    QScrollBar::add-line:vertical,
    QScrollBar::sub-line:vertical {
      height: 0px;
    }
  )");
}

void PageThumbnailPanel::toggleVisibility() {
  setVisible(!isVisible());
  emit visibilityChanged(isVisible());
}

void PageThumbnailPanel::refreshThumbnails() { generateThumbnails(); }

void PageThumbnailPanel::setCurrentPage(int pageIndex) {
  if (thumbnailList_ && pageIndex >= 0 && pageIndex < thumbnailList_->count()) {
    thumbnailList_->blockSignals(true);
    thumbnailList_->setCurrentRow(pageIndex);
    thumbnailList_->scrollToItem(thumbnailList_->item(pageIndex));
    thumbnailList_->blockSignals(false);
  }
}

void PageThumbnailPanel::onItemClicked(QListWidgetItem *item) {
  if (item) {
    int pageIndex = thumbnailList_->row(item);
    emit pageSelected(pageIndex);
  }
}

void PageThumbnailPanel::onPdfLoaded() {
  generateThumbnails();
  show();
  emit visibilityChanged(true);
}

void PageThumbnailPanel::onPdfClosed() {
  thumbnailList_->clear();
  hide();
  emit visibilityChanged(false);
}

void PageThumbnailPanel::onPageChanged(int pageIndex, int pageCount) {
  Q_UNUSED(pageCount);
  setCurrentPage(pageIndex);
}

void PageThumbnailPanel::generateThumbnails() {
  if (!pdfViewer_ || !pdfViewer_->hasPdf()) {
    return;
  }

  thumbnailList_->clear();

  PdfDocument *doc = pdfViewer_->document();
  if (!doc) {
    return;
  }

  int pageCount = doc->pageCount();

  for (int i = 0; i < pageCount; ++i) {
    QPixmap thumbnail = renderThumbnail(i);

    QListWidgetItem *item = new QListWidgetItem();
    item->setIcon(QIcon(thumbnail));
    item->setText(QString::number(i + 1));
    item->setTextAlignment(Qt::AlignHCenter | Qt::AlignBottom);
    item->setSizeHint(QSize(THUMBNAIL_WIDTH + 16, THUMBNAIL_HEIGHT + 24));

    thumbnailList_->addItem(item);
  }

  // Select current page
  if (thumbnailList_->count() > 0) {
    setCurrentPage(pdfViewer_->currentPage());
  }
}

QPixmap PageThumbnailPanel::renderThumbnail(int pageIndex) {
  if (!pdfViewer_ || !pdfViewer_->document()) {
    return QPixmap();
  }

  // Render at low DPI for thumbnail
  QImage pageImage = pdfViewer_->document()->renderPage(pageIndex, 36, false);
  if (pageImage.isNull()) {
    // Return a placeholder
    QPixmap placeholder(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);
    placeholder.fill(Qt::gray);
    return placeholder;
  }

  // Scale to thumbnail size while maintaining aspect ratio
  QPixmap pixmap = QPixmap::fromImage(pageImage);
  return pixmap.scaled(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, Qt::KeepAspectRatio,
                       Qt::SmoothTransformation);
}

#endif // HAVE_QT_PDF
