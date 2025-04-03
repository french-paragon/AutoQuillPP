#include "exportactions.h"

#include "../lib/documenttemplate.h"
#include "../lib/jsondocumentdatainterface.h"
#include "../lib/documentrenderer.h"
#include "../lib/renderplugin.h"

#include "mainwindows.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QStandardPaths>
#include <QTextStream>

#include <QJsonDocument>
#include <QJsonObject>

bool exportTemplateUsingJson(AutoQuill::DocumentTemplate *documentTemplate,
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

	AutoQuill::JsonDocumentDataInterface dataInterface(obj);

	AutoQuill::DocumentRenderer renderer(documentTemplate);
	AutoQuill::RenderPluginManager defaultPluginManager;

	auto rendering_status = renderer.render(&dataInterface, defaultPluginManager, outFileName);

	QTextStream out(stdout);

	if (rendering_status.status == AutoQuill::DocumentRenderer::Status::Success) {
		out << "Successfully rendered document" << Qt::endl;
	} else {
		out << "The renderer encountered some errors: " << rendering_status.message << Qt::endl;
	}

    return true;
}
