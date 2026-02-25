/**
 * @file scan_document_dialog.cpp
 * @brief Implementation of the scan document options dialog.
 */
#include "scan_document_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

ScanDocumentDialog::ScanDocumentDialog(bool hasSelection, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("Scan Document Filter");
  setModal(true);
  setMinimumWidth(460);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(14);

  // --- Target selector ---
  QGroupBox *targetGroup = new QGroupBox("Apply To", this);
  QHBoxLayout *targetLayout = new QHBoxLayout(targetGroup);
  targetCombo_ = new QComboBox(this);
  targetCombo_->addItem("Selected Element", SelectedElement);
  targetCombo_->addItem("Whole Canvas", WholeCanvas);
  if (!hasSelection)
    targetCombo_->setCurrentIndex(1);
  targetLayout->addWidget(targetCombo_);
  mainLayout->addWidget(targetGroup);

  // --- Mode selector ---
  QGroupBox *modeGroup = new QGroupBox("Mode", this);
  QHBoxLayout *modeLayout = new QHBoxLayout(modeGroup);
  modeCombo_ = new QComboBox(this);
  modeCombo_->addItem("Enhance Document (recommended)");
  modeCombo_->addItem("Hard Black && White");
  modeLayout->addWidget(modeCombo_);
  mainLayout->addWidget(modeGroup);

  // --- Contrast / Threshold ---
  QGroupBox *contrastGroup = new QGroupBox("Contrast / Threshold", this);
  QFormLayout *contrastLayout = new QFormLayout(contrastGroup);
  thresholdSlider_ = new QSlider(Qt::Horizontal, this);
  thresholdSlider_->setRange(0, 100);
  thresholdSlider_->setValue(50);
  thresholdLabel_ = new QLabel("0.50", this);
  thresholdLabel_->setMinimumWidth(36);
  QHBoxLayout *threshRow = new QHBoxLayout();
  threshRow->addWidget(thresholdSlider_, 1);
  threshRow->addWidget(thresholdLabel_);
  contrastLayout->addRow("Strength:", threshRow);
  mainLayout->addWidget(contrastGroup);

  // --- Text Sharpening ---
  QGroupBox *sharpenGroup = new QGroupBox("Text Sharpening", this);
  QFormLayout *sharpenLayout = new QFormLayout(sharpenGroup);
  sharpenSlider_ = new QSlider(Qt::Horizontal, this);
  sharpenSlider_->setRange(0, 30); // 0.0 – 3.0
  sharpenSlider_->setValue(15);    // default 1.5
  sharpenLabel_ = new QLabel("1.5", this);
  sharpenLabel_->setMinimumWidth(36);
  QHBoxLayout *sharpenRow = new QHBoxLayout();
  sharpenRow->addWidget(sharpenSlider_, 1);
  sharpenRow->addWidget(sharpenLabel_);
  sharpenLayout->addRow("Strength:", sharpenRow);
  mainLayout->addWidget(sharpenGroup);

  // --- Background Whitening ---
  QGroupBox *wpGroup = new QGroupBox("Background Whitening", this);
  QFormLayout *wpLayout = new QFormLayout(wpGroup);
  whitePointSlider_ = new QSlider(Qt::Horizontal, this);
  whitePointSlider_->setRange(0, 100);
  whitePointSlider_->setValue(90); // default 0.9
  whitePointLabel_ = new QLabel("0.90", this);
  whitePointLabel_->setMinimumWidth(36);
  QHBoxLayout *wpRow = new QHBoxLayout();
  wpRow->addWidget(whitePointSlider_, 1);
  wpRow->addWidget(whitePointLabel_);
  wpLayout->addRow("Aggressiveness:", wpRow);
  mainLayout->addWidget(wpGroup);

  // --- Noise ---
  QGroupBox *noiseGroup = new QGroupBox("Scanner Noise", this);
  QFormLayout *noiseLayout = new QFormLayout(noiseGroup);
  noiseSlider_ = new QSlider(Qt::Horizontal, this);
  noiseSlider_->setRange(0, 10);
  noiseSlider_->setValue(0); // default: clean
  noiseLabel_ = new QLabel("0", this);
  noiseLabel_->setMinimumWidth(36);
  QHBoxLayout *noiseRow = new QHBoxLayout();
  noiseRow->addWidget(noiseSlider_, 1);
  noiseRow->addWidget(noiseLabel_);
  noiseLayout->addRow("Level:", noiseRow);
  mainLayout->addWidget(noiseGroup);

  // --- Sepia tint ---
  QGroupBox *sepiaGroup = new QGroupBox("Paper Tint", this);
  QVBoxLayout *sepiaVLayout = new QVBoxLayout(sepiaGroup);
  sepiaCheckBox_ = new QCheckBox("Enable warm sepia tint", this);
  sepiaCheckBox_->setChecked(false); // default off for document mode
  sepiaVLayout->addWidget(sepiaCheckBox_);
  QFormLayout *sepiaFormLayout = new QFormLayout();
  sepiaSlider_ = new QSlider(Qt::Horizontal, this);
  sepiaSlider_->setRange(0, 100);
  sepiaSlider_->setValue(50);
  sepiaLabel_ = new QLabel("0.50", this);
  sepiaLabel_->setMinimumWidth(36);
  QHBoxLayout *sepiaRow = new QHBoxLayout();
  sepiaRow->addWidget(sepiaSlider_, 1);
  sepiaRow->addWidget(sepiaLabel_);
  sepiaFormLayout->addRow("Strength:", sepiaRow);
  sepiaVLayout->addLayout(sepiaFormLayout);
  mainLayout->addWidget(sepiaGroup);

  // --- Vignette ---
  QGroupBox *vignetteGroup = new QGroupBox("Edge Vignette", this);
  QVBoxLayout *vigVLayout = new QVBoxLayout(vignetteGroup);
  vignetteCheckBox_ = new QCheckBox("Enable edge darkening", this);
  vignetteCheckBox_->setChecked(false); // default off
  vigVLayout->addWidget(vignetteCheckBox_);
  QFormLayout *vigFormLayout = new QFormLayout();
  vignetteSlider_ = new QSlider(Qt::Horizontal, this);
  vignetteSlider_->setRange(0, 100);
  vignetteSlider_->setValue(50);
  vignetteLabel_ = new QLabel("0.50", this);
  vignetteLabel_->setMinimumWidth(36);
  QHBoxLayout *vigRow = new QHBoxLayout();
  vigRow->addWidget(vignetteSlider_, 1);
  vigRow->addWidget(vignetteLabel_);
  vigFormLayout->addRow("Strength:", vigRow);
  vigVLayout->addLayout(vigFormLayout);
  mainLayout->addWidget(vignetteGroup);

  // --- Buttons ---
  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setText("Apply");
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(40);
  mainLayout->addWidget(buttonBox);

  // Connections
  connect(thresholdSlider_, &QSlider::valueChanged, this,
          &ScanDocumentDialog::onThresholdChanged);
  connect(sharpenSlider_, &QSlider::valueChanged, this,
          &ScanDocumentDialog::onSharpenChanged);
  connect(whitePointSlider_, &QSlider::valueChanged, this,
          &ScanDocumentDialog::onWhitePointChanged);
  connect(noiseSlider_, &QSlider::valueChanged, this,
          &ScanDocumentDialog::onNoiseChanged);
  connect(sepiaSlider_, &QSlider::valueChanged, this,
          &ScanDocumentDialog::onSepiaStrengthChanged);
  connect(vignetteSlider_, &QSlider::valueChanged, this,
          &ScanDocumentDialog::onVignetteStrengthChanged);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

ScanDocumentDialog::Target ScanDocumentDialog::target() const {
  return static_cast<Target>(targetCombo_->currentData().toInt());
}

bool ScanDocumentDialog::hardBinarize() const {
  return modeCombo_->currentIndex() == 1;
}

double ScanDocumentDialog::threshold() const {
  return thresholdSlider_->value() / 100.0;
}

double ScanDocumentDialog::sharpenStrength() const {
  return sharpenSlider_->value() / 10.0;
}

double ScanDocumentDialog::whitePoint() const {
  return whitePointSlider_->value() / 100.0;
}

int ScanDocumentDialog::noiseLevel() const { return noiseSlider_->value(); }

bool ScanDocumentDialog::sepiaEnabled() const {
  return sepiaCheckBox_->isChecked();
}

double ScanDocumentDialog::sepiaStrength() const {
  return sepiaSlider_->value() / 100.0;
}

bool ScanDocumentDialog::vignetteEnabled() const {
  return vignetteCheckBox_->isChecked();
}

double ScanDocumentDialog::vignetteStrength() const {
  return vignetteSlider_->value() / 100.0;
}

void ScanDocumentDialog::onThresholdChanged(int value) {
  thresholdLabel_->setText(QString::number(value / 100.0, 'f', 2));
}

void ScanDocumentDialog::onSharpenChanged(int value) {
  sharpenLabel_->setText(QString::number(value / 10.0, 'f', 1));
}

void ScanDocumentDialog::onWhitePointChanged(int value) {
  whitePointLabel_->setText(QString::number(value / 100.0, 'f', 2));
}

void ScanDocumentDialog::onNoiseChanged(int value) {
  noiseLabel_->setText(QString::number(value));
}

void ScanDocumentDialog::onSepiaStrengthChanged(int value) {
  sepiaLabel_->setText(QString::number(value / 100.0, 'f', 2));
}

void ScanDocumentDialog::onVignetteStrengthChanged(int value) {
  vignetteLabel_->setText(QString::number(value / 100.0, 'f', 2));
}
