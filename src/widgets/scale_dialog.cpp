/**
 * @file scale_dialog.cpp
 * @brief Implementation of the ScaleDialog.
 */
#include "scale_dialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QPushButton>
#include <QVBoxLayout>

ScaleDialog::ScaleDialog(QWidget *parent)
    : QDialog(parent), updatingValues_(false) {
  setWindowTitle("Scale");
  setModal(true);
  setMinimumWidth(300);

  scaleXSpinBox_ = new QDoubleSpinBox(this);
  scaleXSpinBox_->setRange(1.0, 1000.0);
  scaleXSpinBox_->setValue(100.0);
  scaleXSpinBox_->setSuffix(" %");
  scaleXSpinBox_->setDecimals(1);
  scaleXSpinBox_->setMinimumHeight(40);

  scaleYSpinBox_ = new QDoubleSpinBox(this);
  scaleYSpinBox_->setRange(1.0, 1000.0);
  scaleYSpinBox_->setValue(100.0);
  scaleYSpinBox_->setSuffix(" %");
  scaleYSpinBox_->setDecimals(1);
  scaleYSpinBox_->setMinimumHeight(40);

  uniformCheckBox_ = new QCheckBox("Maintain aspect ratio", this);
  uniformCheckBox_->setChecked(true);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(18);

  QFormLayout *formLayout = new QFormLayout();
  formLayout->setSpacing(14);
  formLayout->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);
  formLayout->addRow("Width:", scaleXSpinBox_);
  formLayout->addRow("Height:", scaleYSpinBox_);
  mainLayout->addLayout(formLayout);

  mainLayout->addWidget(uniformCheckBox_);
  mainLayout->addSpacing(10);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(40);
  mainLayout->addWidget(buttonBox);

  connect(scaleXSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ScaleDialog::onScaleXChanged);
  connect(scaleYSpinBox_, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
          this, &ScaleDialog::onScaleYChanged);
  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

double ScaleDialog::scaleX() const { return scaleXSpinBox_->value() / 100.0; }

double ScaleDialog::scaleY() const { return scaleYSpinBox_->value() / 100.0; }

void ScaleDialog::onScaleXChanged(double value) {
  if (updatingValues_)
    return;
  if (uniformCheckBox_->isChecked()) {
    updatingValues_ = true;
    scaleYSpinBox_->setValue(value);
    updatingValues_ = false;
  }
}

void ScaleDialog::onScaleYChanged(double value) {
  if (updatingValues_)
    return;
  if (uniformCheckBox_->isChecked()) {
    updatingValues_ = true;
    scaleXSpinBox_->setValue(value);
    updatingValues_ = false;
  }
}
