#include "mainwindows.h"

#include <iostream>

#include <QApplication>

int main(int argc, char** argv) {

    QApplication app(argc, argv);

    MainWindows mw;
    mw.show();

    return app.exec();
}
