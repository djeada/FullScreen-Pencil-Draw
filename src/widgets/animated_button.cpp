/**
 * @file animated_button.cpp
 * @brief Animated button implementation.
 */
#include "animated_button.h"
#include "../core/theme_manager.h"
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyleOptionToolButton>

namespace {

QColor blend(const QColor &from, const QColor &to, qreal progress) {
  const qreal t = qBound(0.0, progress, 1.0);
  return QColor::fromRgbF(from.redF() + (to.redF() - from.redF()) * t,
                          from.greenF() + (to.greenF() - from.greenF()) * t,
                          from.blueF() + (to.blueF() - from.blueF()) * t,
                          from.alphaF() + (to.alphaF() - from.alphaF()) * t);
}

bool isDarkSurface(const QWidget *widget) {
  Q_UNUSED(widget);
  return ThemeManager::instance().isDarkTheme();
}

struct ButtonPaletteSet {
  QColor base;
  QColor hover;
  QColor pressed;
  QColor checked;
  QColor border;
  QColor borderHover;
  QColor text;
  QColor textChecked;
  qreal radius;
};

ButtonPaletteSet paletteFor(const QWidget *widget,
                            AnimatedButtonBase::Variant variant) {
  const bool dark = isDarkSurface(widget);

  switch (variant) {
  case AnimatedButtonBase::Variant::Compact:
    if (dark) {
      return {QColor("#17212b"), QColor("#22303c"), QColor("#2b1f15"),
              QColor("#f97316"), QColor(255, 244, 230, 22),
              QColor(249, 115, 22, 110), QColor("#fff7ed"),
              QColor("#fffaf4"), 10.0};
    }
    return {QColor("#fff9f1"), QColor("#fff1e1"), QColor("#f6dfca"),
            QColor("#f97316"), QColor("#ddcfbc"), QColor(234, 88, 12, 82),
            QColor("#31261d"), QColor("#fffaf4"), 10.0};
  case AnimatedButtonBase::Variant::SideHandle:
    if (dark) {
      return {QColor("#17212b"), QColor("#22303c"), QColor("#2b1f15"),
              QColor("#fb923c"), QColor(255, 244, 230, 20),
              QColor(249, 115, 22, 120), QColor("#fff7ed"),
              QColor("#fffaf4"), 12.0};
    }
    return {QColor("#fff9f1"), QColor("#fff1e1"), QColor("#f6dfca"),
            QColor("#fb923c"), QColor("#ddcfbc"), QColor(234, 88, 12, 92),
            QColor("#31261d"), QColor("#fffaf4"), 12.0};
  case AnimatedButtonBase::Variant::TitleBar:
    if (dark) {
      return {QColor("#17212b"), QColor("#1f2b36"), QColor("#2b1f15"),
              QColor("#f97316"), QColor(255, 244, 230, 20),
              QColor(249, 115, 22, 110), QColor("#fff7ed"),
              QColor("#fffaf4"), 9.0};
    }
    return {QColor("#fff9f1"), QColor("#fff1e1"), QColor("#f6dfca"),
            QColor("#f97316"), QColor("#ddcfbc"), QColor(234, 88, 12, 82),
            QColor("#31261d"), QColor("#fffaf4"), 9.0};
  case AnimatedButtonBase::Variant::PanelTile:
  default:
    if (dark) {
      return {QColor("#17212b"), QColor("#21303b"), QColor("#2b1f15"),
              QColor("#f97316"), QColor(255, 244, 230, 20),
              QColor(249, 115, 22, 110), QColor("#fff7ed"),
              QColor("#fffaf4"), 12.0};
    }
    return {QColor("#fff9f1"), QColor("#fff1e1"), QColor("#f6dfca"),
            QColor("#f97316"), QColor("#ddcfbc"), QColor(234, 88, 12, 88),
            QColor("#31261d"), QColor("#fffaf4"), 12.0};
  }
}

void paintButtonSurface(QPainter &painter, QWidget *widget,
                        AnimatedButtonBase::Variant variant, qreal hover,
                        qreal press, bool checked, bool enabled) {
  const ButtonPaletteSet palette = paletteFor(widget, variant);

  QColor fill = checked ? palette.checked : blend(palette.base, palette.hover, hover);
  fill = blend(fill, palette.pressed, press);
  QColor border = blend(palette.border, palette.borderHover, hover);
  border = checked ? palette.borderHover : border;

  if (!enabled) {
    fill.setAlphaF(fill.alphaF() * 0.45);
    border.setAlphaF(border.alphaF() * 0.35);
  }

  painter.save();
  painter.setRenderHint(QPainter::Antialiasing, true);

  QRectF rect = widget->rect().adjusted(0.5, 0.5, -0.5, -0.5);
  rect.translate(0.0, press * 1.0);

  if (hover > 0.01 || checked) {
    QColor glow = checked ? palette.checked : palette.borderHover;
    glow.setAlphaF((checked ? 0.18 : 0.11) + hover * 0.08);
    painter.setPen(Qt::NoPen);
    painter.setBrush(glow);
    painter.drawRoundedRect(rect.adjusted(-1.5, -1.5, 1.5, 1.5),
                            palette.radius + 1.5, palette.radius + 1.5);
  }

  painter.setPen(QPen(border, 1.2));
  painter.setBrush(fill);
  painter.drawRoundedRect(rect, palette.radius, palette.radius);
  painter.restore();
}

void paintToolButtonLabel(QPainter &painter, AnimatedToolButton *button,
                          AnimatedButtonBase::Variant variant, qreal press) {
  QStyleOptionToolButton opt = button->buildStyleOption();
  const ButtonPaletteSet palette = paletteFor(button, variant);

  opt.state &= ~QStyle::State_MouseOver;
  opt.state &= ~QStyle::State_Sunken;
  opt.state &= ~QStyle::State_Raised;
  opt.palette.setColor(QPalette::ButtonText,
                       button->isChecked() ? palette.textChecked : palette.text);
  opt.palette.setColor(QPalette::WindowText,
                       button->isChecked() ? palette.textChecked : palette.text);

  painter.save();
  painter.translate(0.0, press * 1.0);
  button->style()->drawControl(QStyle::CE_ToolButtonLabel, &opt, &painter,
                               button);
  painter.restore();
}

void paintPushButtonLabel(QPainter &painter, AnimatedPushButton *button,
                          AnimatedButtonBase::Variant variant, qreal press) {
  QStyleOptionButton opt;
  opt.initFrom(button);
  opt.features = QStyleOptionButton::None;
  if (button->isDefault())
    opt.features |= QStyleOptionButton::DefaultButton;
  if (button->isFlat())
    opt.features |= QStyleOptionButton::Flat;
  if (button->menu())
    opt.features |= QStyleOptionButton::HasMenu;
  opt.text = button->text();
  opt.icon = button->icon();
  opt.iconSize = button->iconSize();

  const ButtonPaletteSet palette = paletteFor(button, variant);
  opt.palette.setColor(QPalette::ButtonText,
                       button->isChecked() ? palette.textChecked : palette.text);
  opt.palette.setColor(QPalette::WindowText,
                       button->isChecked() ? palette.textChecked : palette.text);

  painter.save();
  painter.translate(0.0, press * 1.0);
  button->style()->drawControl(QStyle::CE_PushButtonLabel, &opt, &painter,
                               button);
  painter.restore();
}

} // namespace

AnimatedButtonBase::AnimatedButtonBase(QWidget *widget) : widget_(widget) {
  hoverAnimation_.setDuration(160);
  hoverAnimation_.setEasingCurve(QEasingCurve::OutCubic);
  hoverAnimation_.setStartValue(0.0);
  hoverAnimation_.setEndValue(1.0);
  QObject::connect(&hoverAnimation_, &QVariantAnimation::valueChanged, widget_,
                   [this](const QVariant &value) {
                     hoverProgress_ = value.toReal();
                     if (widget_)
                       widget_->update();
                   });

  pressAnimation_.setDuration(110);
  pressAnimation_.setEasingCurve(QEasingCurve::OutCubic);
  pressAnimation_.setStartValue(0.0);
  pressAnimation_.setEndValue(1.0);
  QObject::connect(&pressAnimation_, &QVariantAnimation::valueChanged, widget_,
                   [this](const QVariant &value) {
                     pressProgress_ = value.toReal();
                     if (widget_)
                       widget_->update();
                   });
}

void AnimatedButtonBase::setVariant(Variant variant) {
  if (variant_ == variant)
    return;
  variant_ = variant;
  if (widget_)
    widget_->update();
}

void AnimatedButtonBase::handleEnter() { animateTo(hoverAnimation_, 1.0); }

void AnimatedButtonBase::handleLeave() { animateTo(hoverAnimation_, 0.0); }

void AnimatedButtonBase::handlePress() { animateTo(pressAnimation_, 1.0); }

void AnimatedButtonBase::handleRelease() { animateTo(pressAnimation_, 0.0); }

void AnimatedButtonBase::updateForCheckedState(bool) {
  if (widget_)
    widget_->update();
}

void AnimatedButtonBase::animateTo(QVariantAnimation &animation, qreal target) {
  animation.stop();
  animation.setStartValue(animation.currentValue().isValid() ? animation.currentValue()
                                                             : QVariant(target));
  animation.setEndValue(target);
  animation.start();
}

AnimatedToolButton::AnimatedToolButton(QWidget *parent)
    : QToolButton(parent), animation_(this) {
  setCursor(Qt::PointingHandCursor);
}

void AnimatedToolButton::setVariant(AnimatedButtonBase::Variant variant) {
  animation_.setVariant(variant);
}

QStyleOptionToolButton AnimatedToolButton::buildStyleOption() const {
  QStyleOptionToolButton opt;
  initStyleOption(&opt);
  return opt;
}

bool AnimatedToolButton::event(QEvent *event) {
  if (event) {
    switch (event->type()) {
    case QEvent::Enter:
      animation_.handleEnter();
      break;
    case QEvent::Leave:
      animation_.handleLeave();
      animation_.handleRelease();
      break;
    case QEvent::EnabledChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
      update();
      break;
    default:
      break;
    }
  }
  return QToolButton::event(event);
}

void AnimatedToolButton::mousePressEvent(QMouseEvent *event) {
  if (event && event->button() == Qt::LeftButton) {
    animation_.handlePress();
  }
  QToolButton::mousePressEvent(event);
}

void AnimatedToolButton::mouseReleaseEvent(QMouseEvent *event) {
  animation_.handleRelease();
  QToolButton::mouseReleaseEvent(event);
  animation_.updateForCheckedState(isChecked());
}

void AnimatedToolButton::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  paintButtonSurface(painter, this, animation_.variant(),
                     animation_.hoverProgress(), animation_.pressProgress(),
                     isChecked(), isEnabled());
  paintToolButtonLabel(painter, this, animation_.variant(),
                       animation_.pressProgress());
}

AnimatedPushButton::AnimatedPushButton(QWidget *parent)
    : QPushButton(parent), animation_(this) {
  setCursor(Qt::PointingHandCursor);
}

AnimatedPushButton::AnimatedPushButton(const QString &text, QWidget *parent)
    : QPushButton(text, parent), animation_(this) {
  setCursor(Qt::PointingHandCursor);
}

void AnimatedPushButton::setVariant(AnimatedButtonBase::Variant variant) {
  animation_.setVariant(variant);
}

bool AnimatedPushButton::event(QEvent *event) {
  if (event) {
    switch (event->type()) {
    case QEvent::Enter:
      animation_.handleEnter();
      break;
    case QEvent::Leave:
      animation_.handleLeave();
      animation_.handleRelease();
      break;
    case QEvent::EnabledChange:
    case QEvent::PaletteChange:
    case QEvent::StyleChange:
      update();
      break;
    default:
      break;
    }
  }
  return QPushButton::event(event);
}

void AnimatedPushButton::mousePressEvent(QMouseEvent *event) {
  if (event && event->button() == Qt::LeftButton) {
    animation_.handlePress();
  }
  QPushButton::mousePressEvent(event);
}

void AnimatedPushButton::mouseReleaseEvent(QMouseEvent *event) {
  animation_.handleRelease();
  QPushButton::mouseReleaseEvent(event);
  animation_.updateForCheckedState(isChecked());
}

void AnimatedPushButton::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  paintButtonSurface(painter, this, animation_.variant(),
                     animation_.hoverProgress(), animation_.pressProgress(),
                     isChecked(), isEnabled());
  paintPushButtonLabel(painter, this, animation_.variant(),
                       animation_.pressProgress());
}
