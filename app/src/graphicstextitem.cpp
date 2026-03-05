#include "graphicstextitem.h"
#include <QPainter>
#include <QStyleOptionGraphicsItem>

GraphicsTextItem::GraphicsTextItem(const QRect & rect, QGraphicsItem * parent)
    : GraphicsRectItem(rect, false, parent)
{
    m_text = tr("Text");
    m_brush = QBrush(Qt::white);
    m_pen = QPen(Qt::black);
    m_font = QFont();
    if (m_font.pointSize() <= 0)
        m_font.setPointSize(12);
    m_textColor = Qt::black;
}

QRectF GraphicsTextItem::boundingRect() const
{
    return m_localRect;
}

QPainterPath GraphicsTextItem::shape() const
{
    QPainterPath path;
    path.addRect(m_localRect);
    return path;
}

QGraphicsItem *GraphicsTextItem::duplicate() const
{
    GraphicsTextItem *item = new GraphicsTextItem(m_localRect.toRect());
    item->m_width = width();
    item->m_height = height();
    item->m_text = m_text;
    item->m_font = m_font;
    item->m_textColor = m_textColor;
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

bool GraphicsTextItem::loadFromXml(QXmlStreamReader *xml)
{
    m_text = xml->attributes().value(QLatin1String("content")).toString();
    if (m_text.isEmpty())
        m_text = tr("Text");

    const auto fontFamily = xml->attributes().value(QLatin1String("fontFamily")).toString();
    if (!fontFamily.isEmpty())
        m_font.setFamily(fontFamily);

    bool ok = false;
    int ptSize = xml->attributes().value(QLatin1String("fontPointSize")).toInt(&ok);
    if (ok && ptSize > 0)
        m_font.setPointSize(ptSize);

    const auto boldAttr = xml->attributes().value(QLatin1String("bold")).toString();
    if (!boldAttr.isEmpty())
        m_font.setBold(boldAttr == QLatin1String("1"));

    const auto italicAttr = xml->attributes().value(QLatin1String("italic")).toString();
    if (!italicAttr.isEmpty())
        m_font.setItalic(italicAttr == QLatin1String("1"));

    const auto colorAttr = xml->attributes().value(QLatin1String("color")).toString();
    if (!colorAttr.isEmpty()) {
        QColor c(colorAttr);
        if (c.isValid())
            m_textColor = c;
    }

    readBaseAttributes(xml);
    xml->skipCurrentElement();
    updateCoordinate();
    return true;
}

bool GraphicsTextItem::saveToXml(QXmlStreamWriter *xml)
{
    xml->writeStartElement(QLatin1String("text"));
    xml->writeAttribute(QLatin1String("content"), m_text);
    xml->writeAttribute(QLatin1String("fontFamily"), m_font.family());
    if (m_font.pointSize() > 0)
        xml->writeAttribute(QLatin1String("fontPointSize"), QString::number(m_font.pointSize()));
    xml->writeAttribute(QLatin1String("bold"), m_font.bold() ? QLatin1String("1") : QLatin1String("0"));
    xml->writeAttribute(QLatin1String("italic"), m_font.italic() ? QLatin1String("1") : QLatin1String("0"));
    xml->writeAttribute(QLatin1String("color"), m_textColor.name(QColor::HexArgb));

    writeBaseAttributes(xml);
    xml->writeEndElement();
    return true;
}

void GraphicsTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    painter->setPen(pen());
    painter->setBrush(brush());
    painter->drawRect(m_localRect);

    painter->setPen(m_textColor);
    painter->setFont(m_font);
    // View uses scale(1,-1) (origin at bottom-left), so flip text in item coords so it appears right-side up
    painter->save();
    painter->translate(m_localRect.center());
    painter->scale(1, -1);
    painter->translate(-m_localRect.center());
    painter->drawText(m_localRect, Qt::TextWordWrap | Qt::AlignCenter, m_text);
    painter->restore();

    if (option->state & QStyle::State_Selected)
        qt_graphicsItem_highlightSelected(this, painter, option);
}

