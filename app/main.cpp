#include "mainwindows.h"

#include <iostream>

#include <QApplication>

int main(int argc, char** argv) {

    QApplication app(argc, argv);

	Q_INIT_RESOURCE(ressources);

    MainWindows mw;
    mw.show();

	if (argc > 1) {
		mw.openProjectFromFile(argv[1]);
	}

    return app.exec();
}
