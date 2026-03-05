#include "graphicsitemgroup.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphicsItemGroup::GraphicsItemGroup(QGraphicsItem *parent)
    :AbstractShapeType <QGraphicsItemGroup>(parent),
      m_parent(parent)
{
    itemsBoundingRect = QRectF();
    m_handles.reserve(Left);
    for (int i = LeftTop; i <= Left; ++i) {
        SizeHandleRect *shr = new SizeHandleRect(this, i);
        m_handles.push_back(shr);
    }
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    this->setAcceptHoverEvents(true);
}

QRectF GraphicsItemGroup::boundingRect() const
{
    return itemsBoundingRect;
}

GraphicsItemGroup::~GraphicsItemGroup()
{

}

bool GraphicsItemGroup::loadFromXml(QXmlStreamReader *xml)
{
    Q_UNUSED(xml);
    return true;
}

bool GraphicsItemGroup::saveToXml(QXmlStreamWriter *xml)
{
    xml->writeStartElement("group");
    xml->writeAttribute(QLatin1String("x"),QString("%1").arg(pos().x()));
    xml->writeAttribute(QLatin1String("y"),QString("%1").arg(pos().y()));
    xml->writeAttribute(QLatin1String("rotate"),QString("%1").arg(rotation()));

    foreach (QGraphicsItem * item , childItems()) {
        removeFromGroup(item);
        AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(item);
        if ( ab &&!qgraphicsitem_cast<SizeHandleRect*>(ab)){
            ab->updateCoordinate();
            ab->saveToXml(xml);
        }
        addToGroup(item);
    }
    xml->writeEndElement();
    return true;
}

QGraphicsItem *GraphicsItemGroup::duplicate() const
{
    GraphicsItemGroup *item = 0;
    QList<QGraphicsItem*> copylist = duplicateItems();
    item = createGroup(copylist);
    item->setPos(pos().x(),pos().y());
    item->setPen(pen());
    item->setBrush(brush());
    item->setTransform(transform());
    item->setTransformOriginPoint(transformOriginPoint());
    item->setRotation(rotation());
    item->setScale(scale());
    item->setZValue(zValue()+0.1);
    item->updateCoordinate();
    item->m_width = m_width;
    item->m_height = m_height;
    return item;
}

void GraphicsItemGroup::control(int dir, const QPointF &delta)
{
    QPointF local = mapFromParent(delta);
    if ( dir < Left ) return ;
    updatehandles();
}

void GraphicsItemGroup::stretch(int handle, double sx, double sy, const QPointF &origin)
{
    QTransform trans ;
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

    foreach (QGraphicsItem *item , childItems()) {
         AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(item);
         if (ab && !qgraphicsitem_cast<SizeHandleRect*>(ab)){
             ab->stretch(handle,sx,sy,ab->mapFromParent(origin));
         }
    }

    trans.translate(origin.x(),origin.y());
    trans.scale(sx,sy);
    trans.translate(-origin.x(),-origin.y());

    prepareGeometryChange();
    itemsBoundingRect = trans.mapRect(m_initialRect);
    m_width = itemsBoundingRect.width();
    m_height = itemsBoundingRect.height();
    updatehandles();

}

void GraphicsItemGroup::updateCoordinate()
{

    QPointF pt1,pt2,delta;
    if (itemsBoundingRect.isNull() )
        itemsBoundingRect = QGraphicsItemGroup::boundingRect();

    pt1 = mapToScene(transformOriginPoint());
    pt2 = mapToScene(itemsBoundingRect.center());
    delta = pt1 - pt2;
    m_initialRect = itemsBoundingRect;
    m_width = itemsBoundingRect.width();
    m_height = itemsBoundingRect.height();
    setTransform(transform().translate(delta.x(),delta.y()));
    setTransformOriginPoint(itemsBoundingRect.center());
    moveBy(-delta.x(),-delta.y());

    foreach (QGraphicsItem *item , childItems()) {
         AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(item);
         if (ab && !qgraphicsitem_cast<SizeHandleRect*>(ab))
             ab->updateCoordinate();
    }
    updatehandles();
}

GraphicsItemGroup *GraphicsItemGroup::createGroup(const QList<QGraphicsItem *> &items) const
{
    QList<QGraphicsItem *> ancestors;
    int n = 0;
    if (!items.isEmpty()) {
        QGraphicsItem *parent = items.at(n++);
        while ((parent = parent->parentItem()))
            ancestors.append(parent);
    }

    QGraphicsItem *commonAncestor = 0;
    if (!ancestors.isEmpty()) {
        while (n < items.size()) {
            int commonIndex = -1;
            QGraphicsItem *parent = items.at(n++);
            do {
                int index = ancestors.indexOf(parent, qMax(0, commonIndex));
                if (index != -1) {
                    commonIndex = index;
                    break;
                }
            } while ((parent = parent->parentItem()));

            if (commonIndex == -1) {
                commonAncestor = 0;
                break;
            }

            commonAncestor = ancestors.at(commonIndex);
        }
    }
    GraphicsItemGroup *group = new GraphicsItemGroup(commonAncestor);
    foreach (QGraphicsItem *item, items){
        item->setSelected(false);
        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
        if ( !g )
             group->addToGroup(item);
    }
    return group;
}

QList<QGraphicsItem *> GraphicsItemGroup::duplicateItems() const
{
    QList<QGraphicsItem*> copylist ;
    foreach (QGraphicsItem * shape , childItems() ) {
        AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(shape);
        if ( ab && !qgraphicsitem_cast<SizeHandleRect*>(ab)){
            QGraphicsItem * cp = ab->duplicate();
            copylist.append(cp);
        }
    }
    return copylist;
}

void GraphicsItemGroup::updatehandles()
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
        case 9:
            hndl->move(transformOriginPoint().x(),transformOriginPoint().y());
            break;
        default:
            break;
        }
    }
}

QVariant GraphicsItemGroup::itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value)
{
    if ( change == QGraphicsItem::ItemSelectedHasChanged ) {
        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(parentItem());
        if (!g)
            setState(value.toBool() ? SelectionHandleActive : SelectionHandleOff);
        else{
            setSelected(false);
            return QVariant::fromValue<bool>(false);
        }
        if( value.toBool()){
            updateCoordinate();
        }
    }

    return QGraphicsItemGroup::itemChange(change, value);
}

void GraphicsItemGroup::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    if (option->state & QStyle::State_Selected)
        qt_graphicsItem_highlightSelected(this, painter, option);
}
