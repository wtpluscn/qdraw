#ifndef DRAWVIEW_H
#define DRAWVIEW_H
#include <QGraphicsView>

#include "rulebar.h"
#include "drawobj.h"
#include "graphicsitemgroup.h"

class QMouseEvent;
class QUndoStack;

class DrawView : public QGraphicsView
{
    Q_OBJECT
public:
    DrawView(QGraphicsScene *scene);
    QUndoStack *undoStack() const { return m_undoStack; }
    void zoomIn();
    void zoomOut();

    void newFile();
    bool loadFile(const QString &fileName);
    bool save();
    bool saveAs();
    bool saveFile(const QString &fileName);
    bool exportToPng(const QString &fileName);
    QString userFriendlyCurrentFile();

    bool loadElementAndInsertAt(const QString &elementPath, const QPointF &scenePos);
    bool saveSelectionAsElement(const QString &elementPath, const QString &displayName);
    QPixmap renderElementPreview(const QString &elementPath, const QSize &size);

    QString currentFile() { return curFile; }
    void setModified( bool value ) { modified = value ; }
    bool isModified() const { return modified; }
signals:
    void positionChanged(int x , int y );
protected:
    void closeEvent(QCloseEvent *event) Q_DECL_OVERRIDE;

    void mouseMoveEvent(QMouseEvent * event) Q_DECL_OVERRIDE;
    void resizeEvent(QResizeEvent *event) Q_DECL_OVERRIDE;
    void scrollContentsBy(int dx, int dy) Q_DECL_OVERRIDE;
    void updateRuler();
    QtRuleBar *m_hruler;
    QtRuleBar *m_vruler;
    QtCornerBox * box;

private:
    bool maybeSave();
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);
    void loadCanvas( QXmlStreamReader *xml );
    GraphicsItemGroup * loadGroupFromXML( QXmlStreamReader * xml );
    AbstractShape * createItemFromXmlName( QXmlStreamReader * xml );
    bool loadElementIntoScene(QXmlStreamReader *xml, QGraphicsScene *targetScene, QList<QGraphicsItem *> *outList);

    QString curFile;
    bool isUntitled;
    bool modified;
    class QUndoStack *m_undoStack;
};

#endif // DRAWVIEW_H
