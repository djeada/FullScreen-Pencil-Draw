/**
 * @file side_tab_bar.cpp
 * @brief Implementation of the SideTabBar widget.
 */
#include "side_tab_bar.h"

SideTabBar::SideTabBar(const QString &title, QWidget *parent)
    : QToolBar(title, parent) {
  setMovable(false);
  setFloatable(false);
  setVisible(false); // hidden until a tracked widget is collapsed
}

void SideTabBar::trackDockWidget(QDockWidget *dock) {
  if (!dock)
    return;

  // Avoid duplicate tracking
  for (const auto &entry : entries_) {
    if (entry.dock == dock)
      return;
  }

  QAction *tabAction = addAction(dock->windowTitle());
  tabAction->setToolTip(
      QString("Show %1 panel").arg(dock->windowTitle()));
  tabAction->setVisible(!dock->isVisible());

  // Restore the dock widget when the tab is clicked
  connect(tabAction, &QAction::triggered, this, [dock]() {
    dock->show();
    dock->raise();
  });

  // Keep tab visibility in sync with the dock widget
  connect(dock, &QDockWidget::visibilityChanged, this,
          [this, tabAction](bool visible) {
            tabAction->setVisible(!visible);
            refreshVisibility();
          });

  entries_.append({dock, tabAction});
  refreshVisibility();
}

int SideTabBar::trackedCount() const { return entries_.size(); }

int SideTabBar::visibleTabCount() const {
  int count = 0;
  for (const auto &entry : entries_) {
    if (entry.tabAction->isVisible())
      ++count;
  }
  return count;
}

void SideTabBar::refreshVisibility() { setVisible(visibleTabCount() > 0); }
