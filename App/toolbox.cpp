#include "toolbox.h"
#include "ui_toolbox.h"
#include "paintscene.h"

ToolBox::ToolBox(QWidget* parent) :
    QWidget(parent),
    ui(new Ui::ToolBox),
    paintScene(nullptr)
{
    ui->setupUi(this);
}

ToolBox::~ToolBox()
{
    delete ui;
}

void ToolBox::setPaintScene(PaintScene* scene)
{
    paintScene = scene;
}

void ToolBox::on_buttonPencil_clicked()
{
    if (paintScene)
          paintScene->setMode(PaintMode::DrawPoint);
}

void ToolBox::on_buttonLine_clicked()
{
    if (paintScene)
          paintScene->setMode(PaintMode::DrawLine);
}
