#ifndef GRAPHICSRECTITEM_H
#define GRAPHICSRECTITEM_H

#include "drawobj.h"

class GraphicsRectItem : public GraphicsItem
{
public:
    GraphicsRectItem(const QRect & rect , bool isRound = false ,QGraphicsItem * parent = 0 );
    QRectF boundingRect() const;
    QPainterPath shape() const;
    void control(int dir, const QPointF & delta);
    void stretch(int handle , double sx , double sy , const QPointF & origin);
    QRectF  rect() const {  return m_localRect;}
    void updateCoordinate();
    void move( const QPointF & point );
    QGraphicsItem *duplicate () const ;
    QString displayName() const { return tr("rectangle"); }

    virtual bool loadFromXml(QXmlStreamReader * xml );
    virtual bool saveToXml( QXmlStreamWriter * xml );

protected:
    void updatehandles();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    bool m_isRound;
    qreal m_fRatioY;
    qreal m_fRatioX;
    QRectF m_initialRect;
    QPointF opposite_;
    QPointF m_originPoint;
};

#endif // GRAPHICSRECTITEM_H
