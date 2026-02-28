/**
 * @file test_alignment_dialog.cpp
 * @brief Unit tests for AlignmentDialog.
 */
#include "../src/widgets/alignment_dialog.h"
#include <QTest>

class TestAlignmentDialog : public QObject {
  Q_OBJECT

private slots:

  void defaultModeIsAlignToAxes() {
    AlignmentDialog dlg(1);
    QCOMPARE(dlg.alignmentMode(), AlignmentMode::AlignToAxes);
  }

  void windowTitleIsSet() {
    AlignmentDialog dlg(2);
    QCOMPARE(dlg.windowTitle(), QString("Align Items"));
  }

  void dialogIsModal() {
    AlignmentDialog dlg(1);
    QVERIFY(dlg.isModal());
  }

  void defaultModeWithMultipleItems() {
    AlignmentDialog dlg(3);
    // Default should still be AlignToAxes
    QCOMPARE(dlg.alignmentMode(), AlignmentMode::AlignToAxes);
  }
};

QTEST_MAIN(TestAlignmentDialog)
#include "test_alignment_dialog.moc"
