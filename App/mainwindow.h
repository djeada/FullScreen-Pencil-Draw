#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "paintscene.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_actionSave_to_svg_triggered();

private:
    Ui::MainWindow *ui;
    PaintScene* scene;
    void saveSceneToSvg(const QString &filename);
};
#endif // MAINWINDOW_H
