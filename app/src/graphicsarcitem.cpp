#include "graphicsarcitem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <cmath>

#define M_PI 3.1415926

GraphicsArcItem::GraphicsArcItem(const QRect & rect, QGraphicsItem * parent)
    : GraphicsRectItem(rect, false, parent)
{
    m_brush = QBrush(Qt::NoBrush);
    m_startAngle = 0;
    m_spanAngle = 90;
    SizeHandleRect *shr = new SizeHandleRect(this, 9, true);
    m_handles.push_back(shr);
    shr = new SizeHandleRect(this, 10, true);
    m_handles.push_back(shr);
    updatehandles();
}

QPainterPath GraphicsArcItem::shape() const
{
    QPainterPath path;
    int startAngle = m_startAngle <= m_spanAngle ? m_startAngle : m_spanAngle;
    int endAngle = m_startAngle >= m_spanAngle ? m_startAngle : m_spanAngle;
    if (endAngle - startAngle > 360)
        endAngle = startAngle + 360;
    if (m_localRect.isNull())
        return path;
    path.arcTo(m_localRect, startAngle, endAngle - startAngle);
    return qt_graphicsItem_shapeFromPath(path, pen());
}

void GraphicsArcItem::control(int dir, const QPointF & delta)
{
    QPointF local = mapFromScene(delta);
    switch (dir) {
    case 9: {
        qreal len_y = local.y() - m_localRect.center().y();
        qreal len_x = local.x() - m_localRect.center().x();
        m_startAngle = -atan2(len_y, len_x) * 180 / M_PI;
        break;
    }
    case 10: {
        qreal len_y = local.y() - m_localRect.center().y();
        qreal len_x = local.x() - m_localRect.center().x();
        m_spanAngle = -atan2(len_y, len_x) * 180 / M_PI;
        break;
    }
    default:
        break;
    }
    prepareGeometryChange();
    if (m_startAngle > m_spanAngle)
        m_startAngle -= 360;
    if (m_spanAngle < m_startAngle) {
        qreal tmp = m_spanAngle;
        m_spanAngle = m_startAngle;
        m_startAngle = tmp;
    }
    if (qAbs(m_spanAngle - m_startAngle) > 360) {
        m_startAngle = 0;
        m_spanAngle = 90;
    }
    updatehandles();
}

QRectF GraphicsArcItem::boundingRect() const
{
    return shape().controlPointRect();
}

QGraphicsItem *GraphicsArcItem::duplicate() const
{
    GraphicsArcItem *item = new GraphicsArcItem(m_localRect.toRect());
    item->m_width = width();
    item->m_height = height();
    item->m_startAngle = m_startAngle;
    item->m_spanAngle = m_spanAngle;
    item->setPos(pos().x(), pos().y());
    item->setPen(pen());
    item->setBrush(brush());
    item->setTransform(transform());
    item->setTransformOriginPoint(transformOriginPoint());
    item->setRotation(rotation());
    item->setScale(scale());
    item->setZValue(zValue() + 0.1);
    item->updateCoordinate();
    return item;
}

bool GraphicsArcItem::loadFromXml(QXmlStreamReader *xml)
{
    m_startAngle = xml->attributes().value(QLatin1String("startAngle")).toInt();
    m_spanAngle  = xml->attributes().value(QLatin1String("spanAngle")).toInt();
    readBaseAttributes(xml);
    xml->skipCurrentElement();
    updateCoordinate();
    return true;
}

bool GraphicsArcItem::saveToXml(QXmlStreamWriter *xml)
{
    xml->writeStartElement(QLatin1String("arc"));
    xml->writeAttribute(QLatin1String("startAngle"), QString("%1").arg(m_startAngle));
    xml->writeAttribute(QLatin1String("spanAngle"), QString("%1").arg(m_spanAngle));
    writeBaseAttributes(xml);
    xml->writeEndElement();
    return true;
}

void GraphicsArcItem::updatehandles()
{
    GraphicsItem::updatehandles();
    QRectF local = QRectF(-m_width/2, -m_height/2, m_width, m_height);
    QPointF delta = local.center() - m_localRect.center();
    qreal x = (m_width/2) * cos(-m_startAngle * M_PI / 180);
    qreal y = (m_height/2) * sin(-m_startAngle * M_PI / 180);
    m_handles.at(8)->move(x - delta.x(), y - delta.y());
    x = (m_width/2) * cos(-m_spanAngle * M_PI / 180);
    y = (m_height/2) * sin(-m_spanAngle * M_PI / 180);
    m_handles.at(9)->move(x - delta.x(), y - delta.y());
}

void GraphicsArcItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    int startAngle = m_startAngle <= m_spanAngle ? m_startAngle : m_spanAngle;
    int endAngle = m_startAngle >= m_spanAngle ? m_startAngle : m_spanAngle;
    if (endAngle - startAngle > 360)
        endAngle = startAngle + 360;
    painter->setPen(pen());
    painter->setBrush(Qt::NoBrush);
    painter->drawArc(m_localRect, startAngle * 16, (endAngle - startAngle) * 16);
    if (option->state & QStyle::State_Selected)
        qt_graphicsItem_highlightSelected(this, painter, option);
}
