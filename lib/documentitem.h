#ifndef DOCUMENTITEM_H
#define DOCUMENTITEM_H

#include <QObject>
#include <QColor>

class DocumentItem : public QObject
{
    Q_OBJECT
public:

    enum Type {
        Frame, //just an empty frame, with possibly a border and a background
        Text, //a frame containing some text
        Image, //a frame containing an image
        Page, //a page
        Condition, //a block containing other blocks based on a condition
        List, //a list of items, laid out in a certain direction
        Loop, //a list of delegate items, based on array data and laid out in a certain direction
        Plugin
    };

    Q_ENUM(Type);

    enum Direction {
        Left2Right,
        Right2Left,
        Top2Bottom,
        Bottom2Top
    };

    Q_ENUM(Direction);

    static QString typeToString(Type const& type);
    static Type stringToType(QString const& str);

    Q_PROPERTY(Direction direction READ direction WRITE setDirection NOTIFY directionChanged);

    Q_PROPERTY(qreal posX READ posX WRITE setPosX NOTIFY posXChanged)
    Q_PROPERTY(qreal posY READ posY WRITE setPosY NOTIFY posYChanged)

    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY widthChanged)
    Q_PROPERTY(qreal height READ height WRITE setHeight NOTIFY heightChanged)

    Q_PROPERTY(qreal maxWidth READ maxWidth WRITE setMaxWidth NOTIFY maxWidthChanged)
    Q_PROPERTY(qreal maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)

    Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)

    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)

    Q_PROPERTY(QString dataKey READ dataKey WRITE setDataKey NOTIFY datakeyChanged)
    Q_PROPERTY(QString data READ data WRITE setData NOTIFY dataChanged)

    DocumentItem(Type type, QObject* parent = nullptr);

    inline Type getType() const {
        return _type;
    }

    inline Direction direction() const {
        return _direction;
    }

    inline void setDirection(Direction direction) {
        if (direction != _direction) {
            _direction = direction;
            Q_EMIT directionChanged();
        }
    }

    inline qreal posX() const {
        return _pos_x;
    }

    inline qreal posY() const {
        return _pos_y;
    }

    inline void setPosX(qreal pos) {
        if (_pos_x != pos) {
            _pos_x = pos;
            Q_EMIT posXChanged();
        }
    }

    inline void setPosY(qreal pos) {
        if (_pos_y != pos) {
            _pos_y = pos;
            Q_EMIT posYChanged();
        }
    }


    inline qreal width() const {
        return _width;
    }

    inline qreal height() const {
        return _height;
    }

    inline void setWidth(qreal width) {
        if (_width != width) {
            _width = width;
            Q_EMIT widthChanged();
        }
    }

    inline void setHeight(qreal height) {
        if (_height != height) {
            _height = height;
            Q_EMIT heightChanged();
        }
    }

    inline qreal maxWidth() const {
        return _max_width;
    }

    inline qreal maxHeight() const {
        return _max_height;
    }

    inline void setMaxWidth(qreal width) {
        if (_max_width != width) {
            _max_width = width;
            Q_EMIT maxWidthChanged();
        }
    }

    inline void setMaxHeight(qreal height) {
        if (_max_height != height) {
            _max_height = height;
            Q_EMIT maxHeightChanged();
        }
    }

    inline qreal borderWidth() const {
        return _border_width;
    }

    inline void setBorderWidth(qreal width) {
        if (_border_width != width) {
            _border_width = width;
            Q_EMIT borderWidthChanged();
        }
    }

    inline QColor borderColor() const {
        return _border_color;
    }

    inline void setBorderColor(QColor color) {
        if (_border_color != color) {
            _border_color = color;
            Q_EMIT borderColorChanged();
        }
    }

    inline QColor fillColor() const {
        return _fill_color;
    }

    inline void setFillColor(QColor color) {
        if (_fill_color != color) {
            _fill_color = color;
            Q_EMIT fillColorChanged();
        }
    }

    inline QString dataKey() {
        return _data_key;
    }

    inline void setDataKey(QString const& key) {
        if (_data_key != key) {
            _data_key = key;
            Q_EMIT datakeyChanged();
        }
    }

    inline QString data() {
        return _data;
    }

    inline void setData(QString const& key) {
        if (_data != key) {
            _data = key;
            Q_EMIT dataChanged();
        }
    }

Q_SIGNALS:

    void directionChanged();

    void posXChanged();
    void posYChanged();

    void widthChanged();
    void heightChanged();

    void maxWidthChanged();
    void maxHeightChanged();

    void borderWidthChanged();
    void borderColorChanged();

    void fillColorChanged();

    void anchorChanged();

    void datakeyChanged();
    void dataChanged();

protected:

    Type _type;

    Direction _direction;

    qreal _pos_x;
    qreal _pos_y;

    qreal _width;
    qreal _height;

    qreal _max_width;
    qreal _max_height;

    qreal _border_width;
    QColor _border_color;

    QColor _fill_color;

    QString _data_key;
    QString _data;
};

#endif // DOCUMENTITEM_H
