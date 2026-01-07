// theme_manager.cpp
#include "theme_manager.h"
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

  // Base colors
  QColor darkGray(45, 45, 45);
  QColor gray(61, 61, 61);
  QColor lightGray(128, 128, 128);
  QColor white(255, 255, 255);
  QColor blue(42, 130, 218);
  QColor darkBlue(25, 25, 25);

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
  darkPalette.setColor(QPalette::Link, blue);
  darkPalette.setColor(QPalette::Highlight, blue);
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);

  // Disabled colors
  darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, lightGray);
  darkPalette.setColor(QPalette::Disabled, QPalette::Text, lightGray);
  darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, lightGray);

  qApp->setPalette(darkPalette);

  // Apply stylesheet for finer control
  QString styleSheet = R"(
    QToolTip {
      color: #ffffff;
      background-color: #3d3d3d;
      border: 1px solid #5a5a5a;
      padding: 4px;
    }
    QMenu {
      background-color: #2d2d2d;
      color: #ffffff;
      border: 1px solid #3d3d3d;
    }
    QMenu::item:selected {
      background-color: #2a82da;
    }
    QMenuBar {
      background-color: #2d2d2d;
      color: #ffffff;
    }
    QMenuBar::item:selected {
      background-color: #3d3d3d;
    }
    QScrollBar:vertical {
      background: #2d2d2d;
      width: 12px;
      margin: 0px;
    }
    QScrollBar::handle:vertical {
      background: #5a5a5a;
      min-height: 20px;
      border-radius: 4px;
    }
    QScrollBar::handle:vertical:hover {
      background: #6a6a6a;
    }
    QScrollBar:horizontal {
      background: #2d2d2d;
      height: 12px;
      margin: 0px;
    }
    QScrollBar::handle:horizontal {
      background: #5a5a5a;
      min-width: 20px;
      border-radius: 4px;
    }
    QScrollBar::handle:horizontal:hover {
      background: #6a6a6a;
    }
    QScrollBar::add-line, QScrollBar::sub-line {
      border: none;
      background: none;
    }
    QDockWidget {
      color: #ffffff;
      titlebar-close-icon: url(close.png);
      titlebar-normal-icon: url(undock.png);
    }
    QDockWidget::title {
      background: #3d3d3d;
      padding: 4px;
    }
  )";

  qApp->setStyleSheet(styleSheet);
}

void ThemeManager::applyLightTheme() {
  QPalette lightPalette;

  // Base colors
  QColor lightGray(240, 240, 240);
  QColor white(255, 255, 255);
  QColor darkGray(60, 60, 60);
  QColor blue(0, 120, 215);
  QColor midGray(200, 200, 200);

  lightPalette.setColor(QPalette::Window, lightGray);
  lightPalette.setColor(QPalette::WindowText, Qt::black);
  lightPalette.setColor(QPalette::Base, white);
  lightPalette.setColor(QPalette::AlternateBase, lightGray);
  lightPalette.setColor(QPalette::ToolTipBase, white);
  lightPalette.setColor(QPalette::ToolTipText, Qt::black);
  lightPalette.setColor(QPalette::Text, Qt::black);
  lightPalette.setColor(QPalette::Button, lightGray);
  lightPalette.setColor(QPalette::ButtonText, Qt::black);
  lightPalette.setColor(QPalette::BrightText, Qt::red);
  lightPalette.setColor(QPalette::Link, blue);
  lightPalette.setColor(QPalette::Highlight, blue);
  lightPalette.setColor(QPalette::HighlightedText, white);

  // Disabled colors
  lightPalette.setColor(QPalette::Disabled, QPalette::WindowText, midGray);
  lightPalette.setColor(QPalette::Disabled, QPalette::Text, midGray);
  lightPalette.setColor(QPalette::Disabled, QPalette::ButtonText, midGray);

  qApp->setPalette(lightPalette);

  // Apply stylesheet for finer control
  QString styleSheet = R"(
    QToolTip {
      color: #000000;
      background-color: #ffffff;
      border: 1px solid #cccccc;
      padding: 4px;
    }
    QMenu {
      background-color: #ffffff;
      color: #000000;
      border: 1px solid #cccccc;
    }
    QMenu::item:selected {
      background-color: #0078d7;
      color: #ffffff;
    }
    QMenuBar {
      background-color: #f0f0f0;
      color: #000000;
    }
    QMenuBar::item:selected {
      background-color: #e0e0e0;
    }
    QScrollBar:vertical {
      background: #f0f0f0;
      width: 12px;
      margin: 0px;
    }
    QScrollBar::handle:vertical {
      background: #c0c0c0;
      min-height: 20px;
      border-radius: 4px;
    }
    QScrollBar::handle:vertical:hover {
      background: #a0a0a0;
    }
    QScrollBar:horizontal {
      background: #f0f0f0;
      height: 12px;
      margin: 0px;
    }
    QScrollBar::handle:horizontal {
      background: #c0c0c0;
      min-width: 20px;
      border-radius: 4px;
    }
    QScrollBar::handle:horizontal:hover {
      background: #a0a0a0;
    }
    QScrollBar::add-line, QScrollBar::sub-line {
      border: none;
      background: none;
    }
    QDockWidget {
      color: #000000;
    }
    QDockWidget::title {
      background: #e0e0e0;
      padding: 4px;
    }
  )";

  qApp->setStyleSheet(styleSheet);
}

void ThemeManager::saveThemePreference() {
  QSettings settings("FullScreenPencilDraw", "FullScreenPencilDraw");
  settings.setValue("theme", currentTheme_ == Dark ? "dark" : "light");
}

void ThemeManager::loadThemePreference() {
  QSettings settings("FullScreenPencilDraw", "FullScreenPencilDraw");
  QString themeName = settings.value("theme", "dark").toString();
  currentTheme_ = (themeName == "light") ? Light : Dark;

  // Apply the loaded theme
  if (currentTheme_ == Dark) {
    applyDarkTheme();
  } else {
    applyLightTheme();
  }
}
