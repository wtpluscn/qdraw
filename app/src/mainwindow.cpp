#include "mainwindow.h"
#include "drawview.h"
#include "drawscene.h"
#include <QtWidgets>
#include <QDir>
#include <QInputDialog>
#include "qtvariantproperty.h"
#include "qttreepropertybrowser.h"
#include <QDockWidget>
#include <QGraphicsItem>
#include <QMdiSubWindow>
#include <QUndoStack>
#include "customproperty.h"
#include "drawshapes.h"
#include "commands.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    undoView = new QUndoView(this);
    undoView->setWindowTitle(tr("Command List"));
    undoView->setAttribute(Qt::WA_QuitOnClose, false);

    QDockWidget *dock = new QDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, dock);

    dock->setWidget(undoView);

    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);

    setWindowTitle(tr("Qt Drawing"));
    setUnifiedTitleAndToolBarOnMac(true);

    connect(mdiArea, SIGNAL(subWindowActivated(QMdiSubWindow*)),
            this, SLOT(updateMenus()));
    windowMapper = new QSignalMapper(this);
    connect(windowMapper, SIGNAL(mapped(QWidget*)),
            this, SLOT(setActiveSubWindow(QWidget*)));

    createActions();
    createMenus();
    createToolbars();
    createToolBox();
    createPropertyEditor();

    newFile();
    mdiArea->tileSubWindows();
 /*
    m_posInfo = new QLabel(tr("x,y"));
    m_posInfo->setMinimumSize(m_posInfo->sizeHint());
    m_posInfo->setAlignment(Qt::AlignHCenter);
    statusBar()->addWidget(m_posInfo);
*/
    connect(QApplication::clipboard(),SIGNAL(dataChanged()),this,SLOT(dataChanged()));
    connect(&m_timer,SIGNAL(timeout()),this,SLOT(updateActions()));
    m_timer.start(500);
    theControlledObject = NULL;

}

MainWindow::~MainWindow()
{

}

void MainWindow::createToolBox()
{
    QDockWidget *dock = new QDockWidget(this);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    toolBox = new QToolBox(dock);
    toolBox->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Ignored));

    listView = new QListWidget();
    listView->setViewMode(QListView::IconMode);
    listView->setDragDropMode(QAbstractItemView::NoDragDrop);
    listView->setStyleSheet(tr("QListView {background-color: #e0f0e0;}"));
    GraphicsRectItem item(QRect(0,0,48,48));
    QIcon icon(item.image());
    GraphicsRectItem roundRectItem(QRect(0,0,48,48), true);
    QIcon iconRoundRect(roundRectItem.image());
    GraphicsEllipseItem item1(QRect(0,0,48,48));
    QIcon icon1(item1.image());
    listView->addItem(new QListWidgetItem(icon, tr("Rectangle")));
    listView->addItem(new QListWidgetItem(iconRoundRect, tr("RoundRect")));
    listView->addItem(new QListWidgetItem(icon1, tr("Ellipse")));
    toolBox->addItem(listView, tr("Basic shapes"));

    listViewElements = new QListWidget();
    listViewElements->setViewMode(QListView::IconMode);
    listViewElements->setDragDropMode(QAbstractItemView::NoDragDrop);
    listViewElements->setStyleSheet(tr("QListView {background-color: #e0e8f0;}"));
    listViewElements->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listViewElements, &QListWidget::customContextMenuRequested,
            this, &MainWindow::onCompoundListContextMenu);
    toolBox->addItem(listViewElements, tr("Compound elements"));

    connect(listView, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(onGraphicsLibraryItemClicked(QListWidgetItem*)));
    connect(listViewElements, SIGNAL(itemClicked(QListWidgetItem*)),
            this, SLOT(onGraphicsLibraryItemClicked(QListWidgetItem*)));

    dock->setWidget(toolBox);
}

QString MainWindow::elementLibraryPath() const
{
    return QCoreApplication::applicationDirPath() + QLatin1String("/element");
}

void MainWindow::loadElementLibrary()
{
    listViewElements->clear();
    QDir elementDir(elementLibraryPath());
    if (!elementDir.exists())
        return;
    QStringList filters;
    filters << QLatin1String("*.xml");
    const QSize iconSize(48, 48);
    DrawView *view = activeMdiChild();
    foreach (const QString &name, elementDir.entryList(filters, QDir::Files, QDir::Name)) {
        QString path = elementDir.absoluteFilePath(name);
        QPixmap icon;
        if (view) {
            qDebug() << "[element] preview via active view for" << path;
            icon = view->renderElementPreview(path, iconSize);
        } else {
            // no active view yet, use a temporary one just for preview
            qDebug() << "[element] preview via temp view for" << path;
            QGraphicsScene dummyScene;
            DrawView tempView(&dummyScene);
            icon = tempView.renderElementPreview(path, iconSize);
        }
        if (icon.isNull())
            icon = QPixmap(iconSize);
        QListWidgetItem *wi = new QListWidgetItem(icon, QFileInfo(name).baseName());
        wi->setData(Qt::UserRole, path);
        listViewElements->addItem(wi);
    }
}

void MainWindow::onGraphicsLibraryItemClicked(QListWidgetItem *item)
{
    if (!item) return;
    QListWidget *from = qobject_cast<QListWidget*>(sender());
    if (from == listView) {
        int row = listView->row(item);
        if (row == 0) {
            DrawTool::c_drawShape = rectangle;
            rectAct->setChecked(true);
        } else if (row == 1) {
            DrawTool::c_drawShape = roundrect;
            roundRectAct->setChecked(true);
        } else if (row == 2) {
            DrawTool::c_drawShape = ellipse;
            ellipseAct->setChecked(true);
        }
        m_editingElementPath.clear();
        if (activeMdiChild()->scene())
            activeMdiChild()->scene()->clearSelection();
        statusBar()->showMessage(tr("Drag on the canvas to draw"));
        return;
    }
    if (from == listViewElements) {
        QString path = item->data(Qt::UserRole).toString();
        if (!path.isEmpty()) {
            qDebug() << "[element] clicked" << path;
            DrawView *view = activeMdiChild();
            if (!view) {
                qDebug() << "[element] no active view, creating new drawing window for pending placement";
                view = createMdiChild();
            }
            DrawScene *sc = view ? qobject_cast<DrawScene*>(view->scene()) : 0;
            if (sc) {
                sc->setPendingElementPath(path);
                DrawTool::c_drawShape = selection;
                selectAct->setChecked(true);
                // 让当前视图获得焦点，并用十字光标提示“待放置”
                foreach (QMdiSubWindow *w, mdiArea->subWindowList()) {
                    if (w->widget() == view) {
                        mdiArea->setActiveSubWindow(w);
                        break;
                    }
                }
                view->setFocus(Qt::MouseFocusReason);
                view->setCursor(Qt::CrossCursor);
                statusBar()->showMessage(tr("[Element] Click on the canvas to place the element"));
            } else {
                qDebug() << "[element] no scene for pending placement";
                statusBar()->showMessage(tr("No drawing window to place element into"));
            }
        }
    }
}

DrawView *MainWindow::activeMdiChild()
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<DrawView *>(activeSubWindow->widget());
    return 0;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

    foreach (QMdiSubWindow *window, mdiArea->subWindowList()) {
        DrawView *mdiChild = qobject_cast<DrawView *>(window->widget());
        if (mdiChild && mdiChild->currentFile() == canonicalFilePath)
            return window;
    }
    return 0;
}

void MainWindow::createActions()
{

    newAct = new QAction(tr("&New"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("Create a new file"));
    connect(newAct, SIGNAL(triggered()), this, SLOT(newFile()));

    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    exportImageAct = new QAction(tr("Export as &image..."), this);
    exportImageAct->setStatusTip(tr("Export current drawing as PNG image"));
    connect(exportImageAct, SIGNAL(triggered()), this, SLOT(exportToImage()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    closeAct = new QAction(tr("Cl&ose"), this);
    closeAct->setStatusTip(tr("Close the active window"));
    connect(closeAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeActiveSubWindow()));

    closeAllAct = new QAction(tr("Close &All"), this);
    closeAllAct->setStatusTip(tr("Close all the windows"));
    connect(closeAllAct, SIGNAL(triggered()),
            mdiArea, SLOT(closeAllSubWindows()));

    tileAct = new QAction(tr("&Tile"), this);
    tileAct->setStatusTip(tr("Tile the windows"));
    connect(tileAct, SIGNAL(triggered()), mdiArea, SLOT(tileSubWindows()));

    cascadeAct = new QAction(tr("&Cascade"), this);
    cascadeAct->setStatusTip(tr("Cascade the windows"));
    connect(cascadeAct, SIGNAL(triggered()), mdiArea, SLOT(cascadeSubWindows()));

    nextAct = new QAction(tr("Ne&xt"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("Move the focus to the next window"));
    connect(nextAct, SIGNAL(triggered()),
            mdiArea, SLOT(activateNextSubWindow()));

    previousAct = new QAction(tr("Pre&vious"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("Move the focus to the previous "
                                 "window"));
    connect(previousAct, SIGNAL(triggered()),
            mdiArea, SLOT(activatePreviousSubWindow()));

    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));

    // create align actions
    rightAct   = new QAction(QIcon(":/icons/align_right.png"),tr("right"),this);
    leftAct    = new QAction(QIcon(":/icons/align_left.png"),tr("left"),this);
    vCenterAct = new QAction(QIcon(":/icons/align_vcenter.png"),tr("vcenter"),this);
    hCenterAct = new QAction(QIcon(":/icons/align_hcenter.png"),tr("hcenter"),this);
    upAct      = new QAction(QIcon(":/icons/align_top.png"),tr("top"),this);
    downAct    = new QAction(QIcon(":/icons/align_bottom.png"),tr("bottom"),this);
    horzAct    = new QAction(QIcon(":/icons/align_horzeven.png"),tr("Horizontal"),this);
    vertAct    = new QAction(QIcon(":/icons/align_verteven.png"),tr("vertical"),this);
    heightAct  = new QAction(QIcon(":/icons/align_height.png"),tr("height"),this);
    widthAct   = new QAction(QIcon(":/icons/align_width.png"),tr("width"),this);
    allAct     = new QAction(QIcon(":/icons/align_all.png"),tr("width and height"),this);

    bringToFrontAct = new QAction(QIcon(":/icons/bringtofront.png"),tr("bring to front"),this);
    sendToBackAct   = new QAction(QIcon(":/icons/sendtoback.png"),tr("send to back"),this);
    groupAct        = new QAction(QIcon(":/icons/group.png"),tr("group"),this);
    unGroupAct        = new QAction(QIcon(":/icons/ungroup.png"),tr("ungroup"),this);

    connect(bringToFrontAct,SIGNAL(triggered()),this,SLOT(on_actionBringToFront_triggered()));
    connect(sendToBackAct,SIGNAL(triggered()),this,SLOT(on_actionSendToBack_triggered()));
    connect(rightAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(leftAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(vCenterAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(hCenterAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(upAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(downAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));

    connect(horzAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(vertAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(heightAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(widthAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));
    connect(allAct,SIGNAL(triggered()),this,SLOT(on_aglin_triggered()));

    connect(groupAct,SIGNAL(triggered()),this,SLOT(on_group_triggered()));
    connect(unGroupAct,SIGNAL(triggered()),this,SLOT(on_unGroup_triggered()));

    saveAsElementAct = new QAction(tr("Save as element..."), this);
    saveAsElementAct->setStatusTip(tr("Save selected items as a compound element to the library"));
    connect(saveAsElementAct, SIGNAL(triggered()), this, SLOT(saveSelectionAsElementTriggered()));

    //create draw actions
    selectAct = new QAction(QIcon(":/icons/arrow.png"),tr("select tool"),this);

    selectAct->setCheckable(true);
    lineAct = new QAction(QIcon(":/icons/line.png"),tr("line tool"),this);
    lineAct->setCheckable(true);
    rectAct = new QAction(QIcon(":/icons/rectangle.png"),tr("rect tool"),this);
    rectAct->setCheckable(true);

    roundRectAct =  new QAction(QIcon(":/icons/roundrect.png"),tr("roundrect tool"),this);
    roundRectAct->setCheckable(true);
    ellipseAct = new QAction(QIcon(":/icons/ellipse.png"),tr("ellipse tool"),this);
    ellipseAct->setCheckable(true);
    arcAct = new QAction(QIcon(":/icons/arc.png"),tr("arc tool"),this);
    arcAct->setCheckable(true);
    textAct = new QAction(tr("Text"),this);
    textAct->setCheckable(true);
    polygonAct = new QAction(QIcon(":/icons/polygon.png"),tr("polygon tool"),this);
    polygonAct->setCheckable(true);
    polylineAct = new QAction(QIcon(":/icons/polyline.png"),tr("polyline tool"),this);
    polylineAct->setCheckable(true);
    bezierAct= new QAction(QIcon(":/icons/bezier.png"),tr("bezier tool"),this);
    bezierAct->setCheckable(true);

    rotateAct = new QAction(QIcon(":/icons/rotate.png"),tr("rotate tool"),this);
    rotateAct->setCheckable(true);

    drawActionGroup = new QActionGroup(this);
    drawActionGroup->addAction(selectAct);
    drawActionGroup->addAction(lineAct);
    drawActionGroup->addAction(rectAct);
    drawActionGroup->addAction(roundRectAct);
    drawActionGroup->addAction(ellipseAct);
    drawActionGroup->addAction(arcAct);
    drawActionGroup->addAction(textAct);
    drawActionGroup->addAction(polygonAct);
    drawActionGroup->addAction(polylineAct);
    drawActionGroup->addAction(bezierAct);
    drawActionGroup->addAction(rotateAct);
    selectAct->setChecked(true);


    connect(selectAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(lineAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(rectAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(roundRectAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(ellipseAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(arcAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(textAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(polygonAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(polylineAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(bezierAct,SIGNAL(triggered()),this,SLOT(addShape()));
    connect(rotateAct,SIGNAL(triggered()),this,SLOT(addShape()));

    deleteAct = new QAction(tr("&Delete"), this);
    deleteAct->setShortcut(QKeySequence::Delete);

    undoAct = new QAction(tr("Undo"), this);
    undoAct->setIcon(QIcon(":/icons/undo.png"));
    undoAct->setShortcuts(QKeySequence::Undo);
    connect(undoAct, SIGNAL(triggered()), this, SLOT(onUndo()));

    redoAct = new QAction(tr("Redo"), this);
    redoAct->setIcon(QIcon(":/icons/redo.png"));
    redoAct->setShortcuts(QKeySequence::Redo);
    connect(redoAct, SIGNAL(triggered()), this, SLOT(onRedo()));

    zoomInAct = new QAction(QIcon(":/icons/zoomin.png"),tr("zoomIn"),this);
    zoomOutAct = new QAction(QIcon(":/icons/zoomout.png"),tr("zoomOut"),this);

    copyAct = new QAction(QIcon(":/icons/copy.png"),tr("copy"),this);
    copyAct->setShortcut(QKeySequence::Copy);

    pasteAct = new QAction(QIcon(":/icons/paste.png"),tr("paste"),this);
    pasteAct->setShortcut(QKeySequence::Paste);
    pasteAct->setEnabled(false);
    cutAct = new QAction(QIcon(":/icons/cut.png"),tr("cut"),this);
    cutAct->setShortcut(QKeySequence::Cut);

    connect(copyAct,SIGNAL(triggered()),this,SLOT(on_copy()));
    connect(pasteAct,SIGNAL(triggered()),this,SLOT(on_paste()));
    connect(cutAct,SIGNAL(triggered()),this,SLOT(on_cut()));

    connect(zoomInAct , SIGNAL(triggered()),this,SLOT(zoomIn()));
    connect(zoomOutAct , SIGNAL(triggered()),this,SLOT(zoomOut()));
    connect(deleteAct, SIGNAL(triggered()), this, SLOT(deleteItem()));

    funcAct = new QAction(tr("func test"),this);
    connect(funcAct,SIGNAL(triggered()),this,SLOT(on_func_test_triggered()));

}

void MainWindow::createMenus()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(exportImageAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));
    editMenu->addAction(undoAct);
    editMenu->addAction(redoAct);
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    editMenu->addAction(deleteAct);
    editMenu->addAction(saveAsElementAct);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(zoomInAct);
    viewMenu->addAction(zoomOutAct);

    QMenu *toolMenu = menuBar()->addMenu(tr("&Tools"));
    QMenu *shapeTool = new QMenu("&Shape");
    shapeTool->addAction(selectAct);
    shapeTool->addAction(rectAct);
    shapeTool->addAction(roundRectAct);
    shapeTool->addAction(ellipseAct);
    shapeTool->addAction(arcAct);
    shapeTool->addAction(polygonAct);
    shapeTool->addAction(polylineAct);
    shapeTool->addAction(bezierAct);
    shapeTool->addAction(rotateAct);
    toolMenu->addMenu(shapeTool);
    QMenu *alignMenu = new QMenu("Align");
    alignMenu->addAction(rightAct);
    alignMenu->addAction(leftAct);
    alignMenu->addAction(hCenterAct);
    alignMenu->addAction(vCenterAct);
    alignMenu->addAction(upAct);
    alignMenu->addAction(downAct);
    alignMenu->addAction(horzAct);
    alignMenu->addAction(vertAct);
    alignMenu->addAction(heightAct);
    alignMenu->addAction(widthAct);
    alignMenu->addAction(allAct);
    toolMenu->addMenu(alignMenu);

    windowMenu = menuBar()->addMenu(tr("&Window"));
    updateWindowMenu();
    connect(windowMenu, SIGNAL(aboutToShow()), this, SLOT(updateWindowMenu()));

    menuBar()->addSeparator();

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
    helpMenu->addAction(funcAct);

}

void MainWindow::createToolbars()
{
    // create edit toolbar
    editToolBar = addToolBar(tr("edit"));
    editToolBar->setIconSize(QSize(24,24));
    editToolBar->addAction(copyAct);
    editToolBar->addAction(pasteAct);
    editToolBar->addAction(cutAct);

    editToolBar->addAction(undoAct);
    editToolBar->addAction(redoAct);

    editToolBar->addAction(zoomInAct);
    editToolBar->addAction(zoomOutAct);

    // create draw toolbar
    drawToolBar = addToolBar(tr("drawing"));
    drawToolBar->setIconSize(QSize(24,24));
    drawToolBar->addAction(selectAct);
    drawToolBar->addAction(lineAct);
    drawToolBar->addAction(rectAct);
    drawToolBar->addAction(roundRectAct);
    drawToolBar->addAction(ellipseAct);
    drawToolBar->addAction(arcAct);
    drawToolBar->addAction(textAct);
    drawToolBar->addAction(polygonAct);
    drawToolBar->addAction(polylineAct);
    drawToolBar->addAction(bezierAct);
    drawToolBar->addAction(rotateAct);

    // create align toolbar
    alignToolBar = addToolBar(tr("align"));
    alignToolBar->setIconSize(QSize(24,24));
    alignToolBar->addAction(upAct);
    alignToolBar->addAction(downAct);
    alignToolBar->addAction(rightAct);
    alignToolBar->addAction(leftAct);
    alignToolBar->addAction(vCenterAct);
    alignToolBar->addAction(hCenterAct);

    alignToolBar->addAction(horzAct);
    alignToolBar->addAction(vertAct);
    alignToolBar->addAction(heightAct);
    alignToolBar->addAction(widthAct);
    alignToolBar->addAction(allAct);

    alignToolBar->addAction(bringToFrontAct);
    alignToolBar->addAction(sendToBackAct);
    alignToolBar->addAction(groupAct);
    alignToolBar->addAction(unGroupAct);
}

void MainWindow::createPropertyEditor()
{
    dockProperty = new QDockWidget(this);
    addDockWidget(Qt::RightDockWidgetArea, dockProperty);

    propertyEditor = new ObjectController(this);
    dockProperty->setWidget(propertyEditor);
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeMdiChild() != 0);
    saveAct->setEnabled(hasMdiChild);
    exportImageAct->setEnabled(hasMdiChild);
    undoView->setStack(hasMdiChild ? activeMdiChild()->undoStack() : 0);
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    separatorAct->setVisible(hasMdiChild);

    bool hasSelection = (activeMdiChild() &&
                         activeMdiChild()->scene()->selectedItems().count()>0);
    DrawView *activeView = activeMdiChild();
    bool isElementWindow = activeView && activeView->windowTitle().contains(QLatin1String("[Element]"));
    int shapeCount = 0;
    if (activeView && activeView->scene()) {
        foreach (QGraphicsItem *it, activeView->scene()->items()) {
            if (qgraphicsitem_cast<AbstractShape*>(it) && !qgraphicsitem_cast<SizeHandleRect*>(it))
                ++shapeCount;
        }
    }
    bool canSaveAsElement = hasSelection || (isElementWindow && shapeCount > 0);

    cutAct->setEnabled(hasSelection);
    copyAct->setEnabled(hasSelection);
    saveAsElementAct->setEnabled(canSaveAsElement);
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);

    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());

    for (int i = 0; i < windows.size(); ++i) {
        DrawView *child = qobject_cast<DrawView *>(windows.at(i)->widget());
        if (!child) continue;

        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1)
                               .arg(child->userFriendlyCurrentFile());
        } else {
            text = tr("%1 %2").arg(i + 1)
                              .arg(child->userFriendlyCurrentFile());
        }
        QAction *action  = windowMenu->addAction(text);
        action->setCheckable(true);
        action ->setChecked(child == activeMdiChild());
        connect(action, SIGNAL(triggered()), windowMapper, SLOT(map()));
        windowMapper->setMapping(action, windows.at(i));
    }
}

void MainWindow::newFile()
{
    DrawView *child = createMdiChild();
    child->newFile();
    child->show();
}

void MainWindow::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open"),
                                                    QString(),
                                                    tr("XML files (*.xml);;All files (*)"));
    if (!fileName.isEmpty()) {
        QMdiSubWindow *existing = findMdiChild(fileName);
        if (existing) {
            mdiArea->setActiveSubWindow(existing);
            return;
        }

        if (openFile(fileName))
            statusBar()->showMessage(tr("File loaded"), 2000);
    }
}

bool MainWindow::openFile(const QString &fileName)
{
    DrawView *child = createMdiChild();
    const bool succeeded = child->loadFile(fileName);
    if (succeeded)
        child->show();
    else
        child->close();
    return succeeded;
}

void MainWindow::save()
{
    if (activeMdiChild() && activeMdiChild()->save())
        statusBar()->showMessage(tr("File saved"), 2000);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        event->accept();
    }
}

DrawView *MainWindow::createMdiChild()
{

    DrawScene *scene = new DrawScene(this);

    QRectF rc = QRectF(0 , 0 , 800, 600);

    scene->setSceneRect(rc);
    qDebug()<<rc.bottomLeft()<<rc.size() << rc.topLeft();

    connect(scene, SIGNAL(selectionChanged()),
            this, SLOT(itemSelected()));

    connect(scene,SIGNAL(itemAdded(QGraphicsItem*)),
            this, SLOT(itemAdded(QGraphicsItem*)));
    connect(scene,SIGNAL(itemMoved(QGraphicsItem*,QPointF)),
            this,SLOT(itemMoved(QGraphicsItem*,QPointF)));
    connect(scene,SIGNAL(itemRotate(QGraphicsItem*,qreal)),
            this,SLOT(itemRotate(QGraphicsItem*,qreal)));

    connect(scene,SIGNAL(itemResize(QGraphicsItem* , int , const QPointF&)),
            this,SLOT(itemResize(QGraphicsItem*,int,QPointF)));

    connect(scene,SIGNAL(itemControl(QGraphicsItem* , int , const QPointF&,const QPointF&)),
            this,SLOT(itemControl(QGraphicsItem*,int,QPointF,QPointF)));
    connect(scene, SIGNAL(sceneClickedForElement(QString,QPointF)),
            this, SLOT(onSceneClickedForElement(QString,QPointF)));

    DrawView *view = new DrawView(scene);
    scene->setView(view);
    connect(view,SIGNAL(positionChanged(int,int)),this,SLOT(positionChanged(int,int)));

    view->setRenderHint(QPainter::Antialiasing);
    view->setCacheMode(QGraphicsView::CacheBackground);
    view->setOptimizationFlags(QGraphicsView::DontSavePainterState);
    view->setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    //view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // move orign point to leftbottom
    view->setTransform(view->transform().scale(1,-1));


    scene->setBackgroundBrush(Qt::darkGray);

    mdiArea->addSubWindow(view);

    if (listViewElements->count() == 0)
        loadElementLibrary();

    view->showMaximized();
    return view;
}

void MainWindow::onSceneClickedForElement(const QString &path, const QPointF &scenePos)
{
    DrawScene *sc = qobject_cast<DrawScene*>(sender());
    if (!sc) return;
    DrawView *view = qobject_cast<DrawView*>(sc->view());
    if (!view) return;
    if (view->loadElementAndInsertAt(path, scenePos)) {
        view->unsetCursor(); // 恢复正常光标
        statusBar()->showMessage(tr("[Element] Placed"), 2000);
    } else {
        statusBar()->showMessage(tr("Failed to load element: %1").arg(path));
    }
}

void MainWindow::onCompoundListContextMenu(const QPoint &pos)
{
    QListWidgetItem *item = listViewElements->itemAt(pos);
    if (item)
        listViewElements->setCurrentItem(item);
    QMenu menu(this);
    menu.addAction(tr("New"), this, &MainWindow::onElementNew);
    if (item) {
        menu.addAction(tr("Edit"), this, &MainWindow::onElementEdit);
        menu.addAction(tr("Delete"), this, &MainWindow::onElementDelete);
    }
    menu.exec(listViewElements->mapToGlobal(pos));
}

void MainWindow::onElementNew()
{
    m_editingElementPath.clear();
    DrawView *view = createMdiChild();
    view->setWindowTitle(tr("[Element] New"));
    QMdiSubWindow *sub = 0;
    foreach (QMdiSubWindow *w, mdiArea->subWindowList()) {
        if (w->widget() == view) {
            sub = w;
            break;
        }
    }
    if (sub) {
        mdiArea->setActiveSubWindow(sub);
        view->setFocus(Qt::MouseFocusReason);
    }
    statusBar()->showMessage(tr("[Element] New canvas: Draw or select shapes, then use Edit -> Save as element to add to library"));
}

void MainWindow::onElementEdit()
{
    QListWidgetItem *item = listViewElements->currentItem();
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    if (path.isEmpty()) return;
    DrawView *view = createMdiChild();
    QString name = QFileInfo(path).baseName();
    view->setWindowTitle(tr("[Element] Editing: %1").arg(name));
    QPointF placePos(100, 100);
    if (view->scene() && view->scene()->sceneRect().isValid())
        placePos = view->scene()->sceneRect().center();
    if (view->loadElementAndInsertAt(path, placePos)) {
        m_editingElementPath = path;
        QMdiSubWindow *sub = 0;
        foreach (QMdiSubWindow *w, mdiArea->subWindowList()) {
            if (w->widget() == view) {
                sub = w;
                break;
            }
        }
        if (sub) {
            mdiArea->setActiveSubWindow(sub);
            view->setFocus(Qt::MouseFocusReason);
        }
        statusBar()->showMessage(tr("[Element] Edit canvas: Modify then use Edit -> Save as element (same name to overwrite)."));
    } else {
        statusBar()->showMessage(tr("Failed to load element for editing"));
        view->close();
    }
}

void MainWindow::onElementDelete()
{
    QListWidgetItem *item = listViewElements->currentItem();
    if (!item) return;
    QString path = item->data(Qt::UserRole).toString();
    QString name = item->text();
    if (path.isEmpty()) return;
    int ret = QMessageBox::question(this, tr("Delete element"),
                                    tr("Delete element \"%1\"? The file will be removed.").arg(name),
                                    QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret != QMessageBox::Yes) return;
    if (QFile::remove(path)) {
        delete listViewElements->takeItem(listViewElements->row(item));
        if (m_editingElementPath == path)
            m_editingElementPath.clear();
        statusBar()->showMessage(tr("Element deleted"), 2000);
    } else {
        statusBar()->showMessage(tr("Failed to delete file: %1").arg(path));
    }
}

void MainWindow::saveSelectionAsElementTriggered()
{
    DrawView *view = activeMdiChild();
    if (!view || !view->scene())
        return;
    QGraphicsScene *scene = view->scene();
    QList<QGraphicsItem*> sel = scene->selectedItems();
    bool isElementWindow = view->windowTitle().contains(QLatin1String("[Element]"));
    if (sel.isEmpty() && isElementWindow) {
        scene->clearSelection();
        foreach (QGraphicsItem *it, scene->items()) {
            if (qgraphicsitem_cast<AbstractShape*>(it) && !qgraphicsitem_cast<SizeHandleRect*>(it))
                it->setSelected(true);
        }
        sel = scene->selectedItems();
    }
    if (sel.isEmpty()) {
        statusBar()->showMessage(tr("Select one or more items first"));
        return;
    }
    QString defaultName;
    if (!m_editingElementPath.isEmpty())
        defaultName = QFileInfo(m_editingElementPath).baseName();
    bool ok = false;
    QString name = QInputDialog::getText(this, tr("Save as element"),
                                         tr("Element name:"), QLineEdit::Normal,
                                         defaultName, &ok);
    if (!ok || name.trimmed().isEmpty())
        return;
    name = name.trimmed();
    QString safeName = name;
    safeName.replace(QLatin1Char('/'), QLatin1Char('_'));
    safeName.replace(QLatin1Char('\\'), QLatin1Char('_'));
    QDir elementDir(elementLibraryPath());
    if (!elementDir.exists())
        elementDir.mkpath(QLatin1String("."));
    QString path = elementDir.absoluteFilePath(safeName + QLatin1String(".xml"));
    if (view->saveSelectionAsElement(path, name)) {
        m_editingElementPath.clear();
        statusBar()->showMessage(tr("[Element] Saved to library"), 2000);
        QPixmap icon = view->renderElementPreview(path, QSize(48, 48));
        if (icon.isNull())
            icon = QPixmap(48, 48);
        QListWidgetItem *existing = 0;
        for (int i = 0; i < listViewElements->count(); ++i) {
            if (listViewElements->item(i)->data(Qt::UserRole).toString() == path) {
                existing = listViewElements->item(i);
                break;
            }
        }
        if (existing) {
            existing->setIcon(icon);
            existing->setText(name);
            existing->setData(Qt::UserRole, path);
        } else {
            QListWidgetItem *wi = new QListWidgetItem(icon, name);
            wi->setData(Qt::UserRole, path);
            listViewElements->addItem(wi);
        }
    } else {
        statusBar()->showMessage(tr("Failed to save element"));
    }
}

void MainWindow::addShape()
{
    if ( sender() == selectAct )
        DrawTool::c_drawShape = selection;
    else if (sender() == lineAct )
        DrawTool::c_drawShape = line;
    else if ( sender() == rectAct )
        DrawTool::c_drawShape = rectangle;
    else if ( sender() == roundRectAct )
        DrawTool::c_drawShape = roundrect;
    else if ( sender() == ellipseAct )
        DrawTool::c_drawShape = ellipse ;
    else if ( sender() == arcAct )
        DrawTool::c_drawShape = arc ;
    else if ( sender() == textAct )
        DrawTool::c_drawShape = text ;
    else if ( sender() == polygonAct )
        DrawTool::c_drawShape = polygon;
    else if ( sender() == bezierAct )
        DrawTool::c_drawShape = bezier ;
    else if (sender() == rotateAct )
        DrawTool::c_drawShape = rotation;
    else if (sender() == polylineAct )
        DrawTool::c_drawShape = polyline;

    if ( sender() != selectAct && sender() != rotateAct ){
        DrawView *child = activeMdiChild();
        if (child && child->scene())
            child->scene()->clearSelection();
    }
}

void MainWindow::updateActions()
{

     QGraphicsScene * scene = NULL;
    if (activeMdiChild())
        scene = activeMdiChild()->scene();

    selectAct->setEnabled(scene);
    lineAct->setEnabled(scene);
    rectAct->setEnabled(scene);
    roundRectAct->setEnabled(scene);
    ellipseAct->setEnabled(scene);
    arcAct->setEnabled(scene);
    textAct->setEnabled(scene);
    bezierAct->setEnabled(scene);
    rotateAct->setEnabled(scene);
    polygonAct->setEnabled(scene);
    polylineAct->setEnabled(scene);

    zoomInAct->setEnabled(scene);
    zoomOutAct->setEnabled(scene);

    selectAct->setChecked(DrawTool::c_drawShape == selection);
    lineAct->setChecked(DrawTool::c_drawShape == line);
    rectAct->setChecked(DrawTool::c_drawShape == rectangle);
    roundRectAct->setChecked(DrawTool::c_drawShape == roundrect);
    ellipseAct->setChecked(DrawTool::c_drawShape == ellipse);
    arcAct->setChecked(DrawTool::c_drawShape == arc);
    textAct->setChecked(DrawTool::c_drawShape == text);
    bezierAct->setChecked(DrawTool::c_drawShape == bezier);
    rotateAct->setChecked(DrawTool::c_drawShape == rotation);
    polygonAct->setChecked(DrawTool::c_drawShape == polygon);
    polylineAct->setChecked(DrawTool::c_drawShape == polyline );
    QUndoStack *stack = activeMdiChild() ? activeMdiChild()->undoStack() : 0;
    undoAct->setEnabled(stack && stack->canUndo());
    redoAct->setEnabled(stack && stack->canRedo());


    bringToFrontAct->setEnabled(scene && scene->selectedItems().count() > 0);
    sendToBackAct->setEnabled(scene && scene->selectedItems().count() > 0);
    groupAct->setEnabled( scene && scene->selectedItems().count() > 0);
    unGroupAct->setEnabled(scene &&scene->selectedItems().count() > 0 &&
                              dynamic_cast<GraphicsItemGroup*>( scene->selectedItems().first()));

    leftAct->setEnabled(scene && scene->selectedItems().count() > 1);
    rightAct->setEnabled(scene && scene->selectedItems().count() > 1);
    vCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
    hCenterAct->setEnabled(scene && scene->selectedItems().count() > 1);
    upAct->setEnabled(scene && scene->selectedItems().count() > 1);
    downAct->setEnabled(scene && scene->selectedItems().count() > 1);

    heightAct->setEnabled(scene && scene->selectedItems().count() > 1);
    widthAct->setEnabled(scene &&scene->selectedItems().count() > 1);
    allAct->setEnabled(scene &&scene->selectedItems().count()>1);
    horzAct->setEnabled(scene &&scene->selectedItems().count() > 2);
    vertAct->setEnabled(scene &&scene->selectedItems().count() > 2);

    copyAct->setEnabled(scene && scene->selectedItems().count() > 0);
    cutAct->setEnabled(scene && scene->selectedItems().count() > 0);
    DrawView *activeView = activeMdiChild();
    bool isElementWindow = activeView && activeView->windowTitle().contains(QLatin1String("[Element]"));
    int shapeCount = 0;
    if (scene) {
        foreach (QGraphicsItem *it, scene->items()) {
            if (qgraphicsitem_cast<AbstractShape*>(it) && !qgraphicsitem_cast<SizeHandleRect*>(it))
                ++shapeCount;
        }
    }
    saveAsElementAct->setEnabled(scene && (scene->selectedItems().count() > 0 || (isElementWindow && shapeCount > 0)));
    pasteAct->setEnabled(scene && (dynamic_cast<const ShapeMimeData*>(QApplication::clipboard()->mimeData()) != 0));
}

void MainWindow::onUndo()
{
    DrawView *v = activeMdiChild();
    if (v && v->undoStack()->canUndo())
        v->undoStack()->undo();
}

void MainWindow::onRedo()
{
    DrawView *v = activeMdiChild();
    if (v && v->undoStack()->canRedo())
        v->undoStack()->redo();
}

void MainWindow::itemSelected()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    int n = scene->selectedItems().count();
    if (n == 0) {
        theControlledObject = 0;
        propertyEditor->setObject(0);
        return;
    }
    if (n > 1) {
        // Multiple selection: do not edit single object to avoid confusion
        theControlledObject = 0;
        propertyEditor->setObject(0);
        return;
    }
    if (scene->selectedItems().first()->isSelected()) {
        QGraphicsItem *item = scene->selectedItems().first();
        theControlledObject = dynamic_cast<QObject*>(item);
        propertyEditor->setObject(theControlledObject);
    }
}

void MainWindow::itemMoved(QGraphicsItem *item, const QPointF &oldPosition)
{
    Q_UNUSED(item);
    if (!activeMdiChild()) return ;
        activeMdiChild()->setModified(true);

    QUndoStack *stack = activeMdiChild()->undoStack();
    if (item) {
        stack->push(new MoveShapeCommand(item, oldPosition));
    } else {
        stack->push(new MoveShapeCommand(activeMdiChild()->scene(), oldPosition));
    }
}

void MainWindow::itemAdded(QGraphicsItem *item)
{
    if (!activeMdiChild()) return ;
        activeMdiChild()->setModified(true);

    QUndoCommand *addCommand = new AddShapeCommand(item, item->scene());
    activeMdiChild()->undoStack()->push(addCommand);
}

void MainWindow::itemRotate(QGraphicsItem *item, const qreal oldAngle)
{
    if (!activeMdiChild()) return ;
        activeMdiChild()->setModified(true);

    activeMdiChild()->undoStack()->push(new RotateShapeCommand(item , oldAngle));
}

void MainWindow::itemResize(QGraphicsItem *item, int handle, const QPointF& scale)
{
    if (!activeMdiChild()) return ;
        activeMdiChild()->setModified(true);

    QUndoCommand *resizeCommand = new ResizeShapeCommand(item ,handle, scale );
    activeMdiChild()->undoStack()->push(resizeCommand);
}

void MainWindow::itemControl(QGraphicsItem *item, int handle, const QPointF & newPos ,const QPointF &lastPos_)
{
    if (!activeMdiChild()) return ;
        activeMdiChild()->setModified(true);

    activeMdiChild()->undoStack()->push(new ControlShapeCommand(item ,handle, newPos, lastPos_ ));
}

void MainWindow::deleteItem()
{
    qDebug()<<"deleteItem";
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    activeMdiChild()->setModified(true);

    if (scene->selectedItems().isEmpty())
        return;

    QUndoCommand *deleteCommand = new RemoveShapeCommand(scene);
    activeMdiChild()->undoStack()->push(deleteCommand);
}

void MainWindow::on_actionBringToFront_triggered()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    if (scene->selectedItems().isEmpty())
        return;
    activeMdiChild()->setModified(true);

    QGraphicsItem *selectedItem = scene->selectedItems().first();

    QList<QGraphicsItem *> overlapItems = selectedItem->collidingItems();
    qreal zValue = 0;
    foreach (QGraphicsItem *item, overlapItems) {
        if (item->zValue() >= zValue && item->type() == GraphicsItem::Type)
            zValue = item->zValue() + 0.1;
    }
    selectedItem->setZValue(zValue);


}
void MainWindow::on_actionSendToBack_triggered()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    if (scene->selectedItems().isEmpty())
        return;
    activeMdiChild()->setModified(true);
    activeMdiChild()->undoStack()->push(new ZOrderCommand(scene, ZOrderCommand::SendToBack));
}

void MainWindow::on_aglin_triggered()
{
    if (!activeMdiChild()) return ;
    DrawScene * scene = dynamic_cast<DrawScene*>(activeMdiChild()->scene());
    if (!scene || scene->selectedItems().isEmpty()) return ;

    activeMdiChild()->setModified(true);

    AlignType at = CENTER_ALIGN;
    if ( sender() == rightAct ) at = RIGHT_ALIGN;
    else if ( sender() == leftAct ) at = LEFT_ALIGN;
    else if ( sender() == upAct ) at = UP_ALIGN;
    else if ( sender() == downAct ) at = DOWN_ALIGN;
    else if ( sender() == vCenterAct ) at = VERT_ALIGN;
    else if ( sender() == hCenterAct ) at = HORZ_ALIGN;
    else if ( sender() == heightAct ) at = HEIGHT_ALIGN;
    else if ( sender() == widthAct ) at = WIDTH_ALIGN;
    else if ( sender() == horzAct ) at = HORZEVEN_ALIGN;
    else if ( sender() == vertAct ) at = VERTEVEN_ALIGN;
    else if ( sender() == allAct ) at = ALL_ALIGN;
    QUndoCommand *alignCmd = new AlignCommand(scene, at);
    activeMdiChild()->undoStack()->push(alignCmd);
}

void MainWindow::exportToImage()
{
    if (!activeMdiChild()) return;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export as image"),
                                                    QString(),
                                                    tr("PNG images (*.png);;All files (*)"));
    if (fileName.isEmpty()) return;
    if (activeMdiChild()->exportToPng(fileName))
        statusBar()->showMessage(tr("Exported to %1").arg(fileName), 2000);
    else
        statusBar()->showMessage(tr("Export failed"), 2000);
}

void MainWindow::zoomIn()
{
    if (!activeMdiChild()) return ;
    activeMdiChild()->zoomIn();
}

void MainWindow::zoomOut()
{
    if (!activeMdiChild()) return ;
     activeMdiChild()->zoomOut();
}

void MainWindow::on_group_triggered()
{
    if (!activeMdiChild()) return ;
    DrawScene * scene = dynamic_cast<DrawScene*>(activeMdiChild()->scene());

    //QGraphicsItemGroup
    QList<QGraphicsItem *> selectedItems = scene->selectedItems();
    // Create a new group at that level
    if ( selectedItems.count() < 1) return;
    GraphicsItemGroup *group = scene->createGroup(selectedItems);
    QUndoCommand *groupCommand = new GroupShapeCommand(group,scene);
    activeMdiChild()->undoStack()->push(groupCommand);
}

void MainWindow::on_unGroup_triggered()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();
    if (scene->selectedItems().isEmpty()) return ;

    QGraphicsItem *selectedItem = scene->selectedItems().first();
    GraphicsItemGroup * group = dynamic_cast<GraphicsItemGroup*>(selectedItem);
    if ( group ){
        QUndoCommand *unGroupCommand = new UnGroupShapeCommand(group,scene);
        activeMdiChild()->undoStack()->push(unGroupCommand);
    }
}

void MainWindow::on_func_test_triggered()
{

       QtPenPropertyManager *penPropertyManager = new QtPenPropertyManager();
       QtProperty * property = penPropertyManager->addProperty("pen");

       QtTreePropertyBrowser *editor = new QtTreePropertyBrowser();
       editor->setFactoryForManager(penPropertyManager->subIntPropertyManager(),new QtSpinBoxFactory());
       editor->setFactoryForManager(penPropertyManager->subEnumPropertyManager(),new QtEnumEditorFactory());
       editor->addProperty(property);

       QPen pen;
       pen.setWidth(10);
       pen.setCapStyle(Qt::RoundCap);
       pen.setJoinStyle(Qt::SvgMiterJoin);
       penPropertyManager->setValue(property,pen);

       editor->show();


/*
        QtGradientEditor * editor = new QtGradientEditor(NULL);
        editor->show();
*/
//    dockProperty->showNormal();
}

void MainWindow::on_copy()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    ShapeMimeData * data = new ShapeMimeData( scene->selectedItems() );
    QApplication::clipboard()->setMimeData(data);
}

void MainWindow::on_paste()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    QMimeData * mp = const_cast<QMimeData *>(QApplication::clipboard()->mimeData()) ;
    ShapeMimeData * data = dynamic_cast< ShapeMimeData*>( mp );
    if ( data ){
        scene->clearSelection();
        foreach (QGraphicsItem * item , data->items() ) {
            AbstractShape *sp = qgraphicsitem_cast<AbstractShape*>(item);
            QGraphicsItem * copy = sp->duplicate();
            if ( copy ){
                copy->setSelected(true);
                copy->moveBy(10,10);
                QUndoCommand *addCommand = new AddShapeCommand(copy, scene);
                activeMdiChild()->undoStack()->push(addCommand);
            }
        }
    }
}

void MainWindow::on_cut()
{
    if (!activeMdiChild()) return ;
    QGraphicsScene * scene = activeMdiChild()->scene();

    QList<QGraphicsItem *> copylist ;
    foreach (QGraphicsItem *item , scene->selectedItems()) {
        AbstractShape *sp = qgraphicsitem_cast<AbstractShape*>(item);
        QGraphicsItem * copy = sp->duplicate();
        if ( copy )
            copylist.append(copy);
    }
    QUndoCommand *deleteCommand = new RemoveShapeCommand(scene);
    activeMdiChild()->undoStack()->push(deleteCommand);
    if ( copylist.count() > 0 ){
        ShapeMimeData * data = new ShapeMimeData( copylist );
        QApplication::clipboard()->setMimeData(data);
    }
}

void MainWindow::dataChanged()
{
    const QMimeData *md = QApplication::clipboard()->mimeData();
    pasteAct->setEnabled(md && (dynamic_cast<const ShapeMimeData*>(md) != 0));
}

void MainWindow::positionChanged(int x, int y)
{
   statusBar()->showMessage(QString("%1,%2").arg(x).arg(y));
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

void MainWindow::about()
{

}
