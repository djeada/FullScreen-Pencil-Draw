/**
 * @file color_curves_dialog.cpp
 * @brief Implementation of the color levels / curves dialog.
 */
#include "color_curves_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

// Helper: create a slider + label row and add it to a form layout.
static QSlider *addSliderRow(QFormLayout *form, const QString &label,
                             QLabel *&valueLabel, int min, int max, int initial,
                             QWidget *parent) {
  auto *slider = new QSlider(Qt::Horizontal, parent);
  slider->setRange(min, max);
  slider->setValue(initial);
  valueLabel = new QLabel(QString::number(initial), parent);
  valueLabel->setMinimumWidth(36);
  auto *row = new QHBoxLayout();
  row->addWidget(slider, 1);
  row->addWidget(valueLabel);
  form->addRow(label, row);
  return slider;
}

ColorCurvesDialog::ColorCurvesDialog(bool hasSelection, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("Color Curves / Levels");
  setModal(true);
  setMinimumWidth(480);

  auto *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(14);

  // --- Target selector ---
  auto *targetGroup = new QGroupBox("Apply To", this);
  auto *targetLayout = new QHBoxLayout(targetGroup);
  targetCombo_ = new QComboBox(this);
  targetCombo_->addItem("Selected Element", SelectedElement);
  targetCombo_->addItem("Whole Canvas", WholeCanvas);
  if (!hasSelection)
    targetCombo_->setCurrentIndex(1);
  targetLayout->addWidget(targetCombo_);
  mainLayout->addWidget(targetGroup);

  // --- Master levels ---
  auto *masterGroup = new QGroupBox("Master Levels", this);
  auto *masterForm = new QFormLayout(masterGroup);
  inputBlackSlider_ = addSliderRow(masterForm, "Input Black:", inputBlackLabel_,
                                   0, 255, 0, this);
  inputWhiteSlider_ = addSliderRow(masterForm, "Input White:", inputWhiteLabel_,
                                   0, 255, 255, this);
  gammaSlider_ =
      addSliderRow(masterForm, "Gamma:", gammaLabel_, 10, 300, 100, this);
  gammaLabel_->setText("1.00");
  mainLayout->addWidget(masterGroup);

  // --- Red channel ---
  auto *redGroup = new QGroupBox("Red Channel", this);
  auto *redForm = new QFormLayout(redGroup);
  redInputBlackSlider_ = addSliderRow(
      redForm, "Input Black:", redInputBlackLabel_, 0, 255, 0, this);
  redInputWhiteSlider_ = addSliderRow(
      redForm, "Input White:", redInputWhiteLabel_, 0, 255, 255, this);
  redGammaSlider_ =
      addSliderRow(redForm, "Gamma:", redGammaLabel_, 10, 300, 100, this);
  redGammaLabel_->setText("1.00");
  mainLayout->addWidget(redGroup);

  // --- Green channel ---
  auto *greenGroup = new QGroupBox("Green Channel", this);
  auto *greenForm = new QFormLayout(greenGroup);
  greenInputBlackSlider_ = addSliderRow(
      greenForm, "Input Black:", greenInputBlackLabel_, 0, 255, 0, this);
  greenInputWhiteSlider_ = addSliderRow(
      greenForm, "Input White:", greenInputWhiteLabel_, 0, 255, 255, this);
  greenGammaSlider_ =
      addSliderRow(greenForm, "Gamma:", greenGammaLabel_, 10, 300, 100, this);
  greenGammaLabel_->setText("1.00");
  mainLayout->addWidget(greenGroup);

  // --- Blue channel ---
  auto *blueGroup = new QGroupBox("Blue Channel", this);
  auto *blueForm = new QFormLayout(blueGroup);
  blueInputBlackSlider_ = addSliderRow(
      blueForm, "Input Black:", blueInputBlackLabel_, 0, 255, 0, this);
  blueInputWhiteSlider_ = addSliderRow(
      blueForm, "Input White:", blueInputWhiteLabel_, 0, 255, 255, this);
  blueGammaSlider_ =
      addSliderRow(blueForm, "Gamma:", blueGammaLabel_, 10, 300, 100, this);
  blueGammaLabel_->setText("1.00");
  mainLayout->addWidget(blueGroup);

  // --- Brightness / Contrast ---
  auto *bcGroup = new QGroupBox("Brightness / Contrast", this);
  auto *bcForm = new QFormLayout(bcGroup);
  brightnessSlider_ =
      addSliderRow(bcForm, "Brightness:", brightnessLabel_, -100, 100, 0, this);
  contrastSlider_ =
      addSliderRow(bcForm, "Contrast:", contrastLabel_, -100, 100, 0, this);
  mainLayout->addWidget(bcGroup);

  // --- Buttons ---
  auto *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setText("Apply");
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(40);
  mainLayout->addWidget(buttonBox);

  // Connections – master
  connect(inputBlackSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onInputBlackChanged);
  connect(inputWhiteSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onInputWhiteChanged);
  connect(gammaSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onGammaChanged);
  // Red
  connect(redInputBlackSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onRedInputBlackChanged);
  connect(redInputWhiteSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onRedInputWhiteChanged);
  connect(redGammaSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onRedGammaChanged);
  // Green
  connect(greenInputBlackSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onGreenInputBlackChanged);
  connect(greenInputWhiteSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onGreenInputWhiteChanged);
  connect(greenGammaSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onGreenGammaChanged);
  // Blue
  connect(blueInputBlackSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onBlueInputBlackChanged);
  connect(blueInputWhiteSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onBlueInputWhiteChanged);
  connect(blueGammaSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onBlueGammaChanged);
  // Brightness / contrast
  connect(brightnessSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onBrightnessChanged);
  connect(contrastSlider_, &QSlider::valueChanged, this,
          &ColorCurvesDialog::onContrastChanged);
  // Buttons
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// --- Accessors ---

ColorCurvesDialog::Target ColorCurvesDialog::target() const {
  return static_cast<Target>(targetCombo_->currentData().toInt());
}

int ColorCurvesDialog::inputBlack() const { return inputBlackSlider_->value(); }
int ColorCurvesDialog::inputWhite() const { return inputWhiteSlider_->value(); }
double ColorCurvesDialog::gamma() const {
  return gammaSlider_->value() / 100.0;
}

int ColorCurvesDialog::redInputBlack() const {
  return redInputBlackSlider_->value();
}
int ColorCurvesDialog::redInputWhite() const {
  return redInputWhiteSlider_->value();
}
double ColorCurvesDialog::redGamma() const {
  return redGammaSlider_->value() / 100.0;
}

int ColorCurvesDialog::greenInputBlack() const {
  return greenInputBlackSlider_->value();
}
int ColorCurvesDialog::greenInputWhite() const {
  return greenInputWhiteSlider_->value();
}
double ColorCurvesDialog::greenGamma() const {
  return greenGammaSlider_->value() / 100.0;
}

int ColorCurvesDialog::blueInputBlack() const {
  return blueInputBlackSlider_->value();
}
int ColorCurvesDialog::blueInputWhite() const {
  return blueInputWhiteSlider_->value();
}
double ColorCurvesDialog::blueGamma() const {
  return blueGammaSlider_->value() / 100.0;
}

int ColorCurvesDialog::brightness() const { return brightnessSlider_->value(); }
int ColorCurvesDialog::contrast() const { return contrastSlider_->value(); }

// --- Slot helpers ---

void ColorCurvesDialog::onInputBlackChanged(int value) {
  inputBlackLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onInputWhiteChanged(int value) {
  inputWhiteLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onGammaChanged(int value) {
  gammaLabel_->setText(QString::number(value / 100.0, 'f', 2));
}
void ColorCurvesDialog::onRedInputBlackChanged(int value) {
  redInputBlackLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onRedInputWhiteChanged(int value) {
  redInputWhiteLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onRedGammaChanged(int value) {
  redGammaLabel_->setText(QString::number(value / 100.0, 'f', 2));
}
void ColorCurvesDialog::onGreenInputBlackChanged(int value) {
  greenInputBlackLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onGreenInputWhiteChanged(int value) {
  greenInputWhiteLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onGreenGammaChanged(int value) {
  greenGammaLabel_->setText(QString::number(value / 100.0, 'f', 2));
}
void ColorCurvesDialog::onBlueInputBlackChanged(int value) {
  blueInputBlackLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onBlueInputWhiteChanged(int value) {
  blueInputWhiteLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onBlueGammaChanged(int value) {
  blueGammaLabel_->setText(QString::number(value / 100.0, 'f', 2));
}
void ColorCurvesDialog::onBrightnessChanged(int value) {
  brightnessLabel_->setText(QString::number(value));
}
void ColorCurvesDialog::onContrastChanged(int value) {
  contrastLabel_->setText(QString::number(value));
}
