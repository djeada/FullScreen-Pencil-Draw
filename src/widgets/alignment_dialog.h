/**
 * @file alignment_dialog.h
 * @brief Dialog for alignment operations on selected items.
 *
 * Provides options to:
 * - Align items to the canvas axes (reset rotation to 0°)
 * - Align two or more items to be parallel (match rotation)
 * - Align two or more items to be perpendicular (90° offset)
 */
#ifndef ALIGNMENT_DIALOG_H
#define ALIGNMENT_DIALOG_H

#include <QDialog>
#include <QRadioButton>

/**
 * @brief Alignment mode enum
 */
enum class AlignmentMode {
  AlignToAxes,   ///< Reset rotation to 0° (align with canvas axes)
  AlignParallel, ///< Make all selected items share the same rotation
  AlignPerpendicular ///< Rotate items to be perpendicular to the first selected
};

class AlignmentDialog : public QDialog {
  Q_OBJECT

public:
  explicit AlignmentDialog(int selectedCount, QWidget *parent = nullptr);

  /**
   * @brief Get the selected alignment mode
   */
  AlignmentMode alignmentMode() const;

private:
  QRadioButton *alignToAxesRadio_;
  QRadioButton *alignParallelRadio_;
  QRadioButton *alignPerpendicularRadio_;
};

#endif // ALIGNMENT_DIALOG_H
