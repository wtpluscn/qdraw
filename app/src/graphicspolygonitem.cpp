#include "graphicspolygonitem.h"
#include <QPainter>
#include <QLinearGradient>
#include <QStyleOptionGraphicsItem>

GraphicsPolygonItem::GraphicsPolygonItem(QGraphicsItem *parent)
    :GraphicsItem(parent)
{
    m_points.clear();
    m_pen = QPen(Qt::black);
}

QRectF GraphicsPolygonItem::boundingRect() const
{
    return shape().controlPointRect();
}

QPainterPath GraphicsPolygonItem::shape() const
{
    QPainterPath path;
    path.addPolygon(m_points);
    path.closeSubpath();
    return qt_graphicsItem_shapeFromPath(path,pen());
}

void GraphicsPolygonItem::addPoint(const QPointF &point)
{
    m_points.append(mapFromScene(point));
    int dir = m_points.count();
    SizeHandleRect *shr = new SizeHandleRect(this, dir+Left, true);
    shr->setState(SelectionHandleActive);
    m_handles.push_back(shr);
}

void GraphicsPolygonItem::control(int dir, const QPointF &delta)
{
    QPointF pt = mapFromScene(delta);
    if ( dir <= Left ) return ;
    m_points[dir - Left -1] = pt;
    prepareGeometryChange();
    m_localRect = m_points.boundingRect();
    m_width = m_localRect.width();
    m_height = m_localRect.height();
    m_initialPoints = m_points;
    updatehandles();
}

void GraphicsPolygonItem::stretch(int handle, double sx, double sy, const QPointF &origin)
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

void GraphicsPolygonItem::updateCoordinate()
{

    QPointF pt1,pt2,delta;
    QPolygonF pts = mapToScene(m_points);
    if (parentItem()==NULL)
    {
        pt1 = mapToScene(transformOriginPoint());
        pt2 = mapToScene(boundingRect().center());
        delta = pt1 - pt2;

        for (int i = 0; i < pts.count() ; ++i )
            pts[i]+=delta;

        prepareGeometryChange();

        m_points = mapFromScene(pts);
        m_localRect = m_points.boundingRect();
        m_width = m_localRect.width();
        m_height = m_localRect.height();

        setTransform(transform().translate(delta.x(),delta.y()));
        moveBy(-delta.x(),-delta.y());
        setTransform(transform().translate(-delta.x(),-delta.y()));
        updatehandles();
    }
    m_initialPoints = m_points;

}

bool GraphicsPolygonItem::loadFromXml(QXmlStreamReader *xml)
{
    readBaseAttributes(xml);
    while(xml->readNextStartElement()){
        if (xml->name()=="point"){
            qreal x = xml->attributes().value("x").toDouble();
            qreal y = xml->attributes().value("y").toDouble();
            m_points.append(QPointF(x,y));
            int dir = m_points.count();
            SizeHandleRect *shr = new SizeHandleRect(this, dir+Left, true);
            m_handles.push_back(shr);
            xml->skipCurrentElement();
        }else
            xml->skipCurrentElement();
    }
    updateCoordinate();
    return true;
}

bool GraphicsPolygonItem::saveToXml(QXmlStreamWriter *xml)
{
    xml->writeStartElement("polygon");
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

void GraphicsPolygonItem::endPoint(const QPointF & point)
{
    Q_UNUSED(point);
    int nPoints = m_points.count();
    if( nPoints > 2 && (m_points[nPoints-1] == m_points[nPoints-2] ||
        m_points[nPoints-1].x() - 1 == m_points[nPoints-2].x() &&
        m_points[nPoints-1].y() == m_points[nPoints-2].y())){
        delete m_handles[Left + nPoints-1];
        m_points.remove(nPoints-1);
        m_handles.resize(Left + nPoints-1);
    }
    m_initialPoints = m_points;
}

QGraphicsItem *GraphicsPolygonItem::duplicate() const
{
    GraphicsPolygonItem * item = new GraphicsPolygonItem( );
    item->m_width = width();
    item->m_height = height();
    item->m_points = m_points;

    for ( int i = 0 ; i < m_points.size() ; ++i ){
        item->m_handles.push_back(new SizeHandleRect(item,Left+i+1,true));
    }

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

void GraphicsPolygonItem::updatehandles()
{
    GraphicsItem::updatehandles();

    for ( int i = 0 ; i < m_points.size() ; ++i ){
        m_handles[Left+i]->move(m_points[i].x() ,m_points[i].y() );
    }
}

void GraphicsPolygonItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);

    QColor c = brushColor();
    QLinearGradient result(boundingRect().topLeft(), boundingRect().topRight());
    result.setColorAt(0, c.dark(150));
    result.setColorAt(0.5, c.light(200));
    result.setColorAt(1, c.dark(150));
    painter->setBrush(result);

    painter->setPen(pen());
    painter->drawPolygon(m_points);

    if (option->state & QStyle::State_Selected)
        qt_graphicsItem_highlightSelected(this, painter, option);
}
