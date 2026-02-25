/**
 * @file color_curves_dialog.h
 * @brief Dialog for configuring color levels / curves adjustment.
 *
 * Provides master and per-channel input black, white, and gamma sliders
 * plus brightness and contrast controls.
 */
#ifndef COLOR_CURVES_DIALOG_H
#define COLOR_CURVES_DIALOG_H

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QSlider>

class ColorCurvesDialog : public QDialog {
  Q_OBJECT

public:
  enum Target { SelectedElement, WholeCanvas };

  explicit ColorCurvesDialog(bool hasSelection, QWidget *parent = nullptr);

  Target target() const;

  // Master
  int inputBlack() const;
  int inputWhite() const;
  double gamma() const;

  // Per-channel
  int redInputBlack() const;
  int redInputWhite() const;
  double redGamma() const;
  int greenInputBlack() const;
  int greenInputWhite() const;
  double greenGamma() const;
  int blueInputBlack() const;
  int blueInputWhite() const;
  double blueGamma() const;

  // Brightness / contrast
  int brightness() const;
  int contrast() const;

private slots:
  void onInputBlackChanged(int value);
  void onInputWhiteChanged(int value);
  void onGammaChanged(int value);
  void onRedInputBlackChanged(int value);
  void onRedInputWhiteChanged(int value);
  void onRedGammaChanged(int value);
  void onGreenInputBlackChanged(int value);
  void onGreenInputWhiteChanged(int value);
  void onGreenGammaChanged(int value);
  void onBlueInputBlackChanged(int value);
  void onBlueInputWhiteChanged(int value);
  void onBlueGammaChanged(int value);
  void onBrightnessChanged(int value);
  void onContrastChanged(int value);

private:
  QComboBox *targetCombo_;

  // Master
  QSlider *inputBlackSlider_;
  QLabel *inputBlackLabel_;
  QSlider *inputWhiteSlider_;
  QLabel *inputWhiteLabel_;
  QSlider *gammaSlider_;
  QLabel *gammaLabel_;

  // Per-channel
  QSlider *redInputBlackSlider_;
  QLabel *redInputBlackLabel_;
  QSlider *redInputWhiteSlider_;
  QLabel *redInputWhiteLabel_;
  QSlider *redGammaSlider_;
  QLabel *redGammaLabel_;

  QSlider *greenInputBlackSlider_;
  QLabel *greenInputBlackLabel_;
  QSlider *greenInputWhiteSlider_;
  QLabel *greenInputWhiteLabel_;
  QSlider *greenGammaSlider_;
  QLabel *greenGammaLabel_;

  QSlider *blueInputBlackSlider_;
  QLabel *blueInputBlackLabel_;
  QSlider *blueInputWhiteSlider_;
  QLabel *blueInputWhiteLabel_;
  QSlider *blueGammaSlider_;
  QLabel *blueGammaLabel_;

  // Brightness / contrast
  QSlider *brightnessSlider_;
  QLabel *brightnessLabel_;
  QSlider *contrastSlider_;
  QLabel *contrastLabel_;
};

#endif // COLOR_CURVES_DIALOG_H
