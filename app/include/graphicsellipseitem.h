#ifndef GRAPHICELLIPSEITEM_H
#define GRAPHICELLIPSEITEM_H

#include "graphicsrectitem.h"

class GraphicsEllipseItem :public GraphicsRectItem
{
public:
    GraphicsEllipseItem(const QRect & rect , QGraphicsItem * parent = 0);
    QPainterPath shape() const;
    void control(int dir, const QPointF & delta );
    QRectF boundingRect() const ;
    QGraphicsItem *duplicate() const;
    QString displayName() const { return tr("ellipse"); }
    virtual bool loadFromXml(QXmlStreamReader * xml );
    virtual bool saveToXml( QXmlStreamWriter * xml );
protected:
    void updatehandles();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    int   m_startAngle;
    int   m_spanAngle;
};

#endif // GRAPHICELLIPSEITEM_H
