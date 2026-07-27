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
    void testLoopWithHeaderLayout();
    void testLoopWithRepeatingHeaderLayout();

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

    auto& page1LayoutInfos = layoutResults.layout[0];
    QCOMPARE(page1LayoutInfos->subitemsRenderInfos.size(), 1);

    constexpr int expectedItemsOnPage1 = 8;
    auto& loop1LayoutInfos = page1LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(loop1LayoutInfos->subitemsRenderInfos.size(), expectedItemsOnPage1);

    for (int i = 0; i < expectedItemsOnPage1; i++) {
        int expectredN = i+1;

        QVariant val = loop1LayoutInfos->subitemsRenderInfos[i]->itemValue.getValue("text").getValue();

        QCOMPARE(val.toString(), QString("Line %1").arg(expectredN));
    }

    auto& page2LayoutInfos = layoutResults.layout[1];
    QCOMPARE(page2LayoutInfos->subitemsRenderInfos.size(), 1);

    constexpr int expectedItemsOnPage2 = 4;
    auto& loop2LayoutInfos = page2LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(loop2LayoutInfos->subitemsRenderInfos.size(), expectedItemsOnPage2);

    for (int i = 0; i < expectedItemsOnPage2; i++) {
        int expectredN = i+expectedItemsOnPage1+1;

        QVariant val = loop2LayoutInfos->subitemsRenderInfos[i]->itemValue.getValue("text").getValue();

        QCOMPARE(val.toString(), QString("Line %1").arg(expectredN));
    }
}

void TestLayouts::testLoopWithHeaderLayout() {

    AutoQuill::DocumentTemplate doc_template;
    AutoQuill::RenderPluginManager pluginManager;

    AutoQuill::DocumentItem* page = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Page, &doc_template);
    page->setInitialWidth(595);
    page->setInitialHeight(842);

    page->setDataKey("page");
    page->setObjectName("Page");

    doc_template.insertSubItem(page);


    AutoQuill::DocumentItem* list = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::List, page);
    list->setPosX(0);
    list->setPosY(0);
    list->setInitialWidth(595);
    list->setInitialHeight(842);
    list->setObjectName("List");
    list->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::OverflowOnNewPage);

    list->setDataKey("list");

    page->insertSubItem(list);

    AutoQuill::DocumentItem* header = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Text, list);
    header->setInitialWidth(595);
    header->setInitialHeight(105);
    header->setMaxWidth(595);
    header->setMaxHeight(105);
    header->setFontName("sans");
    header->setFontSize(14);
    header->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::DrawFirstInstanceOnly);

    header->setDataKey("header");
    header->setObjectName("Header");

    list->insertSubItem(header);

    AutoQuill::DocumentItem* loop = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Loop, list);
    loop->setPosX(0);
    loop->setPosY(0);
    loop->setInitialWidth(595);
    loop->setInitialHeight(105);
    loop->setMaxWidth(595);
    loop->setMaxHeight(842);
    loop->setObjectName("Loop");
    loop->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::OverflowOnNewPage);

    loop->setDataKey("loop");

    list->insertSubItem(loop);

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

    QJsonObject list_data;

    const QString header_title = "Header";
    QJsonObject header_data;
    header_data.insert("header", header_title); //TODO, check if we can change the behavior of text blocks to just take the text, instead of looking for a nested object
    list_data.insert("header", header_data);

    QJsonArray loop_data;

    constexpr int nLines = 12;

    for (int i = 0; i < nLines; i++) {
        QJsonObject text_data;
        text_data.insert("text", QString("Line %1").arg(i+1));

        loop_data.push_back(text_data);
    }

    list_data.insert("loop", loop_data);

    page_data.insert("list", list_data);

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

    auto& page1LayoutInfos = layoutResults.layout[0];
    QCOMPARE(page1LayoutInfos->subitemsRenderInfos.size(), 1);

    constexpr int expectedListItemsOnPage1 = 2;
    auto& list1LayoutInfos = page1LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(list1LayoutInfos->subitemsRenderInfos.size(), expectedListItemsOnPage1);

    QVariant val = list1LayoutInfos->subitemsRenderInfos[0]->itemValue.getValue("header").getValue();

    QCOMPARE(val.toString(), header_title);

    constexpr int expectedItemsOnPage1 = 7;
    auto& loop1LayoutInfos = list1LayoutInfos->subitemsRenderInfos[1];
    QCOMPARE(loop1LayoutInfos->subitemsRenderInfos.size(), expectedItemsOnPage1);

    for (int i = 0; i < expectedItemsOnPage1; i++) {
        int expectredN = i+1;

        QVariant val = loop1LayoutInfos->subitemsRenderInfos[i]->itemValue.getValue("text").getValue();

        QCOMPARE(val.toString(), QString("Line %1").arg(expectredN));
    }

    auto& page2LayoutInfos = layoutResults.layout[1];
    QCOMPARE(page2LayoutInfos->subitemsRenderInfos.size(), 1);

    constexpr int expectedListItemsOnPage2 = 1;
    auto& list2LayoutInfos = page2LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(list2LayoutInfos->subitemsRenderInfos.size(), expectedListItemsOnPage2);

    constexpr int expectedItemsOnPage2 = 5;
    auto& loop2LayoutInfos = list2LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(loop2LayoutInfos->subitemsRenderInfos.size(), expectedItemsOnPage2);

    for (int i = 0; i < expectedItemsOnPage2; i++) {
        int expectredN = i+expectedItemsOnPage1+1;

        QVariant val = loop2LayoutInfos->subitemsRenderInfos[i]->itemValue.getValue("text").getValue();

        QCOMPARE(val.toString(), QString("Line %1").arg(expectredN));
    }
}

void TestLayouts::testLoopWithRepeatingHeaderLayout() {

    AutoQuill::DocumentTemplate doc_template;
    AutoQuill::RenderPluginManager pluginManager;

    AutoQuill::DocumentItem* page = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Page, &doc_template);
    page->setInitialWidth(595);
    page->setInitialHeight(842);

    page->setDataKey("page");
    page->setObjectName("Page");

    doc_template.insertSubItem(page);


    AutoQuill::DocumentItem* frame = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Frame, page);
    frame->setPosX(0);
    frame->setPosY(0);
    frame->setInitialWidth(595);
    frame->setInitialHeight(842);
    frame->setMaxWidth(595);
    frame->setMaxHeight(842);
    frame->setObjectName("List");
    frame->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::CopyOnNewPages);

    frame->setDataKey("frame");

    page->insertSubItem(frame);

    AutoQuill::DocumentItem* header = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Text, frame);
    header->setInitialWidth(595);
    header->setInitialHeight(105);
    header->setMaxWidth(595);
    header->setMaxHeight(105);
    header->setFontName("sans");
    header->setFontSize(14);
    header->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::CopyOnNewPages);

    header->setDataKey("header");
    header->setObjectName("Header");

    frame->insertSubItem(header);

    AutoQuill::DocumentItem* loop = new AutoQuill::DocumentItem(AutoQuill::DocumentItem::Loop, frame);
    loop->setPosX(0);
    loop->setPosY(105);
    loop->setInitialWidth(595);
    loop->setInitialHeight(105);
    loop->setMaxWidth(595);
    loop->setMaxHeight(737);
    loop->setObjectName("Loop");
    loop->setOverflowBehavior(AutoQuill::DocumentItem::OverflowBehavior::OverflowOnNewPage);

    loop->setDataKey("loop");

    frame->insertSubItem(loop);

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

    QJsonObject frame_data;

    const QString header_title = "Header";
    QJsonObject header_data;
    header_data.insert("header", header_title); //TODO, check if we can change the behavior of text blocks to just take the text, instead of looking for a nested object
    frame_data.insert("header", header_data);

    QJsonArray loop_data;

    constexpr int nLines = 12;

    for (int i = 0; i < nLines; i++) {
        QJsonObject text_data;
        text_data.insert("text", QString("Line %1").arg(i+1));

        loop_data.push_back(text_data);
    }

    frame_data.insert("loop", loop_data);

    page_data.insert("frame", frame_data);

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

    auto& page1LayoutInfos = layoutResults.layout[0];
    QCOMPARE(page1LayoutInfos->subitemsRenderInfos.size(), 1);

    constexpr int expectedListItemsOnPage1 = 2;
    auto& list1LayoutInfos = page1LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(list1LayoutInfos->subitemsRenderInfos.size(), expectedListItemsOnPage1);

    QVariant val = list1LayoutInfos->subitemsRenderInfos[0]->itemValue.getValue("header").getValue();

    QCOMPARE(val.toString(), header_title);

    constexpr int expectedItemsOnPage1 = 7;
    auto& loop1LayoutInfos = list1LayoutInfos->subitemsRenderInfos[1];
    QCOMPARE(loop1LayoutInfos->subitemsRenderInfos.size(), expectedItemsOnPage1);

    for (int i = 0; i < expectedItemsOnPage1; i++) {
        int expectredN = i+1;

        QVariant val = loop1LayoutInfos->subitemsRenderInfos[i]->itemValue.getValue("text").getValue();

        QCOMPARE(val.toString(), QString("Line %1").arg(expectredN));
    }

    auto& page2LayoutInfos = layoutResults.layout[1];
    QCOMPARE(page2LayoutInfos->subitemsRenderInfos.size(), 1);

    constexpr int expectedListItemsOnPage2 = 2;
    auto& list2LayoutInfos = page2LayoutInfos->subitemsRenderInfos[0];
    QCOMPARE(list2LayoutInfos->subitemsRenderInfos.size(), expectedListItemsOnPage2);

    val = list2LayoutInfos->subitemsRenderInfos[0]->itemValue.getValue("header").getValue();

    QCOMPARE(val.toString(), header_title);

    constexpr int expectedItemsOnPage2 = 5;
    auto& loop2LayoutInfos = list2LayoutInfos->subitemsRenderInfos[1];
    QCOMPARE(loop2LayoutInfos->subitemsRenderInfos.size(), expectedItemsOnPage2);

    for (int i = 0; i < expectedItemsOnPage2; i++) {
        int expectredN = i+expectedItemsOnPage1+1;

        QVariant val = loop2LayoutInfos->subitemsRenderInfos[i]->itemValue.getValue("text").getValue();

        QCOMPARE(val.toString(), QString("Line %1").arg(expectredN));
    }
}

#include "test_layouts.moc"

QTEST_MAIN(TestLayouts)
