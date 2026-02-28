/**
 * @file alignment_dialog.cpp
 * @brief Implementation of the AlignmentDialog.
 */
#include "alignment_dialog.h"
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

AlignmentDialog::AlignmentDialog(int selectedCount, QWidget *parent)
    : QDialog(parent) {
  setWindowTitle("Align Items");
  setModal(true);
  setMinimumWidth(340);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(24, 24, 24, 24);
  mainLayout->setSpacing(18);

  QGroupBox *group = new QGroupBox("Alignment Mode", this);
  QVBoxLayout *groupLayout = new QVBoxLayout(group);
  groupLayout->setSpacing(12);

  alignToAxesRadio_ = new QRadioButton(
      "Align with axes (reset rotation to 0°)", group);
  alignToAxesRadio_->setChecked(true);
  groupLayout->addWidget(alignToAxesRadio_);

  alignParallelRadio_ = new QRadioButton(
      "Make parallel (match rotation of first item)", group);
  groupLayout->addWidget(alignParallelRadio_);

  alignPerpendicularRadio_ = new QRadioButton(
      "Make perpendicular (90° offset from first item)", group);
  groupLayout->addWidget(alignPerpendicularRadio_);

  // Disable multi-item options when fewer than 2 items are selected
  bool multiItem = selectedCount >= 2;
  alignParallelRadio_->setEnabled(multiItem);
  alignPerpendicularRadio_->setEnabled(multiItem);

  if (!multiItem) {
    alignParallelRadio_->setToolTip("Select two or more items to enable");
    alignPerpendicularRadio_->setToolTip("Select two or more items to enable");
  }

  mainLayout->addWidget(group);
  mainLayout->addSpacing(10);

  QDialogButtonBox *buttonBox = new QDialogButtonBox(
      QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
  buttonBox->button(QDialogButtonBox::Ok)->setMinimumHeight(40);
  buttonBox->button(QDialogButtonBox::Cancel)->setMinimumHeight(40);
  mainLayout->addWidget(buttonBox);

  connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
  connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

AlignmentMode AlignmentDialog::alignmentMode() const {
  if (alignParallelRadio_->isChecked())
    return AlignmentMode::AlignParallel;
  if (alignPerpendicularRadio_->isChecked())
    return AlignmentMode::AlignPerpendicular;
  return AlignmentMode::AlignToAxes;
}
