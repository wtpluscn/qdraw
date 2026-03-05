#include "drawobj.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsItemGroup>
#include <QGraphicsEllipseItem>
#include <QGraphicsPathItem>
#include <QGraphicsPolygonItem>
#include <QGraphicsRectItem>
#include <QGraphicsLineItem>
#include <QGraphicsSimpleTextItem>
#include <QDebug>

ShapeMimeData::ShapeMimeData(QList<QGraphicsItem *> items)
{
    foreach (QGraphicsItem *item , items ) {
       AbstractShape *sp = qgraphicsitem_cast<AbstractShape*>(item);
       m_items.append(sp->duplicate());
    }
}
ShapeMimeData::~ShapeMimeData()
{
    foreach (QGraphicsItem *item , m_items ) {
        delete item;
    }
    m_items.clear();
}

QList<QGraphicsItem *> ShapeMimeData::items() const
{
    return m_items;
}

QPainterPath qt_graphicsItem_shapeFromPath(const QPainterPath &path, const QPen &pen)
{
    const qreal penWidthZero = qreal(0.00000001);

    if (path == QPainterPath() || pen == Qt::NoPen)
        return path;
    QPainterPathStroker ps;
    ps.setCapStyle(pen.capStyle());
    if (pen.widthF() <= 0.0)
        ps.setWidth(penWidthZero);
    else
        ps.setWidth(pen.widthF());
    ps.setJoinStyle(pen.joinStyle());
    ps.setMiterLimit(pen.miterLimit());
    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    return p;
}

void qt_graphicsItem_highlightSelected(
    QGraphicsItem *item, QPainter *painter, const QStyleOptionGraphicsItem *option)
{
    const QRectF murect = painter->transform().mapRect(QRectF(0, 0, 1, 1));
    if (qFuzzyIsNull(qMax(murect.width(), murect.height())))
        return;

    const QRectF mbrect = painter->transform().mapRect(item->boundingRect());
    if (qMin(mbrect.width(), mbrect.height()) < qreal(1.0))
        return;

    qreal itemPenWidth;
    switch (item->type()) {
        case QGraphicsEllipseItem::Type:
            itemPenWidth = static_cast<QGraphicsEllipseItem *>(item)->pen().widthF();
            break;
        case QGraphicsPathItem::Type:
            itemPenWidth = static_cast<QGraphicsPathItem *>(item)->pen().widthF();
            break;
        case QGraphicsPolygonItem::Type:
            itemPenWidth = static_cast<QGraphicsPolygonItem *>(item)->pen().widthF();
            break;
        case QGraphicsRectItem::Type:
            itemPenWidth = static_cast<QGraphicsRectItem *>(item)->pen().widthF();
            break;
        case QGraphicsSimpleTextItem::Type:
            itemPenWidth = static_cast<QGraphicsSimpleTextItem *>(item)->pen().widthF();
            break;
        case QGraphicsLineItem::Type:
            itemPenWidth = static_cast<QGraphicsLineItem *>(item)->pen().widthF();
            break;
        default:
            itemPenWidth = 1.0;
    }
    const qreal pad = itemPenWidth / 2;
    const qreal penWidth = 0;
    const QColor fgcolor = option->palette.windowText().color();
    const QColor bgcolor(
        fgcolor.red()   > 127 ? 0 : 255,
        fgcolor.green() > 127 ? 0 : 255,
        fgcolor.blue()  > 127 ? 0 : 255);

    painter->setPen(QPen(bgcolor, penWidth, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(item->boundingRect().adjusted(-pad, -pad, pad, pad));

    painter->setPen(QPen(QColor("lightskyblue"), 0, Qt::SolidLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(item->boundingRect().adjusted(-pad, -pad, pad, pad));
}

GraphicsItem::GraphicsItem(QGraphicsItem *parent)
    :AbstractShapeType<QGraphicsItem>(parent)
{
    m_handles.reserve(Left);
    for (int i = LeftTop; i <= Left; ++i) {
        SizeHandleRect *shr = new SizeHandleRect(this,i);
        m_handles.push_back(shr);
    }

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    this->setAcceptHoverEvents(true);
}

QPixmap GraphicsItem::image() {
    QPixmap pixmap(64, 64);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    setPen(QPen(Qt::black));
    setBrush(Qt::white);
    QStyleOptionGraphicsItem *styleOption = new QStyleOptionGraphicsItem;
    paint(&painter,styleOption);
    delete styleOption;
    return pixmap;
}

void GraphicsItem::updatehandles()
{
    const QRectF &geom = this->boundingRect();

    const Handles::iterator hend =  m_handles.end();
    for (Handles::iterator it = m_handles.begin(); it != hend; ++it) {
        SizeHandleRect *hndl = *it;;
        switch (hndl->dir()) {
        case LeftTop:
            hndl->move(geom.x() , geom.y() );
            break;
        case Top:
            hndl->move(geom.x() + geom.width() / 2 , geom.y() );
            break;
        case RightTop:
            hndl->move(geom.x() + geom.width() , geom.y() );
            break;
        case Right:
            hndl->move(geom.x() + geom.width() , geom.y() + geom.height() / 2 );
            break;
        case RightBottom:
            hndl->move(geom.x() + geom.width() , geom.y() + geom.height() );
            break;
        case Bottom:
            hndl->move(geom.x() + geom.width() / 2 , geom.y() + geom.height() );
            break;
        case LeftBottom:
            hndl->move(geom.x(), geom.y() + geom.height());
            break;
        case Left:
            hndl->move(geom.x(), geom.y() + geom.height() / 2);
            break;
        default:
            break;
        }
    }
}

void GraphicsItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
    Q_UNUSED(event);
}

bool GraphicsItem::readBaseAttributes(QXmlStreamReader *xml)
{
    qreal x = xml->attributes().value(QLatin1String("x")).toDouble();
    qreal y = xml->attributes().value(QLatin1String("y")).toDouble();
    m_width = xml->attributes().value(QLatin1String("width")).toDouble();
    m_height = xml->attributes().value(QLatin1String("height")).toDouble();
    setZValue(xml->attributes().value(QLatin1String("z")).toDouble());
    setRotation(xml->attributes().value(QLatin1String("rotate")).toDouble());
    setPos(x,y);
    return true;
}

bool GraphicsItem::writeBaseAttributes(QXmlStreamWriter *xml)
{
    xml->writeAttribute(QLatin1String("rotate"),QString("%1").arg(rotation()));
    xml->writeAttribute(QLatin1String("x"),QString("%1").arg(pos().x()));
    xml->writeAttribute(QLatin1String("y"),QString("%1").arg(pos().y()));
    xml->writeAttribute(QLatin1String("z"),QString("%1").arg(zValue()));
    xml->writeAttribute(QLatin1String("width"),QString("%1").arg(m_width));
    xml->writeAttribute(QLatin1String("height"),QString("%1").arg(m_height));
    return true;
}

QVariant GraphicsItem::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if ( change == QGraphicsItem::ItemSelectedHasChanged ) {
        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(parentItem());
        if (!g)
            setState(value.toBool() ? SelectionHandleActive : SelectionHandleOff);
        else{
            setSelected(false);
            return QVariant::fromValue<bool>(false);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}
