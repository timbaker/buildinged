/*
 * Copyright 2013, Tim Baker <treectrl@users.sf.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "tileeditmode.h"
#include "tileeditmode_p.h"

#include "building.h"
#include "buildingdocument.h"
#include "buildingdocumentmgr.h"
#include "buildingeditorwindow.h"
#include "buildingfloor.h"
#include "ui_buildingeditorwindow.h"
#include "buildingfurnituredock.h"
#include "buildingisoview.h"
#include "buildinglayersdock.h"
#include "buildingtemplates.h"
#include "buildingtilesetdock.h"
#include "buildingtiletools.h"
#include "editmodestatusbar.h"
#include "embeddedmainwindow.h"

#include "zoomable.h"

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QUndoStack>
#include <QVBoxLayout>

#define docman() BuildingDocumentMgr::instance()

using namespace BuildingEditor;

/////

TileEditModeToolBar::TileEditModeToolBar(QWidget *parent) :
    QToolBar(parent),
    mCurrentDocument(0)
{
    setObjectName(QString::fromUtf8("TileEditModeToolBar"));
    setWindowTitle(tr("Tile ToolBar"));

    mFloorLabel = new QToolButton;
    mFloorLabel->setMinimumWidth(90);
    mFloorLabel->setAutoRaise(true);
    mFloorLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    mFloorLabel->setToolTip(tr("Click to edit floors"));
    connect(mFloorLabel, &QAbstractButton::clicked,
            BuildingEditorWindow::instance(), &BuildingEditorWindow::floorsDialog);

    addAction(BuildingEditorWindow::instance()->actionIface()->actionDrawTiles);
    addAction(BuildingEditorWindow::instance()->actionIface()->actionSelectTiles);
    addAction(BuildingEditorWindow::instance()->actionIface()->actionPickTiles);
    addSeparator();
    addWidget(mFloorLabel);
    addAction(BuildingEditorWindow::instance()->actionIface()->actionUpLevel);
    addAction(BuildingEditorWindow::instance()->actionIface()->actionDownLevel);

    connect(docman(), &BuildingDocumentMgr::currentDocumentChanged,
            this, &TileEditModeToolBar::currentDocumentChanged);
}

void TileEditModeToolBar::currentDocumentChanged(BuildingDocument *doc)
{
    if (mCurrentDocument)
        mCurrentDocument->disconnect(this);

    mCurrentDocument = doc;

    if (mCurrentDocument) {
        connect(mCurrentDocument, &BuildingDocument::floorAdded,
                this, &TileEditModeToolBar::updateActions);
        connect(mCurrentDocument, &BuildingDocument::floorRemoved,
                this, &TileEditModeToolBar::updateActions);
        connect(mCurrentDocument, &BuildingDocument::currentFloorChanged,
                this, &TileEditModeToolBar::updateActions);
    }

    updateActions();
}

void TileEditModeToolBar::updateActions()
{
    if (mCurrentDocument)
        mFloorLabel->setText(tr("Floor %1/%2")
                             .arg(mCurrentDocument->currentLevel() + 1)
                             .arg(mCurrentDocument->building()->floorCount()));
    else
        mFloorLabel->setText(QString());
    mFloorLabel->setEnabled(mCurrentDocument != 0);
}

/////

TileEditModePerDocumentStuff::TileEditModePerDocumentStuff(
        TileEditMode *mode, BuildingDocument *doc) :
    QObject(doc),
    mMode(mode),
    mDocument(doc),
    mIsoView(new BuildingIsoView),
    mIsoScene(new BuildingIsoScene(mIsoView))
{
    mIsoScene->setEditingTiles(true);
    mIsoView->setScene(mIsoScene);
    mIsoView->setDocument(document());

    connect(document(), &BuildingDocument::fileNameChanged, this, &TileEditModePerDocumentStuff::updateDocumentTab);
    connect(document(), &BuildingDocument::cleanChanged, this, &TileEditModePerDocumentStuff::updateDocumentTab);
    connect(document()->undoStack(), &QUndoStack::cleanChanged, this, &TileEditModePerDocumentStuff::updateDocumentTab);

    connect(ToolManager::instance(), &ToolManager::currentEditorChanged,
            this, &TileEditModePerDocumentStuff::updateActions);
}

TileEditModePerDocumentStuff::~TileEditModePerDocumentStuff()
{
    // This is added to a QTabWidget.
    // Removing a tab does not delete the page widget.
    // mIsoScene is a child of the view.
    delete mIsoView;
}

Tiled::Internal::Zoomable *TileEditModePerDocumentStuff::zoomable() const
{
    return mIsoView->zoomable();
}

void TileEditModePerDocumentStuff::activate()
{
    ToolManager::instance()->setEditor(scene());

    connect(view(), &BuildingIsoView::mouseCoordinateChanged,
            mMode->mStatusBar, &EditModeStatusBar::mouseCoordinateChanged);
    connect(zoomable(), &Tiled::Internal::Zoomable::scaleChanged,
            this, &TileEditModePerDocumentStuff::updateActions);

    zoomable()->connectToComboBox(mMode->mStatusBar->editorScaleComboBox);

    connect(document(), &BuildingDocument::tileSelectionChanged,
            this, &TileEditModePerDocumentStuff::updateActions);
    connect(document(), &BuildingDocument::clipboardTilesChanged,
            this, &TileEditModePerDocumentStuff::updateActions);

//    connect(document(), SIGNAL(cleanChanged()), SLOT(updateWindowTitle()));

    Ui::BuildingEditorWindow *actions = BuildingEditorWindow::instance()->actionIface();
    connect(actions->actionZoomIn, &QAction::triggered,
            this, &TileEditModePerDocumentStuff::zoomIn);
    connect(actions->actionZoomOut, &QAction::triggered,
            this, &TileEditModePerDocumentStuff::zoomOut);
    connect(actions->actionNormalSize, &QAction::triggered,
            this, &TileEditModePerDocumentStuff::zoomNormal);
}

void TileEditModePerDocumentStuff::deactivate()
{
//    document()->disconnect(this);
//    document()->disconnect(mMode); /////
    view()->disconnect(mMode->mStatusBar);
    view()->disconnect(this);
    zoomable()->disconnect(this);

    Ui::BuildingEditorWindow *actions = BuildingEditorWindow::instance()->actionIface();
    actions->actionZoomIn->disconnect(this);
    actions->actionZoomOut->disconnect(this);
    actions->actionNormalSize->disconnect(this);

    actions->actionZoomIn->setEnabled(false);
    actions->actionZoomOut->setEnabled(false);
    actions->actionNormalSize->setEnabled(false);
}

void TileEditModePerDocumentStuff::updateDocumentTab()
{
    int tabIndex = BuildingDocumentMgr::instance()->indexOf(document());
    if (tabIndex == -1)
        return;

    QString tabText = document()->displayName();
    if (document()->isModified())
        tabText.prepend(QLatin1Char('*'));
    mMode->mTabWidget->setTabText(tabIndex, tabText);

    QString tooltipText = QDir::toNativeSeparators(document()->fileName());
    mMode->mTabWidget->setTabToolTip(tabIndex, tooltipText);
}


void TileEditModePerDocumentStuff::zoomIn()
{
    zoomable()->zoomIn();
}

void TileEditModePerDocumentStuff::zoomOut()
{
    zoomable()->zoomOut();
}

void TileEditModePerDocumentStuff::zoomNormal()
{
    zoomable()->resetZoom();
}

void TileEditModePerDocumentStuff::updateActions()
{
    if (ToolManager::instance()->currentEditor() == scene()) {
        BuildingEditorWindow::instance()->actionIface()->actionZoomIn->setEnabled(zoomable()->canZoomIn());
        BuildingEditorWindow::instance()->actionIface()->actionZoomOut->setEnabled(zoomable()->canZoomOut());
        BuildingEditorWindow::instance()->actionIface()->actionNormalSize->setEnabled(zoomable()->scale() != 1.0);
    }
}

/////

TileEditMode::TileEditMode(QObject *parent) :
    IMode(parent),
    mTabWidget(new QTabWidget),
    mStatusBar(new EditModeStatusBar(QLatin1String("TileEditModeStatusBar."))),
    mToolBar(new TileEditModeToolBar),
    mFurnitureDock(new BuildingFurnitureDock),
    mLayersDock(new BuildingLayersDock),
    mTilesetDock(new BuildingTilesetDock),
    mFirstTimeSeen(true),
    mCurrentDocument(0),
    mCurrentDocumentStuff(0)
{
    setDisplayName(tr("Tile"));
    setIcon(QIcon(QLatin1String(":/BuildingEditor/icons/mode_tile.png")));

    mMainWindow = new EmbeddedMainWindow;
    mMainWindow->setObjectName(QString::fromUtf8("TileEditMode.Widget"));

    mTabWidget->setObjectName(QString::fromUtf8("TileEditMode.TabWidget"));
    mTabWidget->setDocumentMode(true);
    mTabWidget->setTabsClosable(true);

    mStatusBar->setObjectName(QLatin1String("TileEditMode.StatusBar"));

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setObjectName(QLatin1String("TileEditMode.VBox"));
    vbox->setContentsMargins(0, 0, 0, 0);
//    vbox->addWidget(mToolBar);
    vbox->addWidget(mTabWidget);
    vbox->setStretchFactor(mTabWidget, 1);
    vbox->addLayout(mStatusBar->statusBarLayout);
    QWidget *w = new QWidget;
    w->setObjectName(QString::fromUtf8("TileEditMode.VBoxWidget"));
    w->setLayout(vbox);

    QToolBar *commonToolBar = BuildingEditorWindow::instance()->createCommonToolBar();
    commonToolBar->setObjectName(QLatin1String("TileEditMode.CommonToolBar"));

    mMainWindow->setCentralWidget(w);
    mMainWindow->addToolBar(Qt::LeftToolBarArea, commonToolBar);
    mMainWindow->addToolBar(mToolBar);
    mMainWindow->registerDockWidget(mLayersDock);
    mMainWindow->registerDockWidget(mTilesetDock);
    mMainWindow->registerDockWidget(mFurnitureDock);
    mMainWindow->addDockWidget(Qt::RightDockWidgetArea, mLayersDock);
    mMainWindow->addDockWidget(Qt::RightDockWidgetArea, mTilesetDock);
    mMainWindow->tabifyDockWidget(mTilesetDock, mFurnitureDock);

    connect(mTabWidget, &QTabWidget::currentChanged,
            this, &TileEditMode::currentDocumentTabChanged);
    connect(mTabWidget, &QTabWidget::tabCloseRequested,
            this, &TileEditMode::documentTabCloseRequested);

#ifndef BUILDINGED_SA
    if (mFirstTimeSeen) {
        mFirstTimeSeen = false;
        mFurnitureDock->switchTo();
        mTilesetDock->firstTimeSetup();
    }
#endif

    setWidget(mMainWindow);

    connect(BuildingDocumentMgr::instance(), &BuildingDocumentMgr::documentAdded,
            this, &TileEditMode::documentAdded);
    connect(BuildingDocumentMgr::instance(), &BuildingDocumentMgr::currentDocumentChanged,
            this, &TileEditMode::currentDocumentChanged);
    connect(BuildingDocumentMgr::instance(), &BuildingDocumentMgr::documentAboutToClose,
            this, &TileEditMode::documentAboutToClose);

    connect(this, &IMode::activeStateChanged, this, &TileEditMode::onActiveStateChanged);
}

#define WIDGET_STATE_VERSION 0
void TileEditMode::readSettings(QSettings &settings)
{
    settings.beginGroup(QLatin1String("TileEditMode"));
    mMainWindow->readSettings(settings);
    mFurnitureDock->readSettings(settings);
    settings.endGroup();
}

void TileEditMode::writeSettings(QSettings &settings)
{
    settings.beginGroup(QLatin1String("TileEditMode"));
    mMainWindow->writeSettings(settings);
    mFurnitureDock->writeSettings(settings);
    settings.endGroup();
}

void TileEditMode::onActiveStateChanged(bool active)
{
    QMenu *menu = BuildingEditorWindow::instance()->actionIface()->menuViews;
    menu->clear();

    if (active) {
        if (mCurrentDocumentStuff)
            mCurrentDocumentStuff->activate();

        QMap<QString,QAction*> map;
        foreach (QDockWidget *dockWidget, mMainWindow->dockWidgets()) {
            QAction *action = dockWidget->toggleViewAction();
            map[action->text()] = action;
        }
        foreach (QAction *action, map.values())
            menu->addAction(action);
        menu->addSeparator();
        foreach (QToolBar *toolBar, mMainWindow->toolBars())
            menu->addAction(toolBar->toggleViewAction());

#ifdef BUILDINGED_SA
        if (mFirstTimeSeen) {
            mFirstTimeSeen = false;
            mFurnitureDock->switchTo();
            mTilesetDock->firstTimeSetup();
        }
#endif
    } else {
        if (mCurrentDocumentStuff)
            mCurrentDocumentStuff->deactivate();
    }
}

void TileEditMode::documentAdded(BuildingDocument *doc)
{
    mDocumentStuff[doc] = new TileEditModePerDocumentStuff(this, doc);

    int docIndex = BuildingDocumentMgr::instance()->indexOf(doc);
    mTabWidget->blockSignals(true);
    mTabWidget->insertTab(docIndex, mDocumentStuff[doc]->view(), doc->displayName());
    mTabWidget->blockSignals(false);
    mDocumentStuff[doc]->updateDocumentTab();

    // Hack to keep iso/tile view position + scale synched.
    emit viewAddedForDocument(doc, mDocumentStuff[doc]->view());
}

void TileEditMode::currentDocumentChanged(BuildingDocument *doc)
{
    if (mCurrentDocument) {
        if (isActive())
            mCurrentDocumentStuff->deactivate();
    }

    mCurrentDocument = doc;
    mCurrentDocumentStuff = doc ? mDocumentStuff[doc] : 0;

    if (mCurrentDocument) {
        mTabWidget->setCurrentIndex(docman()->indexOf(doc));
        if (isActive())
            mCurrentDocumentStuff->activate();
    }
}

void TileEditMode::documentAboutToClose(int index, BuildingDocument *doc)
{
    Q_UNUSED(doc)
    // At this point, the document is not in the DocumentManager's list of documents.
    // Removing the current tab will cause another tab to be selected and
    // the current document to change.
    mTabWidget->removeTab(index);
}

void TileEditMode::currentDocumentTabChanged(int index)
{
    docman()->setCurrentDocument(index);
}

void TileEditMode::documentTabCloseRequested(int index)
{
    BuildingEditorWindow::instance()->documentTabCloseRequested(index);
}

void TileEditMode::updateActions()
{
}

