#ifndef DOCUMENTITEM_H
#define DOCUMENTITEM_H

#include <QObject>
#include <QColor>
#include <QPoint>
#include <QSize>

class DocumentItem : public QObject
{
    Q_OBJECT
public:

    enum Type {
		Frame, //item frame, with possibly a border and a background.
        Text, //a frame containing some text
        Image, //a frame containing an image
        Page, //a page
        Condition, //a block containing other blocks based on a condition
        List, //a list of items, laid out in a certain direction
        Loop, //a list of delegate items, based on array data and laid out in a certain direction
		Plugin, //a datablock pointing to a plugin for rendering
		Invalid //invalid type
	};

	static QList<Type> supportedRootTypes();

	Q_ENUM(Type);

	enum Direction {
		Left2Right,
		Right2Left,
		Top2Bottom,
		Bottom2Top
	};

	Q_ENUM(Direction);

	enum OverflowBehavior {
		DrawFirstInstanceOnly,
		CopyOnNewPages,
		OverflowOnNewPage
	};

	enum TextAlign {
		AlignLeft,
		AlignRight,
		AlignCenter,
		AlignJustify
	};

	Q_ENUM(TextAlign);

	enum TextWeight {
		Thin = 100,
		ExtraLight = 200,
		Light = 300,
		Normal = 400,
		Medium = 500,
		DemiBold = 600,
		Bold = 700,
		ExtraBold = 800,
		Black = 900,
	};

	Q_ENUM(TextWeight);

	Q_ENUM(OverflowBehavior);

	static QIcon iconForType(Type const& type);
	static QString typeToString(Type const& type);
    static Type stringToType(QString const& str);
	static inline bool typeAcceptChildrens(Type const& type) {
		switch (type) {
		case Frame:
		case Page:
		case Condition:
		case List:
		case Loop:
			return true;
		default:
			return false;
		}
	}

	Q_PROPERTY(QString direction READ directionStr WRITE setDirectionStr NOTIFY directionChanged);
	Q_PROPERTY(QString overflowBehavior READ overflowBehaviorStr WRITE setOverflowBehaviorStr NOTIFY overflowBehaviorChanged);

    Q_PROPERTY(qreal posX READ posX WRITE setPosX NOTIFY posXChanged)
    Q_PROPERTY(qreal posY READ posY WRITE setPosY NOTIFY posYChanged)

	Q_PROPERTY(qreal initialWidth READ initialWidth WRITE setInitialWidth NOTIFY initialWidthChanged)
    Q_PROPERTY(qreal initialHeight READ initialHeight WRITE setInitialHeight NOTIFY initialHeightChanged)

    Q_PROPERTY(qreal maxWidth READ maxWidth WRITE setMaxWidth NOTIFY maxWidthChanged)
    Q_PROPERTY(qreal maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)

    Q_PROPERTY(qreal borderWidth READ borderWidth WRITE setBorderWidth NOTIFY borderWidthChanged)
    Q_PROPERTY(QColor borderColor READ borderColor WRITE setBorderColor NOTIFY borderColorChanged)

    Q_PROPERTY(QColor fillColor READ fillColor WRITE setFillColor NOTIFY fillColorChanged)

	Q_PROPERTY(qreal fontSize READ fontSize WRITE setFontSize NOTIFY fontSizeChanged)
	Q_PROPERTY(int fontWeight READ fontWeight WRITE setFontWeight NOTIFY fontWeightChanged)
	Q_PROPERTY(QString textAlign READ textAlignName WRITE setTextAlignName NOTIFY textAlignChanged)

    Q_PROPERTY(QString dataKey READ dataKey WRITE setDataKey NOTIFY datakeyChanged)
    Q_PROPERTY(QString data READ data WRITE setData NOTIFY dataChanged)

	DocumentItem(Type type = Invalid, QObject* parent = nullptr);

    inline Type getType() const {
        return _type;
	}

	inline Direction direction() const {
		return _direction;
	}

	inline QString directionStr() const {
		switch(_direction) {
		case Left2Right:
			return "Left2Right";
		case Right2Left:
			return "Right2Left";
		case Top2Bottom:
			return "Top2Bottom";
		case Bottom2Top:
			return "Bottom2Top";
		}
		return "Top2Bottom";
	}

	inline void setDirection(Direction direction) {
		if (direction != _direction) {
			_direction = direction;
			Q_EMIT directionChanged();
		}
	}

	inline void setDirectionStr(QString const& dirStr) {
		Direction direction = Top2Bottom;

		QString str = dirStr.toLower();

		if (str == "left2right" or str == "lefttoright") {
			direction = Left2Right;
		} else if (str == "right2left" or str == "righttoleft") {
			direction = Right2Left;
		} else if (str == "top2bottom" or str == "toptobottom") {
			direction = Top2Bottom;
		} else if (str == "bottom2top" or str == "bottomtotop") {
			direction = Bottom2Top;
		}

		setDirection(direction);
	}

	inline OverflowBehavior overflowBehavior() const {
		return _overflowBehavior;
	}

	inline QString overflowBehaviorStr() const {
		switch (_overflowBehavior) {
		case DrawFirstInstanceOnly:
			return "DrawFirstInstanceOnly";
		case CopyOnNewPages:
			return "CopyFirst";
		case OverflowOnNewPage:
			return "OverflowOnNewPage";
		}
		return "DrawFirstInstanceOnly";
	}

	inline void setOverflowBehavior(OverflowBehavior overflowBehavior) {
		if (overflowBehavior != _overflowBehavior) {
			_overflowBehavior = overflowBehavior;
			Q_EMIT overflowBehaviorChanged();
		}
	}

	inline void setOverflowBehaviorStr(QString str) {
		OverflowBehavior behavior = DrawFirstInstanceOnly;

		QString lowercase = str.toLower();

		if (lowercase == "drawfirstinstanceonly") {
			behavior = DrawFirstInstanceOnly;
		} else if (lowercase == "copyfirst") {
			behavior = CopyOnNewPages;
		} else if (lowercase == "overflowonnewpage") {
			behavior = OverflowOnNewPage;
		}

		setOverflowBehavior(behavior);
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


	inline qreal initialWidth() const {
		return _initial_width;
    }

	inline qreal initialHeight() const {
		return _initial_height;
    }

	inline void setInitialWidth(qreal width) {
		if (_initial_width != width) {
			_initial_width = width;
			Q_EMIT initialWidthChanged();
        }
    }

	inline void setInitialHeight(qreal height) {
		if (_initial_height != height) {
			_initial_height = height;
			Q_EMIT initialHeightChanged();
        }
    }

    inline qreal maxWidth() const {
        return std::max(_initial_width,_max_width);
    }

    inline qreal maxHeight() const {
        return std::max(_initial_height,_max_height);
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

	inline qreal fontSize() const {
		return _font_size;
	}

	inline void setFontSize(qreal size) {
		if (_font_size != size) {
			_font_size = size;
			Q_EMIT fontSizeChanged();
		}
	}

	inline int fontWeight() const {
		return _font_weight;
	}

	inline void setFontWeight(int weight) {

		if (_font_weight == weight) {
			return;
		}

		const static QList<TextWeight> acceptableWeights = {Thin, ExtraLight, Light, Normal, Medium, DemiBold, Bold, ExtraBold, Black};

		TextWeight val = acceptableWeights[0];

		for (TextWeight candidate : acceptableWeights) {
			if (std::abs(weight - candidate) < std::abs(weight - val)) {
				val = candidate;
			}
		}

		if (_font_weight == val) {
			return;
		}

		_font_weight = val;
		Q_EMIT fontWeightChanged();
	}

	inline QString textAlignName() const {
		switch (_text_align) {
		case AlignLeft:
			return "left";
		case AlignRight:
			return "right";
		case AlignCenter:
			return "center";
		case AlignJustify:
			return "justify";
		}
		return "left";
	}

	inline TextAlign textAlign() const {
		return _text_align;
	}

	inline void setTextAlign(TextAlign align) {
		if (align != _text_align) {
			_text_align = align;
			Q_EMIT textAlignChanged();
		}
	}

	inline void setTextAlignName(QString const& align) {

		TextAlign val = AlignLeft;

		QString align_lower = align.toLower();

		if (align_lower.endsWith("right")) {
			val = AlignRight;
		} else if (align_lower.endsWith("center")) {
			val = AlignCenter;
		} else if (align_lower.endsWith("justify")) {
			val = AlignJustify;
		}

		setTextAlign(val);

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

	inline bool removeSubItem(int index) {
		if (index < 0 or index >= _items.size()) {
			return false;
		}

		_items.removeAt(index);
		return true;
	}

	inline bool moveSubItem(int previousIndex, int newIndex) {
		if (previousIndex < 0 or newIndex < 0 or previousIndex >= _items.size() or newIndex >= _items.size()) {
			return false;
		}

		_items.move(previousIndex, newIndex);
		return true;
	}

	inline DocumentItem* parentDocumentItem() {
		return qobject_cast<DocumentItem*>(parent());
	}

	inline void insertSubItem(DocumentItem* item, int position = -1) {

		item->setParent(this);

		if (position < 0) {
			_items.insert((_items.size()+1+position)%(_items.size()+1), item);
			return;
		}

		if (position >= _items.size()) {
			_items.insert(_items.size(), item);
			return;
		}

		_items.insert(position, item);
	}

	inline QList<DocumentItem*> const& subitems() const {
		return _items;
	}

	int pageId();

	inline QPointF origin() const {
		switch(_direction) {
		case Left2Right:
			return QPointF(posX(), posY());
		case Right2Left:
			return QPointF(posX()+initialWidth(), posY());
		case Top2Bottom:
			return QPointF(posX(), posY());
		case Bottom2Top:
			return QPointF(posX(), posY()+initialHeight());
		}
		return QPointF(posX(), posY());
	}

	inline QSizeF initialSize() const {
		return QSizeF(initialWidth(), initialHeight());
	}

	inline QSizeF maxSize() const {
		return QSizeF(maxWidth(), maxHeight());
	}

	QList<Type> supportedSubTypes();

    QJsonValue encapsulateToJson() const;
	static DocumentItem* buildFromJson(QJsonValue const& value);

Q_SIGNALS:

    void directionChanged();
	void overflowBehaviorChanged();

    void posXChanged();
    void posYChanged();

	void initialWidthChanged();
	void initialHeightChanged();

    void maxWidthChanged();
    void maxHeightChanged();

    void borderWidthChanged();
    void borderColorChanged();

    void fillColorChanged();

	void fontSizeChanged();
	void textAlignChanged();
	void fontWeightChanged();

    void anchorChanged();

    void datakeyChanged();
	void dataChanged();

protected:

    bool propertyIsStoredForCurrentType(const char* propName) const;

    Type _type;

    Direction _direction;
	OverflowBehavior _overflowBehavior;

    qreal _pos_x;
    qreal _pos_y;

	qreal _initial_width;
	qreal _initial_height;

    qreal _max_width;
    qreal _max_height;

    qreal _border_width;
    QColor _border_color;

    QColor _fill_color;

	qreal _font_size;
	TextAlign _text_align;
	int _font_weight;

    QString _data_key;
    QString _data;

	QList<DocumentItem*> _items;
};

#endif // DOCUMENTITEM_H
