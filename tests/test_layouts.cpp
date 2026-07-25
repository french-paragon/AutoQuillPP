#include <QTest>

#include "../lib/jsondocumentdatainterface.h"
#include "../lib/documenttemplate.h"
#include "../lib/documentitem.h"
#include "../lib/documentrenderer.h"
#include "../lib/renderplugin.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include <QPainter>
#include <QPdfWriter>
#include <QIODevice>

class NullDevice : public QIODevice {
    Q_OBJECT
public:
    explicit NullDevice(QObject *parent = nullptr) : QIODevice(parent) {}

    bool isSequential() const override { return true; }

protected:
    qint64 readData(char *data, qint64 maxSize) override {
        Q_UNUSED(data);
        Q_UNUSED(maxSize);
        return -1; // End of file / nothing to read
    }

    qint64 writeData(const char *data, qint64 maxSize) override {
        Q_UNUSED(data);
        return maxSize; // Silently discard all written data
    }
};

class TestLayouts : public QObject {

    Q_OBJECT
private Q_SLOTS:

    void initTestCase();

    void testBasicLoopLayout();

private:

};

void TestLayouts::initTestCase() {

}

void TestLayouts::testBasicLoopLayout() {

    AutoQuill::DocumentTemplate doc_template;
    AutoQuill::RenderPluginManager pluginManager;

    AutoQuill::DocumentItem* page = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Page, &doc_template);
    page->setInitialWidth(595);
    page->setInitialHeight(842);

    page->setDataKey("page");
    page->setObjectName("Page");

    doc_template.insertSubItem(page);

    AutoQuill::DocumentItem* loop = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Loop, page);
    loop->setPosX(0);
    loop->setPosY(0);
    loop->setInitialWidth(595);
    loop->setInitialHeight(842);
    loop->setObjectName("Loop");
    loop->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::OverflowOnNewPage);

    loop->setDataKey("loop");

    page->insertSubItem(loop);

    AutoQuill::DocumentItem* text = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Text, loop);
    text->setInitialWidth(595);
    text->setInitialHeight(105);
    text->setMaxWidth(595);
    text->setMaxHeight(105);
    text->setFontName("sans");
    text->setFontSize(12);

    text->setDataKey("text");
    text->setObjectName("Text");

    loop->insertSubItem(text);

    QJsonObject layout_data;

    QJsonObject page_data;

    QJsonArray loop_data;

    constexpr int nLines = 12;

    for (int i = 0; i < nLines; i++) {
        QJsonObject text_data;
        text_data.insert("text", QString("Line %1").arg(i+1));

        loop_data.push_back(text_data);
    }

    page_data.insert("loop", loop_data);

    layout_data.insert("page", page_data);

    AutoQuill::JsonDocumentDataInterface data_interface(layout_data);

    NullDevice device;
    device.open(QIODevice::WriteOnly);

    QPdfWriter writer(&device);
    writer.setResolution(72);
    writer.setTitle("Test");
    writer.setPageMargins(QMarginsF(0,0,0,0));

    QPainter tmpPainter(&writer);

    AutoQuill::DocumentRenderer renderer(doc_template);
    auto layoutResults = renderer.layoutHeadless(&data_interface, pluginManager, &tmpPainter);

    if (layoutResults.status.status != AutoQuill::DocumentRenderer::Status::Success) {
        qWarning() << "Error while laying out the document: " << layoutResults.status.message;
    }

    QCOMPARE(layoutResults.status.status, AutoQuill::DocumentRenderer::Status::Success);

    QCOMPARE(layoutResults.layout.size(), 2); //expect two pages

}

#include "test_layouts.moc"

QTEST_MAIN(TestLayouts)
