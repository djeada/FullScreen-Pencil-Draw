/**
 * @file side_tab_bar.h
 * @brief A toolbar that displays tabs for hidden/collapsed dock widgets.
 *
 * When a tracked QDockWidget is hidden, a clickable tab appears in this bar.
 * Clicking the tab restores the dock widget. The bar auto-hides when every
 * tracked dock widget is visible.
 */
#ifndef SIDE_TAB_BAR_H
#define SIDE_TAB_BAR_H

#include <QDockWidget>
#include <QToolBar>
#include <QVector>

/**
 * @brief Vertical toolbar that shows tabs for side-hidden dock widgets.
 *
 * Track one or more QDockWidget instances with trackDockWidget(). When a
 * tracked widget becomes invisible the bar shows a labelled tab for it;
 * clicking the tab restores the widget. The bar itself is only visible
 * when at least one tracked widget is hidden.
 */
class SideTabBar : public QToolBar {
  Q_OBJECT

public:
  explicit SideTabBar(const QString &title, QWidget *parent = nullptr);

  /**
   * @brief Begin tracking a dock widget.
   *
   * A tab is created for the widget but only shown when the widget is hidden.
   * Safe to call multiple times for the same widget (duplicates are ignored).
   */
  void trackDockWidget(QDockWidget *dock);

  /**
   * @brief Return the number of tracked dock widgets.
   */
  int trackedCount() const;

  /**
   * @brief Return the number of currently visible tabs (hidden dock widgets).
   */
  int visibleTabCount() const;

private:
  /** Update the overall bar visibility based on tab states. */
  void refreshVisibility();

  struct DockEntry {
    QDockWidget *dock;
    QAction *tabAction;
  };
  QVector<DockEntry> entries_;
};

#endif // SIDE_TAB_BAR_H
