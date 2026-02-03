#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow w;
    w.showFullScreen(); // Fotobox = kein Fenster-Gedöns

    return app.exec();
}