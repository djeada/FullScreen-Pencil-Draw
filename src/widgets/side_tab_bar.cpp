/**
 * @file side_tab_bar.cpp
 * @brief Implementation of the SideTabBar widget.
 */
#include "side_tab_bar.h"
#include <QMainWindow>

namespace {

constexpr char kRestoreViaSideTabProperty[] = "restoreViaSideTab";

bool shouldShowSideTab(QDockWidget *dock) {
  return dock && !dock->isVisible() &&
         dock->property(kRestoreViaSideTabProperty).toBool();
}

} // namespace

SideTabBar::SideTabBar(const QString &title, QWidget *parent)
    : QToolBar(title, parent) {
  setObjectName(QStringLiteral("sideDockHandleBar"));
  setMovable(false);
  setFloatable(false);
  setOrientation(Qt::Vertical);
  setToolButtonStyle(Qt::ToolButtonTextOnly);
  setVisible(false); // hidden until a tracked widget is collapsed
  applyTheme();
}

void SideTabBar::changeEvent(QEvent *event) {
  if (event && !applyingTheme_ &&
      (event->type() == QEvent::PaletteChange ||
       event->type() == QEvent::ApplicationPaletteChange)) {
    applyTheme();
  }
  QToolBar::changeEvent(event);
}

void SideTabBar::trackDockWidget(QDockWidget *dock) {
  if (!dock)
    return;

  // Avoid duplicate tracking
  for (const auto &entry : entries_) {
    if (entry.dock == dock)
      return;
  }

  QAction *tabAction = addAction(handleTextForDock(dock));
  tabAction->setToolTip(QString("Restore %1").arg(dock->windowTitle()));
  tabAction->setVisible(shouldShowSideTab(dock));

  // Restore the dock widget when the tab is clicked
  connect(tabAction, &QAction::triggered, this, [dock]() {
    dock->setProperty(kRestoreViaSideTabProperty, false);
    dock->show();
    dock->raise();
  });

  // Keep tab visibility in sync with the dock widget
  connect(dock, &QDockWidget::visibilityChanged, this,
          [this, dock, tabAction](bool) {
            tabAction->setVisible(shouldShowSideTab(dock));
            refreshVisibility();
          });
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
  connect(dock, &QDockWidget::dockLocationChanged, this,
          [this, dock](Qt::DockWidgetArea) {
            for (const auto &entry : entries_) {
              if (entry.dock == dock) {
                refreshEntry(entry);
                break;
              }
            }
          });
#endif

  entries_.append({dock, tabAction});
  refreshEntry(entries_.last());
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

void SideTabBar::applyTheme() {
  const bool darkTheme = palette().window().color().lightness() < 128;
  if (hasAppliedTheme_ && lastDarkTheme_ == darkTheme) {
    return;
  }

  applyingTheme_ = true;
  setStyleSheet(darkTheme ? R"(
              QToolBar {
                background-color: transparent;
                border: none;
                spacing: 6px;
                padding: 6px 2px;
              }
              QToolButton {
                background-color: #132031;
                color: #f4e6d6;
                border: 1px solid rgba(255, 244, 230, 0.08);
                border-radius: 10px;
                padding: 8px 7px;
                font-weight: 700;
                min-width: 34px;
              }
              QToolButton:hover {
                background-color: #1a2b40;
                border: 1px solid rgba(249, 115, 22, 0.35);
              }
              QToolButton:pressed {
                background-color: rgba(249, 115, 22, 0.14);
              }
            )"
                          : R"(
              QToolBar {
                background-color: transparent;
                border: none;
                spacing: 6px;
                padding: 6px 2px;
              }
              QToolButton {
                background-color: #fffaf5;
                color: #31261d;
                border: 1px solid #ddcfbc;
                border-radius: 10px;
                padding: 8px 7px;
                font-weight: 700;
                min-width: 34px;
              }
              QToolButton:hover {
                background-color: #fff4e7;
                border: 1px solid rgba(234, 88, 12, 0.28);
              }
              QToolButton:pressed {
                background-color: #f6dfca;
              }
            )");
  lastDarkTheme_ = darkTheme;
  hasAppliedTheme_ = true;
  applyingTheme_ = false;
}

void SideTabBar::refreshEntry(const DockEntry &entry) {
  if (!entry.dock || !entry.tabAction) {
    return;
  }

  entry.tabAction->setText(handleTextForDock(entry.dock));
  entry.tabAction->setToolTip(
      QString("Restore %1").arg(entry.dock->windowTitle()));
  entry.tabAction->setVisible(shouldShowSideTab(entry.dock));
}

QString SideTabBar::handleTextForDock(QDockWidget *dock) const {
  if (!dock) {
    return QString();
  }

  return QStringLiteral("‹ %1").arg(dock->windowTitle());
}
