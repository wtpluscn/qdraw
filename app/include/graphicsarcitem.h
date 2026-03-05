#ifndef GRAPHICSARCITEM_H
#define GRAPHICSARCITEM_H

#include "graphicsrectitem.h"

class GraphicsArcItem : public GraphicsRectItem
{
public:
    GraphicsArcItem(const QRect & rect, QGraphicsItem * parent = 0);
    QPainterPath shape() const Q_DECL_OVERRIDE;
    void control(int dir, const QPointF & delta) Q_DECL_OVERRIDE;
    QRectF boundingRect() const Q_DECL_OVERRIDE;
    QGraphicsItem *duplicate() const Q_DECL_OVERRIDE;
    QString displayName() const { return tr("arc"); }
    virtual bool loadFromXml(QXmlStreamReader * xml);
    virtual bool saveToXml( QXmlStreamWriter * xml);
protected:
    void updatehandles();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) Q_DECL_OVERRIDE;
    int m_startAngle;
    int m_spanAngle;
};

#endif // GRAPHICSARCITEM_H
