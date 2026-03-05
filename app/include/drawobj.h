#ifndef DRAWOBJ
#define DRAWOBJ

#include <qgraphicsitem.h>
#include <QtCore/QObject>
#include <QGraphicsSceneMouseEvent>
#include "sizehandle.h"
#include <QVector>
#include <QGraphicsScene>
#include <QList>
#include <QCursor>
#include <vector>
#include <QMimeData>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QRandomGenerator>

class QPainter;
class QStyleOptionGraphicsItem;

class ShapeMimeData : public QMimeData
{
    Q_OBJECT
public:
    ShapeMimeData( QList<QGraphicsItem * > items);
    ~ShapeMimeData();
    QList<QGraphicsItem *> items() const ;
private:
    QList<QGraphicsItem * > m_items;
};

template < typename BaseType = QGraphicsItem >
class AbstractShapeType : public BaseType
{
public:
   explicit AbstractShapeType(QGraphicsItem * parent = 0 )
        :BaseType(parent)
    {
        m_pen=QPen(Qt::NoPen);
        m_brush= QBrush(QColor(QRandomGenerator::global()->bounded(256),
                              QRandomGenerator::global()->bounded(256),
                              QRandomGenerator::global()->bounded(256)));
        m_width = m_height = 0;
    }
    virtual ~AbstractShapeType(){}
    virtual QString displayName () const { return QString("AbstractType");}
    virtual void control(int dir, const QPointF & delta ){ Q_UNUSED(dir);Q_UNUSED(delta);}
    virtual void stretch( int  , double  , double  , const QPointF & ) {}
    virtual QRectF  rect() const { return m_localRect; }
    virtual void updateCoordinate () {}
    virtual void move( const QPointF & point ){Q_UNUSED(point);}
    virtual QGraphicsItem * duplicate() const { return NULL;}
    virtual int handleCount() const { return static_cast<int>(m_handles.size());}
    virtual bool loadFromXml(QXmlStreamReader * xml ) = 0;
    virtual bool saveToXml( QXmlStreamWriter * xml ) = 0 ;
    int collidesWithHandle( const QPointF & point ) const
    {
        const Handles::const_reverse_iterator hend =  m_handles.rend();
        for (Handles::const_reverse_iterator it = m_handles.rbegin(); it != hend; ++it)
        {
            QPointF pt = (*it)->mapFromScene(point);
            if ((*it)->contains(pt) ){
                return (*it)->dir();
            }
        }
        return Handle_None;
    }
    virtual QPointF handlePos( int handle ) const
    {
        const Handles::const_reverse_iterator hend =  m_handles.rend();
        for (Handles::const_reverse_iterator it = m_handles.rbegin(); it != hend; ++it)
        {
            if ((*it)->dir() == handle ){
                return (*it)->pos();
            }
        }
        return QPointF();
    }
    int swapHandle(int handle, const QPointF& scale ) const
    {
        int dir = Handle_None;
        if ( scale.x() < 0 || scale.y() < 0 ){
            switch (handle) {
            case RightTop:
                if ( scale.x() < 0 && scale.y() < 0 )
                    dir = LeftBottom;
                else if ( scale.x() > 0 && scale.y() < 0 )
                    dir = RightBottom;
                else
                    dir = LeftTop;
                break;
            case RightBottom:
                if ( scale.x() < 0 && scale.y() < 0 )
                    dir = LeftTop;
                else if ( scale.x() > 0 && scale.y() < 0 )
                    dir = RightTop;
                else
                    dir = LeftBottom;
                break;
            case LeftBottom:
                if ( scale.x() < 0 && scale.y() < 0 )
                    dir = RightTop;
                else if ( scale.x() > 0 && scale.y() < 0 )
                    dir = LeftTop;
                else
                    dir = RightBottom;
                break;
            case LeftTop:
                if ( scale.x() < 0 && scale.y() < 0 )
                    dir = RightBottom;
                else if ( scale.x() > 0 && scale.y() < 0 )
                    dir = LeftBottom;
                else
                    dir = RightTop;
                break;
            case Right:
                if (scale.x() < 0 )
                    dir = Left;
                break;
            case Left:
                if (scale.x() < 0 )
                    dir = Right;
                break;
            case Top:
                if (scale.y()<0)
                    dir = Bottom;
                break;
            case Bottom:
                if (scale.y()<0)
                   dir = Top;
                break;
            }
        }
        return dir;
    }
    virtual QPointF opposite( int handle ) {
        QPointF pt;
        switch (handle) {
        case Right:
            pt = m_handles.at(Left-1)->pos();
            break;
        case RightTop:
            pt = m_handles[LeftBottom-1]->pos();
            break;
        case RightBottom:
            pt = m_handles[LeftTop-1]->pos();
            break;
        case LeftBottom:
            pt = m_handles[RightTop-1]->pos();
            break;
        case Bottom:
            pt = m_handles[Top-1]->pos();
            break;
        case LeftTop:
            pt = m_handles[RightBottom-1]->pos();
            break;
        case Left:
            pt = m_handles[Right-1]->pos();
            break;
        case Top:
            pt = m_handles[Bottom-1]->pos();
            break;
         }
        return pt;
    }

    QColor brushColor() const {return m_brush.color();}
    QBrush brush() const {return m_brush;}
    QPen   pen() const {return m_pen;}
    QColor penColor() const {return m_pen.color();}
    void   setPen(const QPen & pen ) { m_pen = pen;}
    void   setBrush( const QBrush & brush ) { m_brush = brush ; }
    void   setBrushColor( const QColor & color ) { m_brush.setColor(color);}
    qreal  width() const { return m_width ; }
    void   setWidth( qreal width )
    {
        m_width = width ;
        updateCoordinate();
    }
    qreal  height() const {return m_height;}
    void   setHeight ( qreal height )
    {
        m_height = height ;
        updateCoordinate();
    }

protected:
    virtual void updatehandles(){}
    void setState(SelectionHandleState st)
    {
        const Handles::iterator hend =  m_handles.end();
        for (Handles::iterator it = m_handles.begin(); it != hend; ++it)
            (*it)->setState(st);
    }
    QBrush m_brush;
    QPen   m_pen ;
    typedef std::vector<SizeHandleRect*> Handles;
    Handles m_handles;
    QRectF m_localRect;
    qreal m_width;
    qreal m_height;
};

typedef  AbstractShapeType< QGraphicsItem > AbstractShape;

class GraphicsItem : public QObject,
        public AbstractShapeType<QGraphicsItem>
{
    Q_OBJECT
    Q_PROPERTY(QColor pen READ penColor WRITE setPen )
    Q_PROPERTY(QColor brush READ brushColor WRITE setBrushColor )
    Q_PROPERTY(qreal  width READ width WRITE setWidth )
    Q_PROPERTY(qreal  height READ height WRITE setHeight )
    Q_PROPERTY(QPointF  position READ pos WRITE setPos )

public:
    GraphicsItem(QGraphicsItem * parent );
    enum {Type = UserType+1};
    int  type() const { return Type; }
    virtual QPixmap image() ;
signals:
    void selectedChange(QGraphicsItem *item);

protected:
    QVariant itemChange(QGraphicsItem::GraphicsItemChange change, const QVariant &value);
    void updatehandles();
    void contextMenuEvent(QGraphicsSceneContextMenuEvent *event);

    bool readBaseAttributes(QXmlStreamReader * xml );
    bool writeBaseAttributes( QXmlStreamWriter * xml );

};

QPainterPath qt_graphicsItem_shapeFromPath(const QPainterPath &path, const QPen &pen);
void qt_graphicsItem_highlightSelected(QGraphicsItem *item, QPainter *painter, const QStyleOptionGraphicsItem *option);

#endif // DRAWOBJ
