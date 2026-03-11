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
        darkTheme ? QColor(0, 0, 0, 105) : QColor(143, 102, 65, 46));
    painter->drawRoundedRect(shadowRect, 10, 10);

    painter->setBrush(darkTheme ? QColor(17, 22, 28, 220)
                                : QColor(255, 250, 244, 238));
    painter->setPen(darkTheme ? QColor(249, 115, 22, 95)
                              : QColor(234, 88, 12, 105));
    painter->drawRoundedRect(badgeRect, 10, 10);

    painter->setPen(darkTheme ? QColor("#fff7ed") : QColor("#31261d"));
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
                 background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                             stop:0 #17212b, stop:1 #10161d);
                 color: #fff7ed;
                 padding: 12px 10px;
                 font-weight: 700;
                 font-size: 12px;
                 letter-spacing: 0.7px;
                 border-bottom: 1px solid rgba(255, 244, 230, 0.08);
               })"
            : R"(QLabel {
                 background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                             stop:0 #fff9f1, stop:1 #f1e5d5);
                 color: #31261d;
                 padding: 12px 10px;
                 font-weight: 700;
                 font-size: 12px;
                 letter-spacing: 0.7px;
                 border-bottom: 1px solid #ddcfbc;
               })");
  }

  if (darkTheme) {
    setStyleSheet(R"(
      PageThumbnailPanel {
        background-color: #10161d;
        border-right: 1px solid rgba(255, 244, 230, 0.06);
      }
      QListWidget {
        background-color: #10161d;
        border: none;
        outline: none;
        color: #fff7ed;
      }
      QListWidget::item {
        background-color: #17212b;
        color: #fff7ed;
        border: 2px solid transparent;
        border-radius: 12px;
        padding: 6px;
        margin: 6px 10px;
      }
      QListWidget::item:hover {
        background-color: #1d2934;
        border: 2px solid rgba(249, 115, 22, 0.28);
      }
      QListWidget::item:selected {
        background-color: #22160d;
        border: 2px solid #f97316;
      }
      QScrollBar:vertical {
        background-color: #10161d;
        width: 10px;
        border: none;
      }
      QScrollBar::handle:vertical {
        background-color: rgba(249, 115, 22, 0.32);
        border-radius: 5px;
        min-height: 30px;
      }
      QScrollBar::handle:vertical:hover {
        background-color: rgba(249, 115, 22, 0.48);
      }
      QScrollBar::add-line:vertical,
      QScrollBar::sub-line:vertical {
        height: 0px;
      }
    )");
  } else {
    setStyleSheet(R"(
      PageThumbnailPanel {
        background-color: #f5efe6;
        border-right: 1px solid #ddcfbc;
      }
      QListWidget {
        background-color: #f5efe6;
        border: none;
        outline: none;
        color: #31261d;
      }
      QListWidget::item {
        background-color: #fff9f1;
        color: #31261d;
        border: 2px solid transparent;
        border-radius: 12px;
        padding: 6px;
        margin: 6px 10px;
      }
      QListWidget::item:hover {
        background-color: #fff4e7;
        border: 2px solid rgba(234, 88, 12, 0.22);
      }
      QListWidget::item:selected {
        background-color: #fff1df;
        border: 2px solid #f97316;
      }
      QScrollBar:vertical {
        background-color: #f5efe6;
        width: 10px;
        border: none;
      }
      QScrollBar::handle:vertical {
        background-color: rgba(234, 88, 12, 0.28);
        border-radius: 5px;
        min-height: 30px;
      }
      QScrollBar::handle:vertical:hover {
        background-color: rgba(234, 88, 12, 0.42);
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
