#ifndef TOOLBOX_H
#define TOOLBOX_H

#include <QWidget>

class PaintScene;

namespace Ui {
    class ToolBox;
}

class ToolBox : public QWidget
{
    Q_OBJECT

public:
    ToolBox(QWidget *parent = nullptr);
    ~ToolBox();
    void setPaintScene(PaintScene* scene);

private slots:
    void on_buttonPencil_clicked();

private:
    Ui::ToolBox* ui;
    PaintScene* paintScene;
};

#endif // TOOLBOX_H
