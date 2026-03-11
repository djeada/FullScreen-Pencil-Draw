/**
 * @file dock_title_bar.h
 * @brief Custom title bar with explicit collapse controls for dock widgets.
 */
#ifndef DOCK_TITLE_BAR_H
#define DOCK_TITLE_BAR_H

#include <QWidget>

class QLabel;
class QToolButton;
class QDockWidget;
class QMouseEvent;

class DockTitleBar : public QWidget {
  Q_OBJECT

public:
  explicit DockTitleBar(QDockWidget *dock, QWidget *parent = nullptr);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private:
  void applyTheme();
  void updateButtons();
  void updateTitle();
  void updateCollapseGlyph();

  QDockWidget *dock_;
  QLabel *titleLabel_;
  QToolButton *collapseButton_;
  QToolButton *floatButton_;
  QToolButton *closeButton_;
};

#endif // DOCK_TITLE_BAR_H
