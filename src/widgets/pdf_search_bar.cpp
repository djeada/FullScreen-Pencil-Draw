/**
 * @file pdf_search_bar.cpp
 * @brief Implementation of floating search bar for PDF text search.
 */
#include "pdf_search_bar.h"

#ifdef HAVE_QT_PDF

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

PdfSearchBar::PdfSearchBar(QWidget *parent) : QFrame(parent) {
  setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
  setAutoFillBackground(true);
  setFixedHeight(40);
  hide();

  setStyleSheet(R"(
    PdfSearchBar {
      background-color: #1e1e24;
      border: 1px solid rgba(255, 255, 255, 0.15);
      border-radius: 8px;
    }
  )");

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 4, 8, 4);
  layout->setSpacing(4);

  // Search input
  searchInput_ = new QLineEdit(this);
  searchInput_->setPlaceholderText("Find in PDF…");
  searchInput_->setClearButtonEnabled(true);
  searchInput_->setMinimumWidth(180);
  searchInput_->setStyleSheet(R"(
    QLineEdit {
      background-color: #2a2a30;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.1);
      border-radius: 4px;
      padding: 4px 8px;
      font-size: 13px;
      selection-background-color: #3b82f6;
    }
    QLineEdit:focus {
      border: 1px solid #3b82f6;
    }
  )");
  searchInput_->installEventFilter(this);
  layout->addWidget(searchInput_);

  // Match count label
  matchLabel_ = new QLabel(this);
  matchLabel_->setStyleSheet(R"(
    QLabel {
      color: #a0a0a8;
      font-size: 12px;
      padding: 0 4px;
      min-width: 60px;
    }
  )");
  matchLabel_->setAlignment(Qt::AlignCenter);
  layout->addWidget(matchLabel_);

  const QString buttonStyle = R"(
    QPushButton {
      background-color: transparent;
      color: #d0d0d8;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 4px;
      padding: 4px 8px;
      font-size: 14px;
      min-width: 28px;
      min-height: 24px;
    }
    QPushButton:hover {
      background-color: rgba(255, 255, 255, 0.08);
      border-color: rgba(255, 255, 255, 0.15);
    }
    QPushButton:pressed {
      background-color: rgba(255, 255, 255, 0.12);
    }
    QPushButton:disabled {
      color: #555;
    }
  )";

  // Previous button
  prevButton_ = new QPushButton("▲", this);
  prevButton_->setToolTip("Previous match (Shift+Enter)");
  prevButton_->setStyleSheet(buttonStyle);
  prevButton_->setEnabled(false);
  layout->addWidget(prevButton_);

  // Next button
  nextButton_ = new QPushButton("▼", this);
  nextButton_->setToolTip("Next match (Enter)");
  nextButton_->setStyleSheet(buttonStyle);
  nextButton_->setEnabled(false);
  layout->addWidget(nextButton_);

  // Close button
  closeButton_ = new QPushButton("✕", this);
  closeButton_->setToolTip("Close (Esc)");
  closeButton_->setStyleSheet(R"(
    QPushButton {
      background-color: transparent;
      color: #a0a0a8;
      border: none;
      border-radius: 4px;
      padding: 4px 6px;
      font-size: 13px;
      min-width: 24px;
      min-height: 24px;
    }
    QPushButton:hover {
      background-color: rgba(255, 80, 80, 0.2);
      color: #ff6060;
    }
  )");
  layout->addWidget(closeButton_);

  // Connections
  connect(searchInput_, &QLineEdit::textChanged, this,
          &PdfSearchBar::searchTextChanged);
  connect(prevButton_, &QPushButton::clicked, this,
          &PdfSearchBar::findPrevious);
  connect(nextButton_, &QPushButton::clicked, this, &PdfSearchBar::findNext);
  connect(closeButton_, &QPushButton::clicked, this, &PdfSearchBar::deactivate);
}

void PdfSearchBar::activate() {
  positionInParent();
  show();
  raise();
  searchInput_->setFocus();
  searchInput_->selectAll();
}

void PdfSearchBar::deactivate() {
  hide();
  searchInput_->clear();
  matchLabel_->clear();
  prevButton_->setEnabled(false);
  nextButton_->setEnabled(false);
  emit closed();
}

void PdfSearchBar::setMatchInfo(int current, int total) {
  if (total == 0) {
    if (searchInput_->text().isEmpty()) {
      matchLabel_->clear();
    } else {
      matchLabel_->setText(tr("No results"));
      matchLabel_->setStyleSheet(R"(
        QLabel {
          color: #f87171;
          font-size: 12px;
          padding: 0 4px;
          min-width: 60px;
        }
      )");
    }
    prevButton_->setEnabled(false);
    nextButton_->setEnabled(false);
  } else {
    matchLabel_->setText(tr("%1 of %2").arg(current).arg(total));
    matchLabel_->setStyleSheet(R"(
      QLabel {
        color: #a0a0a8;
        font-size: 12px;
        padding: 0 4px;
        min-width: 60px;
      }
    )");
    prevButton_->setEnabled(true);
    nextButton_->setEnabled(true);
  }
}

QString PdfSearchBar::searchText() const { return searchInput_->text(); }

bool PdfSearchBar::eventFilter(QObject *obj, QEvent *event) {
  if (obj == searchInput_ && event->type() == QEvent::KeyPress) {
    auto *keyEvent = static_cast<QKeyEvent *>(event);
    if (keyEvent->key() == Qt::Key_Escape) {
      deactivate();
      return true;
    }
    if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
      if (keyEvent->modifiers() & Qt::ShiftModifier) {
        emit findPrevious();
      } else {
        emit findNext();
      }
      return true;
    }
  }
  return QFrame::eventFilter(obj, event);
}

void PdfSearchBar::positionInParent() {
  if (!parentWidget()) {
    return;
  }
  int pw = parentWidget()->width();
  int w = qMin(380, pw - 20);
  setFixedWidth(w);
  // Top-right corner with margin
  move(pw - w - 10, 10);
}

#endif // HAVE_QT_PDF
