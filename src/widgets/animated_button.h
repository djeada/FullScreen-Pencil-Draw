/**
 * @file animated_button.h
 * @brief Animated tool and push buttons with smooth hover and press feedback.
 */
#ifndef ANIMATED_BUTTON_H
#define ANIMATED_BUTTON_H

#include <QPushButton>
#include <QToolButton>
#include <QVariantAnimation>

class AnimatedButtonBase {
public:
  enum class Variant {
    PanelTile,
    Compact,
    SideHandle,
    TitleBar
  };

  explicit AnimatedButtonBase(QWidget *widget);
  virtual ~AnimatedButtonBase() = default;

  void setVariant(Variant variant);
  Variant variant() const { return variant_; }

  void handleEnter();
  void handleLeave();
  void handlePress();
  void handleRelease();
  void updateForCheckedState(bool checked);

  qreal hoverProgress() const { return hoverProgress_; }
  qreal pressProgress() const { return pressProgress_; }

protected:
  QWidget *widget() const { return widget_; }
  Variant variant_ = Variant::PanelTile;

private:
  void animateTo(QVariantAnimation &animation, qreal target);

  QWidget *widget_;
  QVariantAnimation hoverAnimation_;
  QVariantAnimation pressAnimation_;
  qreal hoverProgress_ = 0.0;
  qreal pressProgress_ = 0.0;
};

class AnimatedToolButton : public QToolButton {
  Q_OBJECT

public:
  explicit AnimatedToolButton(QWidget *parent = nullptr);

  void setVariant(AnimatedButtonBase::Variant variant);
  AnimatedButtonBase::Variant variant() const { return animation_.variant(); }
  QStyleOptionToolButton buildStyleOption() const;

protected:
  bool event(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  AnimatedButtonBase animation_;
};

class AnimatedPushButton : public QPushButton {
  Q_OBJECT

public:
  explicit AnimatedPushButton(QWidget *parent = nullptr);
  explicit AnimatedPushButton(const QString &text, QWidget *parent = nullptr);

  void setVariant(AnimatedButtonBase::Variant variant);
  AnimatedButtonBase::Variant variant() const { return animation_.variant(); }

protected:
  bool event(QEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private:
  AnimatedButtonBase animation_;
};

#endif // ANIMATED_BUTTON_H
