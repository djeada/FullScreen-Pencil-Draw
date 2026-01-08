// theme_manager.h
#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <QObject>
#include <QPalette>
#include <QString>

/**
 * @brief Manages application themes (light/dark mode).
 *
 * The ThemeManager provides functionality to switch between light and dark
 * themes, applying appropriate color palettes to the application.
 */
class ThemeManager : public QObject {
  Q_OBJECT

public:
  enum Theme { Light, Dark };

  static ThemeManager &instance();

  Theme currentTheme() const;
  bool isDarkTheme() const;

public slots:
  void setTheme(Theme theme);
  void toggleTheme();

signals:
  void themeChanged(Theme newTheme);

private:
  explicit ThemeManager(QObject *parent = nullptr);
  ~ThemeManager() = default;

  // Prevent copying
  ThemeManager(const ThemeManager &) = delete;
  ThemeManager &operator=(const ThemeManager &) = delete;

  void applyLightTheme();
  void applyDarkTheme();
  void saveThemePreference();
  void loadThemePreference();

  Theme currentTheme_;
};

#endif // THEME_MANAGER_H
