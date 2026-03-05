#include "graphicslineitem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphicsLineItem::GraphicsLineItem(QGraphicsItem *parent)
    :GraphicsPolygonItem(parent)
{
    m_pen = QPen(Qt::black);
    m_handles.reserve(Left);

    Handles::iterator hend =  m_handles.end();
    for (Handles::iterator it = m_handles.begin(); it != hend; ++it)
       delete (*it);
    m_handles.clear();
}

QPainterPath GraphicsLineItem::shape() const
{
    QPainterPath path;
    if ( m_points.size() > 1 ){
        path.moveTo(m_points.at(0));
        path.lineTo(m_points.at(1));
    }
    return qt_graphicsItem_shapeFromPath(path,pen());
}

QGraphicsItem *GraphicsLineItem::duplicate() const
{
    GraphicsLineItem * item = new GraphicsLineItem();
    item->m_width = width();
    item->m_height = height();
    item->m_points = m_points;
    item->m_initialPoints = m_initialPoints;
    item->setPos(pos().x(),pos().y());
    item->setPen(pen());
    item->setBrush(brush());
    item->setTransform(transform());
    item->setTransformOriginPoint(transformOriginPoint());
    item->setRotation(rotation());
    item->setScale(scale());
    item->setZValue(zValue()+0.1);
    item->updateCoordinate();
    return item;
}

void GraphicsLineItem::addPoint(const QPointF &point)
{
    m_points.append(mapFromScene(point));
    int dir = m_points.count();
    SizeHandleRect *shr = new SizeHandleRect(this, dir+Left, dir == 1 ? false : true);
    shr->setState(SelectionHandleActive);
    m_handles.push_back(shr);
}

void GraphicsLineItem::endPoint(const QPointF &point)
{
    Q_UNUSED(point);
    int nPoints = m_points.count();
    if( nPoints > 2 && (m_points[nPoints-1] == m_points[nPoints-2] ||
        m_points[nPoints-1].x() - 1 == m_points[nPoints-2].x() &&
        m_points[nPoints-1].y() == m_points[nPoints-2].y())){
        delete m_handles[ nPoints-1];
        m_points.remove(nPoints-1);
        m_handles.resize(nPoints-1);
    }
    m_initialPoints = m_points;
}

QPointF GraphicsLineItem::opposite(int handle)
{
    QPointF pt;
    switch (handle) {
    case Right:
    case Left:
    case Top:
    case LeftTop:
    case RightTop:
        pt = m_handles[1]->pos();
        break;
    case RightBottom:
    case LeftBottom:
    case Bottom:
        pt = m_handles[0]->pos();
        break;
     }
    return pt;
}

void GraphicsLineItem::stretch(int handle, double sx, double sy, const QPointF &origin)
{
    QTransform trans;
    switch (handle) {
    case Right:
    case Left:
        sy = 1;
        break;
    case Top:
    case Bottom:
        sx = 1;
        break;
    default:
        break;
    }
    trans.translate(origin.x(),origin.y());
    trans.scale(sx,sy);
    trans.translate(-origin.x(),-origin.y());

    prepareGeometryChange();
    m_points = trans.map(m_initialPoints);
    m_localRect = m_points.boundingRect();
    m_width = m_localRect.width();
    m_height = m_localRect.height();
    updatehandles();
}

bool GraphicsLineItem::loadFromXml(QXmlStreamReader *xml)
{
    readBaseAttributes(xml);
    while(xml->readNextStartElement()){
        if (xml->name()=="point"){
            qreal x = xml->attributes().value("x").toDouble();
            qreal y = xml->attributes().value("y").toDouble();
            m_points.append(QPointF(x,y));
            int dir = m_points.count();
            SizeHandleRect *shr = new SizeHandleRect(this, dir+Left, dir == 1 ? false : true);
            m_handles.push_back(shr);
            xml->skipCurrentElement();
        }else
            xml->skipCurrentElement();
    }
    updatehandles();
    return true;
}

bool GraphicsLineItem::saveToXml(QXmlStreamWriter *xml)
{
    xml->writeStartElement("line");
    writeBaseAttributes(xml);
    for ( int i = 0 ; i < m_points.count();++i){
        xml->writeStartElement("point");
        xml->writeAttribute("x",QString("%1").arg(m_points[i].x()));
        xml->writeAttribute("y",QString("%1").arg(m_points[i].y()));
        xml->writeEndElement();
    }
    xml->writeEndElement();
    return true;
}

void GraphicsLineItem::updatehandles()
{
    for ( int i = 0 ; i < m_points.size() ; ++i ){
        m_handles[i]->move(m_points[i].x() ,m_points[i].y() );
    }
}

void GraphicsLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(pen());
    if ( m_points.size() > 1)
    painter->drawLine(m_points.at(0),m_points.at(1));
}
