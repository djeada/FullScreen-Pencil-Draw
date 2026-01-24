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
  setMinimumWidth(340);
  
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
  widthSpinBox->setMinimumHeight(40);
  
  heightSpinBox = new QSpinBox(this);
  heightSpinBox->setRange(1, 10000);
  heightSpinBox->setValue(originalHeight);
  heightSpinBox->setSuffix(" px");
  heightSpinBox->setMinimumHeight(40);
  
  maintainAspectCheckBox = new QCheckBox("Maintain aspect ratio", this);
  maintainAspectCheckBox->setChecked(true);
  
  originalSizeLabel = new QLabel(QString("Original size: %1 × %2 px").arg(this->originalWidth).arg(this->originalHeight), this);
  originalSizeLabel->setStyleSheet("QLabel { color: #a0a0a8; font-size: 12px; padding: 6px 0; font-weight: 500; }");
  
  // Create layout with modern spacing
  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(18);
  
  mainLayout->addWidget(originalSizeLabel);
  
  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(14);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  formLayout->addRow("Width:", widthSpinBox);
  formLayout->addRow("Height:", heightSpinBox);
  mainLayout->addLayout(formLayout);
  
  mainLayout->addWidget(maintainAspectCheckBox);
  
  mainLayout->addSpacing(10);
  
  // Add buttons with modern styling
  QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
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
