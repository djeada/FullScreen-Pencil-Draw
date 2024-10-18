#include "windows/main_window.h"
#include <QApplication>


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    MainWindow window;
    window.showFullScreen(); // or window.showMaximized();
    return app.exec();
}
