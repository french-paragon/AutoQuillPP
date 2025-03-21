#include "exportactions.h"

#include "../lib/documenttemplate.h"
#include "../lib/jsondocumentdatainterface.h"
#include "../lib/documentrenderer.h"

#include "mainwindows.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>

#include <QJsonDocument>
#include <QJsonObject>

bool exportTemplateUsingJson(DocumentTemplate* documentTemplate,
                             MainWindows* mainWindows) {

    if (mainWindows == nullptr) {
        return false;
    }

    if (documentTemplate == nullptr) {
        QMessageBox::warning(mainWindows,
                             QObject::tr("Error exporting template"),
                             QObject::tr("Null template provided"));
        return false;
    }

    QString docFolderPath = QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first();

    QString fileName = QFileDialog::getOpenFileName(mainWindows,
                                                    QObject::tr("Open json data file"),
                                                    docFolderPath,
                                                    QObject::tr("Json files (*.json)"));

    if (fileName.isEmpty()) {
        return false;
    }

    QString outFileName = QFileDialog::getSaveFileName(mainWindows,
                                                       QObject::tr("Save pdf file to"),
                                                       docFolderPath,
                                                       QObject::tr("PDF files (*.pdf)"));

    if (outFileName.isEmpty()) {
        return false;
    }

    QFile inFile(fileName);

    if (!inFile.open(QFile::ReadOnly)) {
        QMessageBox::warning(mainWindows,
                             QObject::tr("Error exporting template"),
                             QObject::tr("Could not open file: %1").arg(fileName));
        return false;
    }

    QByteArray data = inFile.readAll();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);

    if (parseError.error != QJsonParseError::NoError) {
        QMessageBox::warning(mainWindows,
                             QObject::tr("Error exporting template"),
                             QObject::tr("Error while parsing json: %1").arg(parseError.errorString()));
        return false;
    }

    if (!doc.isObject()) {
        QMessageBox::warning(mainWindows,
                             QObject::tr("Error exporting template"),
                             QObject::tr("Error while parsing json: document is not an object"));
        return false;
    }

    QJsonObject obj = doc.object();

    JsonDocumentDataInterface dataInterface(obj);

    DocumentRenderer renderer(documentTemplate);

    renderer.render(&dataInterface, outFileName);

    return true;
}
