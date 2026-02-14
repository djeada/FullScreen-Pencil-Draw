// theme_manager.cpp
#include "theme_manager.h"
#include "app_constants.h"
#include <QApplication>
#include <QSettings>
#include <QStyleFactory>

ThemeManager &ThemeManager::instance() {
  static ThemeManager instance;
  return instance;
}

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent), currentTheme_(Dark) {
  loadThemePreference();
}

ThemeManager::Theme ThemeManager::currentTheme() const { return currentTheme_; }

bool ThemeManager::isDarkTheme() const { return currentTheme_ == Dark; }

void ThemeManager::setTheme(Theme theme) {
  if (currentTheme_ == theme)
    return;

  currentTheme_ = theme;

  if (theme == Dark) {
    applyDarkTheme();
  } else {
    applyLightTheme();
  }

  saveThemePreference();
  emit themeChanged(currentTheme_);
}

void ThemeManager::toggleTheme() {
  setTheme(currentTheme_ == Dark ? Light : Dark);
}

void ThemeManager::applyDarkTheme() {
  QPalette darkPalette;

  // Base colors - Modern flat design palette with refined tones
  QColor darkGray(32, 32, 36); // Slightly darker for more depth
  QColor gray(48, 48, 54);     // More refined gray
  QColor lightGray(128, 128, 134);
  QColor white(248, 248, 252);     // Slightly warmer white
  QColor accentBlue(59, 130, 246); // More vibrant accent
  QColor accentBlueHover(96, 165, 250);
  QColor accentBluePressed(37, 99, 235);
  QColor darkBlue(22, 22, 26);
  QColor surfaceColor(38, 38, 43); // Card/surface color
  QColor borderColor(55, 55, 62);  // Subtle borders

  darkPalette.setColor(QPalette::Window, darkGray);
  darkPalette.setColor(QPalette::WindowText, white);
  darkPalette.setColor(QPalette::Base, darkBlue);
  darkPalette.setColor(QPalette::AlternateBase, darkGray);
  darkPalette.setColor(QPalette::ToolTipBase, gray);
  darkPalette.setColor(QPalette::ToolTipText, white);
  darkPalette.setColor(QPalette::Text, white);
  darkPalette.setColor(QPalette::Button, darkGray);
  darkPalette.setColor(QPalette::ButtonText, white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, accentBlue);
  darkPalette.setColor(QPalette::Highlight, accentBlue);
  darkPalette.setColor(QPalette::HighlightedText, Qt::white);

  // Disabled colors
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, lightGray);
  darkPalette.setColor(QPalette::Disabled, QPalette::Text, lightGray);
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, lightGray);

  qApp->setPalette(darkPalette);

  // Apply comprehensive modern flat stylesheet with enhanced hover effects
  QString styleSheet = R"(
    /* ===== GLOBAL STYLES ===== */
    * {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    }

    /* ===== TOOLTIPS ===== */
    QToolTip {
      color: #f8f8fc;
      background-color: rgba(48, 48, 54, 0.95);
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 8px;
      padding: 10px 14px;
      font-size: 12px;
    }

    /* ===== MENUS ===== */
    QMenu {
      background-color: #202024;
      color: #f8f8fc;
      border: 1px solid #37373e;
      border-radius: 10px;
      padding: 8px;
    }
    QMenu::item {
      padding: 10px 36px 10px 16px;
      border-radius: 6px;
      margin: 2px 4px;
    }
    QMenu::item:selected {
      background-color: #3b82f6;
      color: #ffffff;
    }
    QMenu::item:hover {
      background-color: rgba(59, 130, 246, 0.15);
    }
    QMenu::separator {
      height: 1px;
      background-color: #37373e;
      margin: 8px 10px;
    }
    QMenuBar {
      background-color: #202024;
      color: #f8f8fc;
      padding: 6px 8px;
      border-bottom: 1px solid #37373e;
    }
    QMenuBar::item {
      padding: 8px 14px;
      border-radius: 6px;
      margin: 2px;
    }
    QMenuBar::item:selected {
      background-color: rgba(255, 255, 255, 0.08);
    }
    QMenuBar::item:pressed {
      background-color: #3b82f6;
    }

    /* ===== PUSH BUTTONS ===== */
    QPushButton {
      background-color: #303036;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 8px;
      padding: 10px 18px;
      font-weight: 500;
      min-height: 26px;
    }
    QPushButton:hover {
      background-color: #3c3c44;
      border: 1px solid rgba(59, 130, 246, 0.3);
    }
    QPushButton:pressed {
      background-color: #28282c;
      border: 1px solid rgba(59, 130, 246, 0.5);
    }
    QPushButton:checked {
      background-color: #3b82f6;
      color: #ffffff;
      border: 1px solid #60a5fa;
    }
    QPushButton:checked:hover {
      background-color: #60a5fa;
    }
    QPushButton:disabled {
      background-color: #242428;
      color: #555560;
      border: 1px solid rgba(255, 255, 255, 0.03);
    }
    QPushButton:focus {
      outline: none;
      border: 2px solid #3b82f6;
    }

    /* ===== TOOL BUTTONS ===== */
    QToolButton {
      background-color: transparent;
      color: #e0e0e6;
      border: 1px solid transparent;
      border-radius: 8px;
      padding: 10px;
      margin: 2px;
    }
    QToolButton:hover {
      background-color: rgba(255, 255, 255, 0.08);
      border: 1px solid rgba(255, 255, 255, 0.1);
      color: #f8f8fc;
    }
    QToolButton:pressed {
      background-color: rgba(255, 255, 255, 0.04);
    }
    QToolButton:checked {
      background-color: #3b82f6;
      color: #ffffff;
      border: 1px solid #60a5fa;
    }
    QToolButton:checked:hover {
      background-color: #60a5fa;
    }
    QToolButton::menu-indicator {
      image: none;
    }

    /* ===== TOOLBARS ===== */
    QToolBar {
      background-color: #1a1a1e;
      border: none;
      border-right: 1px solid #37373e;
      spacing: 4px;
      padding: 10px 8px;
    }
    QToolBar::separator {
      width: 1px;
      background-color: rgba(255, 255, 255, 0.08);
      margin: 10px 8px;
    }

    /* ===== SLIDERS ===== */
    QSlider::groove:horizontal {
      background: #28282e;
      height: 8px;
      border-radius: 4px;
    }
    QSlider::handle:horizontal {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #60a5fa, stop:1 #3b82f6);
      width: 18px;
      height: 18px;
      margin: -5px 0;
      border-radius: 9px;
      border: 2px solid rgba(255, 255, 255, 0.15);
    }
    QSlider::handle:horizontal:hover {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #93c5fd, stop:1 #60a5fa);
      width: 20px;
      height: 20px;
      margin: -6px 0;
      border-radius: 10px;
    }
    QSlider::handle:horizontal:pressed {
      background: #2563eb;
    }
    QSlider::sub-page:horizontal {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa);
      border-radius: 4px;
    }
    QSlider::groove:vertical {
      background: #28282e;
      width: 8px;
      border-radius: 4px;
    }
    QSlider::handle:vertical {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #60a5fa, stop:1 #3b82f6);
      width: 18px;
      height: 18px;
      margin: 0 -5px;
      border-radius: 9px;
      border: 2px solid rgba(255, 255, 255, 0.15);
    }
    QSlider::handle:vertical:hover {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #93c5fd, stop:1 #60a5fa);
    }

    /* ===== SCROLLBARS ===== */
    QScrollBar:vertical {
      background: transparent;
      width: 10px;
      margin: 0px;
      border-radius: 5px;
    }
    QScrollBar::handle:vertical {
      background: rgba(255, 255, 255, 0.15);
      min-height: 40px;
      border-radius: 4px;
      margin: 2px;
    }
    QScrollBar::handle:vertical:hover {
      background: rgba(255, 255, 255, 0.25);
    }
    QScrollBar:horizontal {
      background: transparent;
      height: 10px;
      margin: 0px;
      border-radius: 5px;
    }
    QScrollBar::handle:horizontal {
      background: rgba(255, 255, 255, 0.15);
      min-width: 40px;
      border-radius: 4px;
      margin: 2px;
    }
    QScrollBar::handle:horizontal:hover {
      background: rgba(255, 255, 255, 0.25);
    }
    QScrollBar::add-line, QScrollBar::sub-line {
      border: none;
      background: none;
      width: 0px;
      height: 0px;
    }
    QScrollBar::add-page, QScrollBar::sub-page {
      background: none;
    }

    /* ===== DOCK WIDGETS ===== */
    QDockWidget {
      color: #f8f8fc;
      font-weight: 500;
    }
    QDockWidget::title {
      background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2a2a30, stop:1 #242428);
      padding: 12px 14px;
      border-radius: 0px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.06);
      font-weight: 600;
    }
    QDockWidget::close-button, QDockWidget::float-button {
      background: transparent;
      border: none;
      border-radius: 6px;
      padding: 4px;
    }
    QDockWidget::close-button:hover, QDockWidget::float-button:hover {
      background: rgba(255, 255, 255, 0.1);
    }

    /* ===== LIST WIDGETS ===== */
    QListWidget {
      background-color: #161618;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 10px;
      padding: 6px;
      outline: none;
    }
    QListWidget::item {
      padding: 10px 14px;
      border-radius: 8px;
      margin: 2px;
    }
    QListWidget::item:hover {
      background-color: rgba(255, 255, 255, 0.06);
    }
    QListWidget::item:selected {
      background-color: #3b82f6;
      color: #ffffff;
    }

    /* ===== GROUP BOXES ===== */
    QGroupBox {
      color: #a0a0a8;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 10px;
      margin-top: 18px;
      padding-top: 14px;
      font-weight: 500;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 14px;
      padding: 0 10px;
      color: #f8f8fc;
    }

    /* ===== LABELS ===== */
    QLabel {
      color: #f8f8fc;
    }

    /* ===== LINE EDITS ===== */
    QLineEdit {
      background-color: #161618;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 8px;
      padding: 10px 14px;
      selection-background-color: #3b82f6;
    }
    QLineEdit:focus {
      border: 2px solid #3b82f6;
    }
    QLineEdit:hover {
      border: 1px solid rgba(59, 130, 246, 0.4);
    }

    /* ===== SPIN BOXES ===== */
    QSpinBox, QDoubleSpinBox {
      background-color: #161618;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 8px;
      padding: 8px 12px;
    }
    QSpinBox:focus, QDoubleSpinBox:focus {
      border: 2px solid #3b82f6;
    }
    QSpinBox:hover, QDoubleSpinBox:hover {
      border: 1px solid rgba(59, 130, 246, 0.4);
    }
    QSpinBox::up-button, QDoubleSpinBox::up-button {
      background-color: rgba(255, 255, 255, 0.08);
      border: none;
      border-radius: 6px;
      margin: 2px;
      width: 22px;
    }
    QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
      background-color: rgba(59, 130, 246, 0.3);
    }
    QSpinBox::down-button, QDoubleSpinBox::down-button {
      background-color: rgba(255, 255, 255, 0.08);
      border: none;
      border-radius: 6px;
      margin: 2px;
      width: 22px;
    }
    QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
      background-color: rgba(59, 130, 246, 0.3);
    }

    /* ===== COMBO BOXES ===== */
    QComboBox {
      background-color: #303036;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 8px;
      padding: 10px 14px;
      min-width: 90px;
    }
    QComboBox:hover {
      background-color: #3c3c44;
      border: 1px solid rgba(59, 130, 246, 0.3);
    }
    QComboBox::drop-down {
      border: none;
      padding-right: 10px;
    }
    QComboBox QAbstractItemView {
      background-color: #202024;
      color: #f8f8fc;
      border: 1px solid rgba(255, 255, 255, 0.08);
      border-radius: 8px;
      selection-background-color: #3b82f6;
    }

    /* ===== CHECK BOXES ===== */
    QCheckBox {
      color: #f8f8fc;
      spacing: 10px;
    }
    QCheckBox::indicator {
      width: 22px;
      height: 22px;
      border-radius: 6px;
      border: 2px solid rgba(255, 255, 255, 0.2);
      background-color: transparent;
    }
    QCheckBox::indicator:hover {
      border: 2px solid #3b82f6;
      background-color: rgba(59, 130, 246, 0.1);
    }
    QCheckBox::indicator:checked {
      background-color: #3b82f6;
      border: 2px solid #3b82f6;
    }

    /* ===== RADIO BUTTONS ===== */
    QRadioButton {
      color: #f8f8fc;
      spacing: 10px;
    }
    QRadioButton::indicator {
      width: 22px;
      height: 22px;
      border-radius: 11px;
      border: 2px solid rgba(255, 255, 255, 0.2);
      background-color: transparent;
    }
    QRadioButton::indicator:hover {
      border: 2px solid #3b82f6;
      background-color: rgba(59, 130, 246, 0.1);
    }
    QRadioButton::indicator:checked {
      background-color: #3b82f6;
      border: 2px solid #3b82f6;
    }

    /* ===== TAB WIDGETS ===== */
    QTabWidget::pane {
      border: 1px solid rgba(255, 255, 255, 0.06);
      border-radius: 10px;
      padding: 10px;
    }
    QTabBar::tab {
      background-color: #28282e;
      color: #a0a0a8;
      padding: 12px 22px;
      border-radius: 8px 8px 0 0;
      margin-right: 3px;
    }
    QTabBar::tab:hover {
      background-color: #3c3c44;
      color: #f8f8fc;
    }
    QTabBar::tab:selected {
      background-color: #3b82f6;
      color: #ffffff;
    }

    /* ===== PROGRESS BARS ===== */
    QProgressBar {
      background-color: #28282e;
      border: none;
      border-radius: 6px;
      height: 10px;
      text-align: center;
    }
    QProgressBar::chunk {
      background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #3b82f6, stop:1 #60a5fa);
      border-radius: 6px;
    }

    /* ===== DIALOGS ===== */
    QDialog {
      background-color: #202024;
    }
    QDialogButtonBox QPushButton {
      min-width: 90px;
    }

    /* ===== STATUS BAR ===== */
    QStatusBar {
      background-color: #161618;
      color: #a0a0a8;
      border-top: 1px solid rgba(255, 255, 255, 0.06);
      padding: 6px 10px;
    }
    QStatusBar::item {
      border: none;
    }

    /* ===== MAIN WINDOW ===== */
    QMainWindow {
      background-color: #121214;
    }
    QMainWindow::separator {
      background-color: rgba(255, 255, 255, 0.08);
      width: 2px;
      height: 2px;
    }
    QMainWindow::separator:hover {
      background-color: #3b82f6;
    }

    /* ===== FRAMES ===== */
    QFrame {
      border: none;
    }
    QFrame[frameShape="4"], QFrame[frameShape="5"] {
      background-color: rgba(255, 255, 255, 0.06);
    }

    /* ===== SPLITTERS ===== */
    QSplitter::handle {
      background-color: rgba(255, 255, 255, 0.06);
    }
    QSplitter::handle:hover {
      background-color: rgba(59, 130, 246, 0.4);
    }
    QSplitter::handle:pressed {
      background-color: #3b82f6;
    }
  )";

  qApp->setStyleSheet(styleSheet);
}

void ThemeManager::applyLightTheme() {
  QPalette lightPalette;

  // Base colors - Modern flat design palette with refined tones
  QColor lightGray(250, 250, 252);
  QColor white(255, 255, 255);
  QColor darkGray(45, 52, 60);
  QColor accentBlue(59, 130, 246);
  QColor midGray(206, 212, 218);
  QColor textGray(73, 80, 87);

  lightPalette.setColor(QPalette::Window, lightGray);
  lightPalette.setColor(QPalette::WindowText, darkGray);
  lightPalette.setColor(QPalette::Base, white);
  lightPalette.setColor(QPalette::AlternateBase, lightGray);
  lightPalette.setColor(QPalette::ToolTipBase, white);
  lightPalette.setColor(QPalette::ToolTipText, darkGray);
  lightPalette.setColor(QPalette::Text, darkGray);
  lightPalette.setColor(QPalette::Button, lightGray);
  lightPalette.setColor(QPalette::ButtonText, darkGray);
  lightPalette.setColor(QPalette::BrightText, Qt::red);
  lightPalette.setColor(QPalette::Link, accentBlue);
  lightPalette.setColor(QPalette::Highlight, accentBlue);
  lightPalette.setColor(QPalette::HighlightedText, white);

  // Disabled colors
  lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, midGray);
  lightPalette.setColor(QPalette::Disabled, QPalette::Text, midGray);
  lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, midGray);

  qApp->setPalette(lightPalette);

  // Apply comprehensive modern flat stylesheet
  QString styleSheet = R"(
    /* ===== GLOBAL STYLES ===== */
    * {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Helvetica Neue", Arial, sans-serif;
    }

    /* ===== TOOLTIPS ===== */
    QToolTip {
      color: #343a40;
      background-color: #ffffff;
      border: 1px solid #ced4da;
      border-radius: 6px;
      padding: 8px 12px;
      font-size: 12px;
    }

    /* ===== MENUS ===== */
    QMenu {
      background-color: #ffffff;
      color: #343a40;
      border: 1px solid #ced4da;
      border-radius: 8px;
      padding: 6px;
    }
    QMenu::item {
      padding: 8px 32px 8px 16px;
      border-radius: 4px;
      margin: 2px 4px;
    }
    QMenu::item:selected {
      background-color: #4285f4;
      color: #ffffff;
    }
    QMenu::separator {
      height: 1px;
      background-color: #e9ecef;
      margin: 6px 8px;
    }
    QMenuBar {
      background-color: #f8f9fa;
      color: #343a40;
      padding: 4px;
      border-bottom: 1px solid #e9ecef;
    }
    QMenuBar::item {
      padding: 8px 12px;
      border-radius: 6px;
      margin: 2px;
    }
    QMenuBar::item:selected {
      background-color: #e9ecef;
    }
    QMenuBar::item:pressed {
      background-color: #4285f4;
      color: #ffffff;
    }

    /* ===== PUSH BUTTONS ===== */
    QPushButton {
      background-color: #e9ecef;
      color: #343a40;
      border: none;
      border-radius: 6px;
      padding: 8px 16px;
      font-weight: 500;
      min-height: 24px;
    }
    QPushButton:hover {
      background-color: #dee2e6;
    }
    QPushButton:pressed {
      background-color: #ced4da;
    }
    QPushButton:checked {
      background-color: #4285f4;
      color: #ffffff;
    }
    QPushButton:checked:hover {
      background-color: #5c9bff;
    }
    QPushButton:disabled {
      background-color: #f8f9fa;
      color: #adb5bd;
    }
    QPushButton:focus {
      outline: none;
      border: 2px solid #4285f4;
    }

    /* ===== TOOL BUTTONS ===== */
    QToolButton {
      background-color: transparent;
      color: #343a40;
      border: none;
      border-radius: 6px;
      padding: 8px;
      margin: 2px;
    }
    QToolButton:hover {
      background-color: #e9ecef;
    }
    QToolButton:pressed {
      background-color: #ced4da;
    }
    QToolButton:checked {
      background-color: #4285f4;
      color: #ffffff;
    }
    QToolButton:checked:hover {
      background-color: #5c9bff;
    }
    QToolButton::menu-indicator {
      image: none;
    }

    /* ===== TOOLBARS ===== */
    QToolBar {
      background-color: #f8f9fa;
      border: none;
      spacing: 4px;
      padding: 6px;
    }
    QToolBar::separator {
      width: 1px;
      background-color: #e9ecef;
      margin: 8px 6px;
    }

    /* ===== SLIDERS ===== */
    QSlider::groove:horizontal {
      background: #e9ecef;
      height: 6px;
      border-radius: 3px;
    }
    QSlider::handle:horizontal {
      background: #4285f4;
      width: 16px;
      height: 16px;
      margin: -5px 0;
      border-radius: 8px;
    }
    QSlider::handle:horizontal:hover {
      background: #5c9bff;
      width: 18px;
      height: 18px;
      margin: -6px 0;
      border-radius: 9px;
    }
    QSlider::handle:horizontal:pressed {
      background: #306ccc;
    }
    QSlider::sub-page:horizontal {
      background: #4285f4;
      border-radius: 3px;
    }
    QSlider::groove:vertical {
      background: #e9ecef;
      width: 6px;
      border-radius: 3px;
    }
    QSlider::handle:vertical {
      background: #4285f4;
      width: 16px;
      height: 16px;
      margin: 0 -5px;
      border-radius: 8px;
    }
    QSlider::handle:vertical:hover {
      background: #5c9bff;
    }

    /* ===== SCROLLBARS ===== */
    QScrollBar:vertical {
      background: transparent;
      width: 12px;
      margin: 0px;
      border-radius: 6px;
    }
    QScrollBar::handle:vertical {
      background: #ced4da;
      min-height: 30px;
      border-radius: 5px;
      margin: 2px;
    }
    QScrollBar::handle:vertical:hover {
      background: #adb5bd;
    }
    QScrollBar:horizontal {
      background: transparent;
      height: 12px;
      margin: 0px;
      border-radius: 6px;
    }
    QScrollBar::handle:horizontal {
      background: #ced4da;
      min-width: 30px;
      border-radius: 5px;
      margin: 2px;
    }
    QScrollBar::handle:horizontal:hover {
      background: #adb5bd;
    }
    QScrollBar::add-line, QScrollBar::sub-line {
      border: none;
      background: none;
      width: 0px;
      height: 0px;
    }
    QScrollBar::add-page, QScrollBar::sub-page {
      background: none;
    }

    /* ===== DOCK WIDGETS ===== */
    QDockWidget {
      color: #343a40;
      font-weight: 500;
    }
    QDockWidget::title {
      background: #e9ecef;
      padding: 10px 12px;
      border-radius: 0px;
      font-weight: 600;
    }
    QDockWidget::close-button, QDockWidget::float-button {
      background: transparent;
      border: none;
      border-radius: 4px;
      padding: 4px;
    }
    QDockWidget::close-button:hover, QDockWidget::float-button:hover {
      background: #dee2e6;
    }

    /* ===== LIST WIDGETS ===== */
    QListWidget {
      background-color: #ffffff;
      color: #343a40;
      border: 1px solid #e9ecef;
      border-radius: 8px;
      padding: 4px;
      outline: none;
    }
    QListWidget::item {
      padding: 8px 12px;
      border-radius: 6px;
      margin: 2px;
    }
    QListWidget::item:hover {
      background-color: #f8f9fa;
    }
    QListWidget::item:selected {
      background-color: #4285f4;
      color: #ffffff;
    }

    /* ===== GROUP BOXES ===== */
    QGroupBox {
      color: #6c757d;
      border: 1px solid #e9ecef;
      border-radius: 8px;
      margin-top: 16px;
      padding-top: 12px;
      font-weight: 500;
    }
    QGroupBox::title {
      subcontrol-origin: margin;
      left: 12px;
      padding: 0 8px;
      color: #343a40;
    }

    /* ===== LABELS ===== */
    QLabel {
      color: #343a40;
    }

    /* ===== LINE EDITS ===== */
    QLineEdit {
      background-color: #ffffff;
      color: #343a40;
      border: 1px solid #ced4da;
      border-radius: 6px;
      padding: 8px 12px;
      selection-background-color: #4285f4;
    }
    QLineEdit:focus {
      border: 2px solid #4285f4;
    }
    QLineEdit:hover {
      border: 1px solid #adb5bd;
    }

    /* ===== SPIN BOXES ===== */
    QSpinBox, QDoubleSpinBox {
      background-color: #ffffff;
      color: #343a40;
      border: 1px solid #ced4da;
      border-radius: 6px;
      padding: 6px 10px;
    }
    QSpinBox:focus, QDoubleSpinBox:focus {
      border: 2px solid #4285f4;
    }
    QSpinBox:hover, QDoubleSpinBox:hover {
      border: 1px solid #adb5bd;
    }
    QSpinBox::up-button, QDoubleSpinBox::up-button {
      background-color: #e9ecef;
      border: none;
      border-radius: 4px;
      margin: 2px;
      width: 20px;
    }
    QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {
      background-color: #dee2e6;
    }
    QSpinBox::down-button, QDoubleSpinBox::down-button {
      background-color: #e9ecef;
      border: none;
      border-radius: 4px;
      margin: 2px;
      width: 20px;
    }
    QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {
      background-color: #dee2e6;
    }

    /* ===== COMBO BOXES ===== */
    QComboBox {
      background-color: #e9ecef;
      color: #343a40;
      border: none;
      border-radius: 6px;
      padding: 8px 12px;
      min-width: 80px;
    }
    QComboBox:hover {
      background-color: #dee2e6;
    }
    QComboBox::drop-down {
      border: none;
      padding-right: 8px;
    }
    QComboBox QAbstractItemView {
      background-color: #ffffff;
      color: #343a40;
      border: 1px solid #ced4da;
      border-radius: 6px;
      selection-background-color: #4285f4;
    }

    /* ===== CHECK BOXES ===== */
    QCheckBox {
      color: #343a40;
      spacing: 8px;
    }
    QCheckBox::indicator {
      width: 20px;
      height: 20px;
      border-radius: 4px;
      border: 2px solid #adb5bd;
      background-color: transparent;
    }
    QCheckBox::indicator:hover {
      border: 2px solid #4285f4;
    }
    QCheckBox::indicator:checked {
      background-color: #4285f4;
      border: 2px solid #4285f4;
    }

    /* ===== RADIO BUTTONS ===== */
    QRadioButton {
      color: #343a40;
      spacing: 8px;
    }
    QRadioButton::indicator {
      width: 20px;
      height: 20px;
      border-radius: 10px;
      border: 2px solid #adb5bd;
      background-color: transparent;
    }
    QRadioButton::indicator:hover {
      border: 2px solid #4285f4;
    }
    QRadioButton::indicator:checked {
      background-color: #4285f4;
      border: 2px solid #4285f4;
    }

    /* ===== TAB WIDGETS ===== */
    QTabWidget::pane {
      border: 1px solid #e9ecef;
      border-radius: 8px;
      padding: 8px;
    }
    QTabBar::tab {
      background-color: #e9ecef;
      color: #6c757d;
      padding: 10px 20px;
      border-radius: 6px 6px 0 0;
      margin-right: 2px;
    }
    QTabBar::tab:hover {
      background-color: #dee2e6;
      color: #343a40;
    }
    QTabBar::tab:selected {
      background-color: #4285f4;
      color: #ffffff;
    }

    /* ===== PROGRESS BARS ===== */
    QProgressBar {
      background-color: #e9ecef;
      border: none;
      border-radius: 6px;
      height: 8px;
      text-align: center;
    }
    QProgressBar::chunk {
      background-color: #4285f4;
      border-radius: 6px;
    }

    /* ===== DIALOGS ===== */
    QDialog {
      background-color: #f8f9fa;
    }
    QDialogButtonBox QPushButton {
      min-width: 80px;
    }

    /* ===== STATUS BAR ===== */
    QStatusBar {
      background-color: #f8f9fa;
      color: #6c757d;
      border-top: 1px solid #e9ecef;
    }
    QStatusBar::item {
      border: none;
    }

    /* ===== MAIN WINDOW ===== */
    QMainWindow {
      background-color: #ffffff;
    }
    QMainWindow::separator {
      background-color: #e9ecef;
      width: 2px;
      height: 2px;
    }
    QMainWindow::separator:hover {
      background-color: #4285f4;
    }
  )";

  qApp->setStyleSheet(styleSheet);
}

void ThemeManager::saveThemePreference() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  settings.setValue("theme", currentTheme_ == Dark ? "dark" : "light");
}

void ThemeManager::loadThemePreference() {
  QSettings settings(AppConstants::OrganizationName,
                     AppConstants::ApplicationName);
  QString themeName = settings.value("theme", "dark").toString();
  currentTheme_ = (themeName == "light") ? Light : Dark;

  // Apply the loaded theme
  if (currentTheme_ == Dark) {
    applyDarkTheme();
  } else {
    applyLightTheme();
  }
}
