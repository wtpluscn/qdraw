#include "drawview.h"
#include "drawscene.h"
#include "drawshapes.h"

#include <QSvgGenerator>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QImage>
#include <QPainter>
#include <QUndoStack>
#include <QFileInfo>

// http://www.w3.org/TR/SVG/Overview.html

DrawView::DrawView(QGraphicsScene *scene)
    : QGraphicsView(scene)
{
    m_hruler = new QtRuleBar(Qt::Horizontal,this,this);
    m_vruler = new QtRuleBar(Qt::Vertical,this,this);
    box = new QtCornerBox(this);
    setViewport(new QWidget);

    setAttribute(Qt::WA_DeleteOnClose);
    isUntitled = true;
    modified = false;

    m_undoStack = new QUndoStack(this);
}

void DrawView::zoomIn()
{
    scale(1.2,1.2);
    updateRuler();
}

void DrawView::zoomOut()
{
    scale(1 / 1.2, 1 / 1.2);
    updateRuler();
}

void DrawView::newFile()
{
    static int sequenceNumber = 1;

    isUntitled = true;
    curFile = tr("drawing%1.xml").arg(sequenceNumber++);
    setWindowTitle(curFile + "[*]");
}

bool DrawView::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Qt Drawing"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QXmlStreamReader xml(&file);
    if (xml.readNextStartElement()) {
        if ( xml.name() == QLatin1String("canvas"))
        {
            int width = xml.attributes().value(QLatin1String("width")).toInt();
            int height = xml.attributes().value(QLatin1String("height")).toInt();
            scene()->setSceneRect(0,0,width,height);
            loadCanvas(&xml);
        }
    }

    setCurrentFile(fileName);
    return !xml.error();
}

bool DrawView::save()
{
    if (isUntitled) {
        return saveAs();
    } else {
        return saveFile(curFile);
    }
}

bool DrawView::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    curFile);
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

bool DrawView::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Qt Drawing"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeDTD("<!DOCTYPE qdraw>");
    xml.writeStartElement("canvas");
    xml.writeAttribute("width",QString("%1").arg(scene()->width()));
    xml.writeAttribute("height",QString("%1").arg(scene()->height()));

    foreach (QGraphicsItem *item , scene()->items()) {
        AbstractShape * ab = qgraphicsitem_cast<AbstractShape*>(item);
        QGraphicsItemGroup *g = dynamic_cast<QGraphicsItemGroup*>(item->parentItem());
        if ( ab && !qgraphicsitem_cast<SizeHandleRect*>(ab) && !g ){
            ab->saveToXml(&xml);
        }
    }
    xml.writeEndElement();
    xml.writeEndDocument();

    setCurrentFile(fileName);
    return true;
}

bool DrawView::exportToPng(const QString &fileName)
{
    QRectF rect = scene()->sceneRect();
    if (!rect.isValid())
        return false;

    QImage image(rect.size().toSize(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    scene()->render(&painter, QRectF(QPointF(0,0), rect.size()), rect);
    painter.end();

    return image.save(fileName, "PNG");
}

QString DrawView::userFriendlyCurrentFile()
{
    return strippedName(curFile);
}

void DrawView::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void DrawView::mouseMoveEvent(QMouseEvent *event)
{
    QPointF pt = mapToScene(event->pos());
    m_hruler->updatePosition(event->pos());
    m_vruler->updatePosition(event->pos());
    emit positionChanged( pt.x() , pt.y() );
    QGraphicsView::mouseMoveEvent(event);
}

void DrawView::resizeEvent(QResizeEvent *event)
{
    QGraphicsView::resizeEvent(event);

    this->setViewportMargins(RULER_SIZE-1,RULER_SIZE-1,0,0);
    m_hruler->resize(this->size().width()- RULER_SIZE - 1,RULER_SIZE);
    m_hruler->move(RULER_SIZE,0);
    m_vruler->resize(RULER_SIZE,this->size().height() - RULER_SIZE - 1);
    m_vruler->move(0,RULER_SIZE);

    box->resize(RULER_SIZE,RULER_SIZE);
    box->move(0,0);
    updateRuler();
}

void DrawView::scrollContentsBy(int dx, int dy)
{
    QGraphicsView::scrollContentsBy(dx,dy);
    updateRuler();
}

void DrawView::updateRuler()
{
    if (!scene())
        return;

    QRectF viewbox = this->rect();
    QPointF offset = mapFromScene(scene()->sceneRect().topLeft());
    double factor =  1./transform().m11();
    double lower_x = factor * ( viewbox.left()  - offset.x() );
    double upper_x = factor * ( viewbox.right() -RULER_SIZE- offset.x()  );
    m_hruler->setRange(lower_x,upper_x,upper_x - lower_x );
    m_hruler->update();

    double lower_y = factor * ( viewbox.top() - offset.y()) * -1;
    double upper_y = factor * ( viewbox.bottom() - RULER_SIZE - offset.y() ) * -1;

    m_vruler->setRange(lower_y,upper_y,upper_y - lower_y );
    m_vruler->update();
}

bool DrawView::maybeSave()
{
    if (isModified()) {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("MDI"),
                     tr("'%1' has been modified.\n"
                        "Do you want to save your changes?")
                     .arg(userFriendlyCurrentFile()),
                     QMessageBox::Save | QMessageBox::Discard
                     | QMessageBox::Cancel);
        if (ret == QMessageBox::Save)
            return save();
        else if (ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void DrawView::setCurrentFile(const QString &fileName)
{
    curFile = QFileInfo(fileName).canonicalFilePath();
    isUntitled = false;
    setModified(false);
    setWindowModified(false);
    setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString DrawView::strippedName(const QString &fullFileName)
{
    return QFileInfo(fullFileName).fileName();
}

AbstractShape *DrawView::createItemFromXmlName(QXmlStreamReader *xml)
{
    const QString name = xml->name().toString();
    if (name == QLatin1String("rect"))
        return new GraphicsRectItem(QRect(0,0,1,1));
    if (name == QLatin1String("roundrect"))
        return new GraphicsRectItem(QRect(0,0,1,1), true);
    if (name == QLatin1String("ellipse"))
        return new GraphicsEllipseItem(QRect(0,0,1,1));
    if (name == QLatin1String("arc"))
        return new GraphicsArcItem(QRect(0,0,1,1));
    if (name == QLatin1String("text"))
        return new GraphicsTextItem(QRect(0,0,80,24));
    if (name == QLatin1String("polygon"))
        return new GraphicsPolygonItem();
    if (name == QLatin1String("bezier"))
        return new GraphicsBezier();
    if (name == QLatin1String("polyline"))
        return new GraphicsBezier(false);
    if (name == QLatin1String("line"))
        return new GraphicsLineItem();
    if (name == QLatin1String("group"))
        return qgraphicsitem_cast<AbstractShape*>(loadGroupFromXML(xml));
    return 0;
}

void DrawView::loadCanvas(QXmlStreamReader *xml)
{
    Q_ASSERT(xml->isStartElement() && xml->name() == QLatin1String("canvas"));

    while (xml->readNextStartElement()) {
        AbstractShape * item = createItemFromXmlName(xml);
        if (item) {
            if (!qgraphicsitem_cast<GraphicsItemGroup*>(item)) {
                if (item->loadFromXml(xml))
                    scene()->addItem(item);
                else
                    delete item;
            } else {
                scene()->addItem(item);
            }
        } else {
            xml->skipCurrentElement();
        }
    }
}

GraphicsItemGroup *DrawView::loadGroupFromXML(QXmlStreamReader *xml)
{
    QList<QGraphicsItem*> items;
    qreal angle = xml->attributes().value(QLatin1String("rotate")).toDouble();
    while (xml->readNextStartElement()) {
        AbstractShape * item = createItemFromXmlName(xml);
        if (item) {
            if (!qgraphicsitem_cast<GraphicsItemGroup*>(item)) {
                if (item->loadFromXml(xml)) {
                    scene()->addItem(item);
                    items.append(item);
                } else {
                    delete item;
                }
            } else {
                scene()->addItem(item);
                items.append(item);
            }
        } else {
            xml->skipCurrentElement();
        }
    }

    if ( items.count() > 0 ){
        DrawScene * s = dynamic_cast<DrawScene*>(scene());
        GraphicsItemGroup * group = s->createGroup(items,false);
        if (group){
            group->setRotation(angle);
            group->updateCoordinate();
        }
        return group;
    }
    return 0;
}

bool DrawView::loadElementIntoScene(QXmlStreamReader *xml, QGraphicsScene *targetScene, QList<QGraphicsItem *> *outList)
{
    if (!targetScene || !outList || xml->name() != QLatin1String("element"))
        return false;
    qDebug() << "[element] loadElementIntoScene start, targetScene rect:"
             << targetScene->sceneRect();
    outList->clear();
    while (xml->readNextStartElement()) {
        AbstractShape *item = createItemFromXmlName(xml);
        if (item) {
            if (!qgraphicsitem_cast<GraphicsItemGroup*>(item)) {
                if (item->loadFromXml(xml)) {
                    targetScene->addItem(item);
                    outList->append(item);
                } else {
                    delete item;
                }
            } else {
                targetScene->addItem(item);
                outList->append(item);
            }
        } else {
            xml->skipCurrentElement();
        }
    }
    qDebug() << "[element] loadElementIntoScene created" << outList->size() << "items";
    return !outList->isEmpty();
}

bool DrawView::loadElementAndInsertAt(const QString &elementPath, const QPointF &scenePos)
{
    QFile file(elementPath);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "[element] failed to open" << elementPath;
        return false;
    }
    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement() || xml.name() != QLatin1String("element")) {
        qDebug() << "[element] root is not <element> for" << elementPath << "got" << xml.name();
        return false;
    }
    QList<QGraphicsItem*> items;
    if (!loadElementIntoScene(&xml, scene(), &items)) {
        qDebug() << "[element] loadElementIntoScene returned false for" << elementPath;
        return false;
    }
    DrawScene *ds = dynamic_cast<DrawScene*>(scene());
    if (!ds || items.isEmpty()) {
        qDebug() << "[element] no DrawScene or no items for" << elementPath;
        return false;
    }
    // For placed elements we want a visible group in the scene,
    // so we let DrawScene add the group item.
    GraphicsItemGroup *group = ds->createGroup(items, true);
    if (!group) {
        qDebug() << "[element] createGroup failed for" << elementPath;
        return false;
    }
    group->updateCoordinate();
    QPointF topLeft = group->sceneBoundingRect().topLeft();
    group->setPos(group->pos() + scenePos - topLeft);
    // 选中并将视图居中到新插入的组合图元，确保用户能看到
    group->setSelected(true);
    this->centerOn(group);
    qDebug() << "[element] placed group at" << group->pos()
             << "sceneBoundingRect" << group->sceneBoundingRect();
    setModified(true);
    return true;
}

static void collectShapesWithScenePos(QGraphicsItem *item, QList<QPair<AbstractShape*, QPointF> > *out)
{
    if (qgraphicsitem_cast<SizeHandleRect*>(item))
        return;
    GraphicsItemGroup *grp = qgraphicsitem_cast<GraphicsItemGroup*>(item);
    if (grp) {
        foreach (QGraphicsItem *child, grp->childItems()) {
            if (!qgraphicsitem_cast<SizeHandleRect*>(child))
                collectShapesWithScenePos(child, out);
        }
        return;
    }
    AbstractShape *ab = qgraphicsitem_cast<AbstractShape*>(item);
    if (ab)
        out->append(qMakePair(ab, item->scenePos()));
}

bool DrawView::saveSelectionAsElement(const QString &elementPath, const QString &displayName)
{
    QList<QGraphicsItem*> sel = scene()->selectedItems();
    if (sel.isEmpty())
        return false;
    QList<QPair<AbstractShape*, QPointF> > shapes;
    foreach (QGraphicsItem *item, sel)
        collectShapesWithScenePos(item, &shapes);
    if (shapes.isEmpty())
        return false;
    QRectF unionRect;
    for (int i = 0; i < shapes.size(); ++i)
        unionRect |= shapes.at(i).first->sceneBoundingRect();
    QPointF origin = unionRect.topLeft();

    QFile file(elementPath);
    if (!file.open(QFile::WriteOnly | QFile::Text))
        return false;
    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement(QLatin1String("element"));
    xml.writeAttribute(QLatin1String("name"), displayName.isEmpty() ? QFileInfo(elementPath).baseName() : displayName);
    for (int i = 0; i < shapes.size(); ++i) {
        AbstractShape *ab = shapes.at(i).first;
        QPointF relPos = shapes.at(i).second - origin;
        QGraphicsItem *dup = ab->duplicate();
        AbstractShape *dupShape = qgraphicsitem_cast<AbstractShape*>(dup);
        if (!dupShape) {
            delete dup;
            continue;
        }
        dupShape->setPos(relPos);
        dupShape->saveToXml(&xml);
        delete dup;
    }
    xml.writeEndElement();
    xml.writeEndDocument();
    return true;
}

QPixmap DrawView::renderElementPreview(const QString &elementPath, const QSize &size)
{
    QFile file(elementPath);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return QPixmap();
    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement() || xml.name() != QLatin1String("element"))
        return QPixmap();
    QGraphicsScene tempScene;
    QList<QGraphicsItem*> items;
    if (!loadElementIntoScene(&xml, &tempScene, &items) || items.isEmpty())
        return QPixmap();
    QRectF bounds = tempScene.itemsBoundingRect();
    if (!bounds.isValid())
        bounds = QRectF(0, 0, 48, 48);
    tempScene.setSceneRect(bounds.adjusted(-4, -4, 4, 4));
    QImage img(size.isEmpty() ? QSize(48, 48) : size, QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    tempScene.render(&p, QRectF(), tempScene.sceneRect());
    p.end();
    return QPixmap::fromImage(img);
}

