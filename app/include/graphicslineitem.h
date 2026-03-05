#ifndef GRAPHICSLINEITEM_H
#define GRAPHICSLINEITEM_H

#include "graphicspolygonitem.h"

class GraphicsLineItem : public GraphicsPolygonItem
{
public:
    GraphicsLineItem(QGraphicsItem * parent = 0);
    QPainterPath shape() const;
    QGraphicsItem *duplicate() const;
    void addPoint( const QPointF & point ) ;
    void endPoint(const QPointF & point );
    virtual QPointF opposite( int handle ) ;
    void updateCoordinate() { m_initialPoints = m_points;}
    int handleCount() const { return static_cast<int>(m_handles.size()) + Left;}
    void stretch( int handle , double sx , double sy , const QPointF & origin );
    virtual bool loadFromXml(QXmlStreamReader * xml );
    virtual bool saveToXml( QXmlStreamWriter * xml );
    QString displayName() const { return tr("line"); }
protected:
    void updatehandles();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

};

#endif // GRAPHICSLINEITEM_H
