/**
 * @file page_thumbnail_panel.cpp
 * @brief Implementation of the page thumbnail panel.
 */
#include "page_thumbnail_panel.h"

#ifdef HAVE_QT_PDF

#include "../core/pdf_document.h"
#include "../core/theme_manager.h"
#include "pdf_viewer.h"
#include <QApplication>
#include <QLabel>
#include <QPalette>
#include <QPainter>
#include <QScrollBar>
#include <QStyledItemDelegate>

namespace {

class ThumbnailItemDelegate : public QStyledItemDelegate {
public:
  explicit ThumbnailItemDelegate(QObject *parent = nullptr)
      : QStyledItemDelegate(parent) {}

  void paint(QPainter *painter, const QStyleOptionViewItem &option,
             const QModelIndex &index) const override {
    QStyleOptionViewItem opt(option);
    initStyleOption(&opt, index);

    if (opt.widget) {
      opt.widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt,
                                         painter, opt.widget);
    }

    const QRect contentRect = opt.rect.adjusted(8, 6, -8, -6);
    const int badgeHeight = 22;
    const int badgeBottomMargin = 4;
    const QRect thumbArea =
        contentRect.adjusted(0, 0, 0, -(badgeHeight + badgeBottomMargin + 2));

    const QPixmap pixmap =
        opt.icon.pixmap(opt.decorationSize, QIcon::Normal, QIcon::Off);
    if (!pixmap.isNull()) {
      QSize scaledSize = pixmap.size();
      scaledSize.scale(thumbArea.size(), Qt::KeepAspectRatio);
      QRect pixmapRect(QPoint(0, 0), scaledSize);
      pixmapRect.moveCenter(thumbArea.center());
      painter->drawPixmap(pixmapRect, pixmap);
    }

    const bool darkTheme = ThemeManager::instance().isDarkTheme();
    const QString pageText = index.data(Qt::DisplayRole).toString();
    const QFontMetrics fm(opt.font);
    const int badgeWidth = qMax(28, fm.horizontalAdvance(pageText) + 16);
    QRect badgeRect(0, 0, badgeWidth, badgeHeight);
    badgeRect.moveCenter(
        QPoint(contentRect.center().x(),
               contentRect.bottom() - badgeHeight / 2 - badgeBottomMargin));

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QRect shadowRect = badgeRect.translated(0, 1);
    painter->setPen(Qt::NoPen);
    painter->setBrush(
        darkTheme ? QColor(0, 0, 0, 90) : QColor(120, 120, 120, 55));
    painter->drawRoundedRect(shadowRect, 10, 10);

    painter->setBrush(darkTheme ? QColor(22, 22, 26, 210)
                                : QColor(255, 255, 255, 235));
    painter->setPen(darkTheme ? QColor(255, 255, 255, 40)
                              : QColor(173, 181, 189, 220));
    painter->drawRoundedRect(badgeRect, 10, 10);

    painter->setPen(darkTheme ? QColor("#f8f8fc") : QColor("#343a40"));
    painter->drawText(badgeRect, Qt::AlignCenter, pageText);

    painter->restore();

    if (opt.state & QStyle::State_HasFocus) {
      QStyleOptionFocusRect focusOpt;
      focusOpt.QStyleOption::operator=(opt);
      focusOpt.rect = opt.rect.adjusted(2, 2, -2, -2);
      focusOpt.state |= QStyle::State_KeyboardFocusChange;
      focusOpt.backgroundColor = opt.palette.color(QPalette::Base);
      if (opt.widget) {
        opt.widget->style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusOpt,
                                           painter, opt.widget);
      }
    }
  }
};

} // namespace

PageThumbnailPanel::PageThumbnailPanel(PdfViewer *viewer, QWidget *parent)
    : QWidget(parent), pdfViewer_(viewer), header_(nullptr),
      thumbnailList_(nullptr),
      layout_(nullptr) {
  setupUI();

  if (pdfViewer_) {
    connect(pdfViewer_, &PdfViewer::pdfLoaded, this,
            &PageThumbnailPanel::onPdfLoaded);
    connect(pdfViewer_, &PdfViewer::pdfClosed, this,
            &PageThumbnailPanel::onPdfClosed);
    connect(pdfViewer_, &PdfViewer::pageChanged, this,
            &PageThumbnailPanel::onPageChanged);
    connect(pdfViewer_, &PdfViewer::darkModeChanged, this,
            [this]() { refreshThumbnails(); });
  }

  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          [this]() { applyTheme(); });
}

PageThumbnailPanel::~PageThumbnailPanel() = default;

void PageThumbnailPanel::setupUI() {
  layout_ = new QVBoxLayout(this);
  layout_->setContentsMargins(0, 0, 0, 0);
  layout_->setSpacing(0);

  // Header
  header_ = new QLabel("Pages", this);
  header_->setAlignment(Qt::AlignCenter);
  layout_->addWidget(header_);

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
  thumbnailList_->setItemDelegate(new ThumbnailItemDelegate(thumbnailList_));

  connect(thumbnailList_, &QListWidget::itemClicked, this,
          &PageThumbnailPanel::onItemClicked);

  layout_->addWidget(thumbnailList_);

  setMinimumWidth(THUMBNAIL_WIDTH + 40);
  setMaximumWidth(THUMBNAIL_WIDTH + 60);

  applyTheme();
}

void PageThumbnailPanel::applyTheme() {
  const bool darkTheme = ThemeManager::instance().isDarkTheme();

  if (header_) {
    header_->setStyleSheet(
        darkTheme
            ? R"(QLabel {
                 background-color: #2a2a30;
                 color: #f8f8fc;
                 padding: 10px;
                 font-weight: 600;
                 font-size: 12px;
                 border-bottom: 1px solid rgba(255, 255, 255, 0.08);
               })"
            : R"(QLabel {
                 background-color: #e9ecef;
                 color: #343a40;
                 padding: 10px;
                 font-weight: 600;
                 font-size: 12px;
                 border-bottom: 1px solid #dee2e6;
               })");
  }

  if (darkTheme) {
    setStyleSheet(R"(
      PageThumbnailPanel {
        background-color: #1a1a1e;
        border-right: 1px solid rgba(255, 255, 255, 0.06);
      }
      QListWidget {
        background-color: #1a1a1e;
        border: none;
        outline: none;
        color: #f8f8fc;
      }
      QListWidget::item {
        background-color: #242428;
        color: #f8f8fc;
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
  } else {
    setStyleSheet(R"(
      PageThumbnailPanel {
        background-color: #f8f9fa;
        border-right: 1px solid #e9ecef;
      }
      QListWidget {
        background-color: #f8f9fa;
        border: none;
        outline: none;
        color: #343a40;
      }
      QListWidget::item {
        background-color: #ffffff;
        color: #343a40;
        border: 2px solid transparent;
        border-radius: 6px;
        padding: 4px;
        margin: 4px 8px;
      }
      QListWidget::item:hover {
        background-color: #f1f3f5;
        border: 2px solid rgba(66, 133, 244, 0.25);
      }
      QListWidget::item:selected {
        background-color: #ffffff;
        border: 2px solid #4285f4;
      }
      QScrollBar:vertical {
        background-color: #f8f9fa;
        width: 10px;
        border: none;
      }
      QScrollBar::handle:vertical {
        background-color: #ced4da;
        border-radius: 5px;
        min-height: 30px;
      }
      QScrollBar::handle:vertical:hover {
        background-color: #adb5bd;
      }
      QScrollBar::add-line:vertical,
      QScrollBar::sub-line:vertical {
        height: 0px;
      }
    )");
  }

  if (!thumbnailList_) {
    return;
  }

  const QColor textColor = palette().color(QPalette::Text);
  for (int i = 0; i < thumbnailList_->count(); ++i) {
    if (QListWidgetItem *item = thumbnailList_->item(i)) {
      item->setForeground(textColor);
    }
  }
  thumbnailList_->viewport()->update();
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
    item->setForeground(palette().color(QPalette::Text));

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

  // Render at a readable DPI and mirror the viewer's dark mode.
  QImage pageImage =
      pdfViewer_->document()->renderPage(pageIndex, 72, pdfViewer_->darkMode());
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
