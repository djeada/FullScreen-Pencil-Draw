/**
 * @file dock_title_bar.cpp
 * @brief Custom dock title bar implementation.
 */
#include "dock_title_bar.h"
#include "../core/theme_manager.h"
#include "animated_button.h"
#include <QDockWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMouseEvent>
#include <QToolButton>

namespace {

constexpr char kRestoreViaSideTabProperty[] = "restoreViaSideTab";

}

DockTitleBar::DockTitleBar(QDockWidget *dock, QWidget *parent)
    : QWidget(parent), dock_(dock), titleLabel_(nullptr),
      collapseButton_(nullptr), floatButton_(nullptr), closeButton_(nullptr) {
  setObjectName(QStringLiteral("dockTitleBar"));
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setMinimumHeight(36);

  auto *layout = new QHBoxLayout(this);
  layout->setContentsMargins(12, 4, 6, 4);
  layout->setSpacing(4);

  titleLabel_ = new QLabel(this);
  titleLabel_->setObjectName(QStringLiteral("dockTitleLabel"));
  titleLabel_->setAttribute(Qt::WA_TransparentForMouseEvents);
  titleLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
  layout->addWidget(titleLabel_);

  collapseButton_ = new AnimatedToolButton(this);
  collapseButton_->setToolTip("Collapse to side handle");
  static_cast<AnimatedToolButton *>(collapseButton_)
      ->setVariant(AnimatedButtonBase::Variant::TitleBar);
  layout->addWidget(collapseButton_);

  floatButton_ = new AnimatedToolButton(this);
  floatButton_->setText(QStringLiteral("□"));
  floatButton_->setToolTip("Toggle floating panel");
  floatButton_->setCheckable(true);
  static_cast<AnimatedToolButton *>(floatButton_)
      ->setVariant(AnimatedButtonBase::Variant::TitleBar);
  layout->addWidget(floatButton_);

  closeButton_ = new AnimatedToolButton(this);
  closeButton_->setText(QStringLiteral("✕"));
  closeButton_->setToolTip("Hide panel");
  static_cast<AnimatedToolButton *>(closeButton_)
      ->setVariant(AnimatedButtonBase::Variant::TitleBar);
  layout->addWidget(closeButton_);

  connect(collapseButton_, &QToolButton::clicked, this, [this]() {
    if (!dock_) {
      return;
    }
    dock_->setProperty(kRestoreViaSideTabProperty, true);
    dock_->hide();
  });
  connect(closeButton_, &QToolButton::clicked, this, [this]() {
    if (!dock_) {
      return;
    }
    dock_->setProperty(kRestoreViaSideTabProperty, false);
    dock_->hide();
  });
  connect(floatButton_, &QToolButton::clicked, this, [this]() {
    if (dock_) {
      dock_->setFloating(!dock_->isFloating());
    }
  });

  connect(dock_, &QDockWidget::windowTitleChanged, this,
          [this]() { updateTitle(); });
  connect(dock_, &QDockWidget::featuresChanged, this,
          [this]() { updateButtons(); });
  connect(dock_, &QDockWidget::topLevelChanged, this, [this](bool floating) {
    floatButton_->setChecked(floating);
    updateCollapseGlyph();
  });
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
  connect(dock_, &QDockWidget::dockLocationChanged, this,
          [this](Qt::DockWidgetArea) { updateCollapseGlyph(); });
#endif
  connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this,
          [this]() { applyTheme(); });

  updateTitle();
  updateButtons();
  updateCollapseGlyph();
  applyTheme();
}

void DockTitleBar::mousePressEvent(QMouseEvent *event) { event->ignore(); }

void DockTitleBar::mouseMoveEvent(QMouseEvent *event) { event->ignore(); }

void DockTitleBar::mouseReleaseEvent(QMouseEvent *event) { event->ignore(); }

void DockTitleBar::mouseDoubleClickEvent(QMouseEvent *event) {
  event->ignore();
}

void DockTitleBar::applyTheme() {
  const bool darkTheme = ThemeManager::instance().isDarkTheme();
  setStyleSheet(darkTheme ? R"(
              #dockTitleBar {
                background-color: #0f1822;
                border-bottom: 1px solid rgba(255, 244, 230, 0.06);
              }
              QLabel#dockTitleLabel {
                color: #f4e6d6;
                font-size: 11px;
                font-weight: 700;
                letter-spacing: 0.3px;
                padding-left: 2px;
              }
              #dockTitleBar QToolButton {
                background-color: transparent;
                color: #d9c8b6;
                border: 1px solid transparent;
                border-radius: 8px;
                min-width: 22px;
                min-height: 22px;
                max-width: 22px;
                max-height: 22px;
                font-size: 11px;
                font-weight: 700;
              }
              #dockTitleBar QToolButton:hover {
                background-color: rgba(255, 255, 255, 0.05);
                color: #fff7ed;
                border: 1px solid rgba(249, 115, 22, 0.22);
              }
              #dockTitleBar QToolButton:pressed {
                background-color: rgba(249, 115, 22, 0.18);
              }
              #dockTitleBar QToolButton:checked {
                background-color: rgba(249, 115, 22, 0.22);
                color: #fffaf4;
                border: 1px solid rgba(249, 115, 22, 0.38);
              }
            )"
                          : R"(
              #dockTitleBar {
                background-color: #f6efe6;
                border-bottom: 1px solid #dfd1c2;
              }
              QLabel#dockTitleLabel {
                color: #34281f;
                font-size: 11px;
                font-weight: 700;
                letter-spacing: 0.3px;
                padding-left: 2px;
              }
              #dockTitleBar QToolButton {
                background-color: transparent;
                color: #6d5a48;
                border: 1px solid transparent;
                border-radius: 8px;
                min-width: 22px;
                min-height: 22px;
                max-width: 22px;
                max-height: 22px;
                font-size: 11px;
                font-weight: 700;
              }
              #dockTitleBar QToolButton:hover {
                background-color: rgba(255, 255, 255, 0.72);
                color: #2f241c;
                border: 1px solid rgba(234, 88, 12, 0.18);
              }
              #dockTitleBar QToolButton:pressed {
                background-color: #f1ddc8;
              }
              #dockTitleBar QToolButton:checked {
                background-color: #f6dfca;
                color: #fffaf4;
                border: 1px solid rgba(234, 88, 12, 0.2);
              }
            )");
}

void DockTitleBar::updateButtons() {
  if (!dock_) {
    return;
  }

  const auto features = dock_->features();
  collapseButton_->setVisible(
      features.testFlag(QDockWidget::DockWidgetClosable));
  closeButton_->setVisible(features.testFlag(QDockWidget::DockWidgetClosable));
  floatButton_->setVisible(features.testFlag(QDockWidget::DockWidgetFloatable));
  floatButton_->setChecked(dock_->isFloating());
}

void DockTitleBar::updateTitle() {
  if (dock_ && titleLabel_) {
    titleLabel_->setText(dock_->windowTitle());
  }
}

void DockTitleBar::updateCollapseGlyph() {
  if (!dock_ || !collapseButton_) {
    return;
  }

  collapseButton_->setText(QStringLiteral("‹"));
}
