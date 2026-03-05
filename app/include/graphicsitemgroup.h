#ifndef GRAPHICSITEMGROUP_H
#define GRAPHICSITEMGROUP_H

#include <QGraphicsItemGroup>
#include "drawobj.h"

class GraphicsItemGroup : public QObject,
        public AbstractShapeType <QGraphicsItemGroup>
{
    Q_OBJECT
    Q_PROPERTY(QColor pen READ penColor WRITE setPen )
    Q_PROPERTY(QColor brush READ brushColor WRITE setBrushColor )
    Q_PROPERTY(qreal  width READ width WRITE setWidth )
    Q_PROPERTY(qreal  height READ height WRITE setHeight )
    Q_PROPERTY(QPointF  position READ pos WRITE setPos )

public:
    enum {Type = UserType+2};
    int  type() const { return Type; }
    explicit GraphicsItemGroup(QGraphicsItem *parent = 0);
    QRectF boundingRect() const;
    ~GraphicsItemGroup();

    QString displayName() const { return tr("group"); }

    virtual bool loadFromXml(QXmlStreamReader * xml );
    virtual bool saveToXml( QXmlStreamWriter * xml );

    QGraphicsItem *duplicate () const ;
    void control(int dir, const QPointF & delta);
    void stretch( int handle , double sx , double sy , const QPointF & origin );
    void updateCoordinate();
signals:
    void selectedChange(QGraphicsItem *item);

protected:
    GraphicsItemGroup * createGroup(const QList<QGraphicsItem *> &items) const;
    QList<QGraphicsItem *> duplicateItems() const;
    void updatehandles();
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
    QGraphicsItem * m_parent;
    QRectF itemsBoundingRect;
    QRectF m_initialRect;
};

#endif // GRAPHICSITEMGROUP_H
