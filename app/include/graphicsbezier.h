#ifndef GRAPHICSBEZIER_H
#define GRAPHICSBEZIER_H

#include "graphicspolygonitem.h"

class GraphicsBezier : public GraphicsPolygonItem
{
public:
    GraphicsBezier(bool bbezier = true , QGraphicsItem * parent = 0);
    QPainterPath shape() const;
    QGraphicsItem *duplicate() const;
    virtual bool loadFromXml(QXmlStreamReader * xml );
    virtual bool saveToXml( QXmlStreamWriter * xml );
    QString displayName() const { return tr("bezier"); }
protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
private:
    bool m_isBezier;
};

#endif // GRAPHICSBEZIER_H
