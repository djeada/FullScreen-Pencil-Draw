#ifndef TOOLPANEL_H
#define TOOLPANEL_H

#include <QToolBar>
#include <QAction>

class ToolPanel : public QToolBar
{
    Q_OBJECT

public:
    explicit ToolPanel(QWidget *parent = nullptr);

signals:
    void penSelected();
    void eraserSelected();

private:
    QAction *actionPen;
    QAction *actionEraser;

private slots:
    void onActionPen();
    void onActionEraser();
};

#endif // TOOLPANEL_H
