/**
 * @file scan_document_dialog.h
 * @brief Dialog for configuring the "scanned document" image filter.
 *
 * Provides mode selector (Enhance Document / Hard B&W), sliders for all
 * parameters, and a choice between applying to the whole canvas or a single
 * element.
 */
#ifndef SCAN_DOCUMENT_DIALOG_H
#define SCAN_DOCUMENT_DIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QSlider>

class ScanDocumentDialog : public QDialog {
  Q_OBJECT

public:
  enum Target { SelectedElement, WholeCanvas };

  explicit ScanDocumentDialog(bool hasSelection, QWidget *parent = nullptr);

  Target target() const;
  bool hardBinarize() const;
  double threshold() const;        // 0.0–1.0
  double sharpenStrength() const;  // 0.0–3.0
  double whitePoint() const;       // 0.0–1.0
  int noiseLevel() const;          // 0–10
  bool sepiaEnabled() const;
  double sepiaStrength() const;    // 0.0–1.0
  bool vignetteEnabled() const;
  double vignetteStrength() const; // 0.0–1.0

private slots:
  void onThresholdChanged(int value);
  void onSharpenChanged(int value);
  void onWhitePointChanged(int value);
  void onNoiseChanged(int value);
  void onSepiaStrengthChanged(int value);
  void onVignetteStrengthChanged(int value);

private:
  QComboBox *targetCombo_;
  QComboBox *modeCombo_;
  QSlider *thresholdSlider_;
  QLabel *thresholdLabel_;
  QSlider *sharpenSlider_;
  QLabel *sharpenLabel_;
  QSlider *whitePointSlider_;
  QLabel *whitePointLabel_;
  QSlider *noiseSlider_;
  QLabel *noiseLabel_;
  QCheckBox *sepiaCheckBox_;
  QSlider *sepiaSlider_;
  QLabel *sepiaLabel_;
  QCheckBox *vignetteCheckBox_;
  QSlider *vignetteSlider_;
  QLabel *vignetteLabel_;
};

#endif // SCAN_DOCUMENT_DIALOG_H
