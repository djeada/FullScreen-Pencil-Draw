// resize_canvas_dialog.cpp
#include "resize_canvas_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>

ResizeCanvasDialog::ResizeCanvasDialog(int currentWidth, int currentHeight,
                                       QWidget *parent)
    : QDialog(parent), selectedAnchor_(Center) {

  setWindowTitle("Resize Canvas");
  setModal(true);
  setMinimumWidth(340);

  currentSizeLabel_ = new QLabel(
      QString("Current size: %1 \u00d7 %2 px").arg(currentWidth).arg(currentHeight),
      this);
  currentSizeLabel_->setStyleSheet("QLabel { color: #a0a0a8; font-size: 12px; "
                                   "padding: 6px 0; font-weight: 500; }");

  widthSpinBox_ = new QSpinBox(this);
  widthSpinBox_->setRange(1, 10000);
  widthSpinBox_->setValue(currentWidth);
  widthSpinBox_->setSuffix(" px");
  widthSpinBox_->setMinimumHeight(40);

  heightSpinBox_ = new QSpinBox(this);
  heightSpinBox_->setRange(1, 10000);
  heightSpinBox_->setValue(currentHeight);
  heightSpinBox_->setSuffix(" px");
  heightSpinBox_->setMinimumHeight(40);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(18);

  mainLayout->addWidget(currentSizeLabel_);

  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(14);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  formLayout->addRow("Width:", widthSpinBox_);
  formLayout->addRow("Height:", heightSpinBox_);
  mainLayout->addLayout(formLayout);

  // Anchor position selector
  QGroupBox *anchorGroup = new QGroupBox("Anchor", this);
  QGridLayout *anchorLayout = new QGridLayout(anchorGroup);
  anchorLayout->setSpacing(4);

  for (int i = 0; i < 9; ++i) {
    anchorButtons_[i] = new QPushButton(this);
    anchorButtons_[i]->setFixedSize(28, 28);
    anchorButtons_[i]->setCheckable(true);
    anchorButtons_[i]->setProperty("anchorIndex", i);
    connect(anchorButtons_[i], &QPushButton::clicked, this,
            &ResizeCanvasDialog::onAnchorClicked);
    anchorLayout->addWidget(anchorButtons_[i], i / 3, i % 3);
  }

  updateAnchorButtons();
  mainLayout->addWidget(anchorGroup);

  mainLayout->addSpacing(10);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(R"(
    QPushButton {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa);
      color: #ffffff;
      border: 1px solid rgba(255, 255, 255, 0.15);
      border-radius: 8px;
      padding: 10px 28px;
      font-weight: 600;
    }
    QPushButton:hover {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #60a5fa, stop:1 #93c5fd);
    }
    QPushButton:pressed {
      background-color: #2563eb;
    }
  )");
  mainLayout->addWidget(buttonBox);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

  setLayout(mainLayout);
}

int ResizeCanvasDialog::getWidth() const { return widthSpinBox_->value(); }

int ResizeCanvasDialog::getHeight() const { return heightSpinBox_->value(); }

ResizeCanvasDialog::Anchor ResizeCanvasDialog::getAnchor() const {
  return selectedAnchor_;
}

void ResizeCanvasDialog::onAnchorClicked() {
  QPushButton *btn = qobject_cast<QPushButton *>(sender());
  if (!btn)
    return;
  selectedAnchor_ = static_cast<Anchor>(btn->property("anchorIndex").toInt());
  updateAnchorButtons();
}

void ResizeCanvasDialog::updateAnchorButtons() {
  for (int i = 0; i < 9; ++i) {
    anchorButtons_[i]->setChecked(i == static_cast<int>(selectedAnchor_));
  }
}
