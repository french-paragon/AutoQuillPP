#include "mainwindows.h"

#include <iostream>

#include <QApplication>

int main(int argc, char** argv) {

    QApplication app(argc, argv);

	Q_INIT_RESOURCE(ressources);

    MainWindows mw;
    mw.show();

    return app.exec();
}
