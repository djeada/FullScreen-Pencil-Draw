// image_size_dialog.cpp
#include "image_size_dialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QtMath>

ImageSizeDialog::ImageSizeDialog(int originalWidth, int originalHeight, QWidget *parent)
    : QDialog(parent), originalWidth(originalWidth), originalHeight(originalHeight),
      updatingValues(false) {
  
  setWindowTitle("Specify Image Dimensions");
  setModal(true);
  setMinimumWidth(320);
  
  // Prevent division by zero
  if (this->originalHeight <= 0) {
    this->originalHeight = 1;
  }
  
  aspectRatio = static_cast<double>(this->originalWidth) / static_cast<double>(this->originalHeight);
  
  // Create widgets with modern styling
  widthSpinBox = new QSpinBox(this);
  widthSpinBox->setRange(1, 10000);
  widthSpinBox->setValue(originalWidth);
  widthSpinBox->setSuffix(" px");
  widthSpinBox->setMinimumHeight(36);
  
  heightSpinBox = new QSpinBox(this);
  heightSpinBox->setRange(1, 10000);
  heightSpinBox->setValue(originalHeight);
  heightSpinBox->setSuffix(" px");
  heightSpinBox->setMinimumHeight(36);
  
  maintainAspectCheckBox = new QCheckBox("Maintain aspect ratio", this);
  maintainAspectCheckBox->setChecked(true);
  
  originalSizeLabel = new QLabel(QString("Original size: %1 × %2 px").arg(this->originalWidth).arg(this->originalHeight), this);
  originalSizeLabel->setStyleSheet("QLabel { color: #a0a0a5; font-size: 12px; padding: 4px 0; }");
  
  // Create layout with modern spacing
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(20, 20, 20, 20);
  mainLayout->setSpacing(16);
  
  mainLayout->addWidget(originalSizeLabel);
  
  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(12);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  formLayout->addRow("Width:", widthSpinBox);
  formLayout->addRow("Height:", heightSpinBox);
  mainLayout->addLayout(formLayout);
  
  mainLayout->addWidget(maintainAspectCheckBox);
  
  mainLayout->addSpacing(8);
  
  // Add buttons with modern styling
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(36);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(36);
  buttonBox->button(QDialogButtonBox::Ok)->setStyleSheet(R"(
    QPushButton {
      background-color: #4285f4;
      color: #ffffff;
      border: none;
      border-radius: 6px;
      padding: 8px 24px;
      font-weight: 600;
    }
    QPushButton:hover {
      background-color: #5c9bff;
    }
    QPushButton:pressed {
      background-color: #306ccc;
    }
  )");
  mainLayout->addWidget(buttonBox);
  
  // Connect signals
  connect(widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageSizeDialog::onWidthChanged);
  connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageSizeDialog::onHeightChanged);
  connect(maintainAspectCheckBox, &QCheckBox::stateChanged, this, &ImageSizeDialog::onMaintainAspectChanged);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
  
  setLayout(mainLayout);
}

int ImageSizeDialog::getWidth() const {
  return widthSpinBox->value();
}

int ImageSizeDialog::getHeight() const {
  return heightSpinBox->value();
}

void ImageSizeDialog::onWidthChanged(int value) {
  if (updatingValues) return;
  
  if (maintainAspectCheckBox->isChecked()) {
    updatingValues = true;
    int newHeight = qRound(value / aspectRatio);
    heightSpinBox->setValue(newHeight);
    updatingValues = false;
  }
}

void ImageSizeDialog::onHeightChanged(int value) {
  if (updatingValues) return;
  
  if (maintainAspectCheckBox->isChecked()) {
    updatingValues = true;
    int newWidth = qRound(value * aspectRatio);
    widthSpinBox->setValue(newWidth);
    updatingValues = false;
  }
}

void ImageSizeDialog::onMaintainAspectChanged(int state) {
  // When aspect ratio is toggled on, adjust height based on current width
  if (state == Qt::Checked) {
    onWidthChanged(widthSpinBox->value());
  }
}
