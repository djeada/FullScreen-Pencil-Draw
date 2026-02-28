/**
 * @file rotation_dialog.cpp
 * @brief Implementation of the RotationDialog.
 */
#include "rotation_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

RotationDialog::RotationDialog(QWidget *parent) : QDialog(parent) {
  setWindowTitle("Rotate");
  setModal(true);
  setMinimumWidth(300);

  angleSpinBox_ = new QDoubleSpinBox(this);
  angleSpinBox_->setRange(-360.0, 360.0);
  angleSpinBox_->setValue(0.0);
  angleSpinBox_->setSuffix(" °");
  angleSpinBox_->setDecimals(1);
  angleSpinBox_->setMinimumHeight(40);
  angleSpinBox_->setWrapping(true);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(18);

  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(14);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  formLayout->addRow("Angle:", angleSpinBox_);
  mainLayout->addLayout(formLayout);

  // Preset angle buttons
  QHBoxLayout *presetLayout = new QHBoxLayout();
  presetLayout->setSpacing(8);

  auto addPreset = [this, presetLayout](const QString &label, double value) {
    QPushButton *btn = new QPushButton(label, this);
    btn->setMinimumHeight(32);
    connect(btn, &QPushButton::clicked, this,
            [this, value]() { angleSpinBox_->setValue(value); });
    presetLayout->addWidget(btn);
  };

  addPreset("90°", 90.0);
  addPreset("180°", 180.0);
  addPreset("270°", 270.0);
  addPreset("-90°", -90.0);
  mainLayout->addLayout(presetLayout);
  mainLayout->addSpacing(10);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(40);
  mainLayout->addWidget(buttonBox);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

double RotationDialog::angle() const { return angleSpinBox_->value(); }
