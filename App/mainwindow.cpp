#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QSvgGenerator>
#include <QGraphicsItem>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scene = new PaintScene(this);
    scene->setSceneRect(ui->graphicsView->rect());

    ui->graphicsView->setRenderHints(QPainter::Antialiasing);
    ui->graphicsView->setScene(scene);
    ui->toolbox->setPaintScene(scene);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionSave_to_svg_triggered()
{
    auto filePath = QFileDialog::getSaveFileName(this, "Save SVG", "", "SVG files (*.svg)");

    if (filePath == "")
        return;

    saveSceneToSvg(filePath);
}

void MainWindow::saveSceneToSvg(const QString &filename) {
    QRectF newSceneRect;
    QGraphicsScene tempScene(scene->sceneRect());
    tempScene.setBackgroundBrush(QBrush(Qt::transparent));
    tempScene.setItemIndexMethod(QGraphicsScene::BspTreeIndex);

    for (auto item : scene->items()) {
        newSceneRect |= item->mapToScene(item->boundingRect()).boundingRect();
        tempScene.addItem(item);
    }

    tempScene.setSceneRect(newSceneRect);
    tempScene.clearSelection();

    QSvgGenerator generator;
    generator.setFileName(filename);
    generator.setSize(ui->graphicsView->size());
    generator.setViewBox(ui->graphicsView->rect());
    generator.setDescription(QObject::tr("My canvas exported to Svg"));
    generator.setTitle(filename);

    QPainter painter;
    painter.begin(&generator);
    tempScene.render(&painter);
    painter.end();

    scene->update();
}
