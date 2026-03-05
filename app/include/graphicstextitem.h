#ifndef GRAPHICSTEXTITEM_H
#define GRAPHICSTEXTITEM_H

#include <QFont>
#include <QColor>
#include "graphicsrectitem.h"

class GraphicsTextItem : public GraphicsRectItem
{
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QFont font READ font WRITE setFont)
    Q_PROPERTY(QColor textColor READ textColor WRITE setTextColor)
    Q_PROPERTY(bool italic READ italic WRITE setItalic)
public:
    GraphicsTextItem(const QRect & rect, QGraphicsItem * parent = 0);

    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QPainterPath shape() const Q_DECL_OVERRIDE;
    QGraphicsItem *duplicate() const Q_DECL_OVERRIDE;

    QString displayName() const { return tr("text"); }

    QString text() const { return m_text; }
    void setText(const QString &t) { m_text = t; update(); }

    QFont font() const { return m_font; }
    void setFont(const QFont &f) { m_font = f; update(); }

    QColor textColor() const { return m_textColor; }
    void setTextColor(const QColor &c) { m_textColor = c; update(); }

    bool italic() const { return m_font.italic(); }
    void setItalic(bool on) { m_font.setItalic(on); update(); }

    virtual bool loadFromXml(QXmlStreamReader * xml);
    virtual bool saveToXml( QXmlStreamWriter * xml);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;

    QString m_text;
    QFont   m_font;
    QColor  m_textColor;
};

#endif // GRAPHICSTEXTITEM_H

