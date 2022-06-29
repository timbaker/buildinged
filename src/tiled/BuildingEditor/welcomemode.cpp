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

#include "welcomemode.h"
#include "ui_welcomemode.h"

#include "building.h"
#include "buildingeditorwindow.h"
#include "buildingreader.h"
#include "buildingwriter.h"
#include "ui_buildingeditorwindow.h"
#include "buildingpreferences.h"

#ifdef BUILDINGED_SA
#include "mapmanager.h"
#else
#include "mainwindow.h"
#endif
#include "mapimagemanager.h"

#include <QCompleter>
#include <QDebug>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>

using namespace BuildingEditor;

static QRectF sceneRectOfItem(QGraphicsItem *item)
{
    return item->mapToScene(item->boundingRect()).boundingRect();
}

namespace WelcomeModeNS {

LinkItem::LinkItem(const QString &text1, const QString &text2, QGraphicsItem *parent) :
    QGraphicsItem(parent),
    mRemoveItem(0)
{
    QGraphicsRectItem *bg = new QGraphicsRectItem(this);
    bg->setBrush(QColor(QLatin1String("#f3f3f3")));
    bg->setPen(Qt::NoPen);
    bg->setVisible(false);

    QGraphicsTextItem *item1 = new QGraphicsTextItem(this);
    item1->setPlainText(text1);
    item1->setDefaultTextColor(Qt::blue);

    mBoundingRect = sceneRectOfItem(item1);

    if (!text2.isEmpty()) {
        QGraphicsTextItem *item2 = new QGraphicsTextItem(this);
        QString s = QFontMetrics(item2->font()).elidedText(text2, Qt::ElideRight, 400 - 40);
        item2->setPlainText(s);
        item2->setDefaultTextColor(QColor("#6b6b6b"));
        item2->setPos(0, item1->boundingRect().height());

        mFilePath = text2;
        mBoundingRect |= sceneRectOfItem(item2);
    }

    mBoundingRect.translate(-mBoundingRect.topLeft());
    mBoundingRect.setRight(400);
    bg->setRect(mBoundingRect);

    //        setFlag(ItemHasNoContents);
    setAcceptHoverEvents(true);
}

QRectF LinkItem::boundingRect() const
{
    return mBoundingRect;
}

void LinkItem::paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *)
{

}

void LinkItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
//    Q_UNUSED(event)
    if (mRemoveItem != 0 && event->pos().x() > mBoundingRect.right() - 32) {
        emit clickedRemove();
        return;
    }
    emit clicked();
}

void LinkItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    childItems().first()->setVisible(true);
    if (mRemoveItem != 0)
        mRemoveItem->setVisible(true);
    emit hovered(true);
}

void LinkItem::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    if (mRemoveItem != 0) {
        if (event->pos().x() > mBoundingRect.right() - 32) {
            mRemoveBGItem->setVisible(true);
        } else {
            mRemoveBGItem->setVisible(false);
        }
    }
}

void LinkItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    childItems().first()->setVisible(false);
    if (mRemoveItem != 0) {
        mRemoveBGItem->setVisible(false);
        mRemoveItem->setVisible(false);
    }
    emit hovered(false);
}

void LinkItem::allowRemove()
{
    QGraphicsRectItem *bg = new QGraphicsRectItem(this);
    bg->setBrush(QColor(QLatin1String("#e0e0e0")));
    bg->setPen(Qt::NoPen);
    bg->setRect(mBoundingRect.adjusted(mBoundingRect.right() - 32, 0, 0, 0));
    bg->setVisible(false);
    mRemoveBGItem = bg;

    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(QPixmap(QLatin1String(":/images/16x16/edit-delete.png")), this);
    item->setX(mBoundingRect.right() - item->boundingRect().width() - 8);
    item->setY((mBoundingRect.height() - item->boundingRect().height()) / 2);
    item->setVisible(false);
    mRemoveItem = item;
}

}

WelcomeMode::WelcomeMode(QObject *parent) :
    IMode(parent),
    ui(new Ui::WelcomeMode)
{
    setDisplayName(tr("Welcome"));
    setIcon(QIcon(QLatin1String(":images/24x24/document-new.png")));

    mWidget = new QWidget;
    mWidget->setObjectName(QLatin1String("WelcomeModeWidget"));
    ui->setupUi(mWidget);

    QGraphicsScene *scene = new QGraphicsScene(ui->graphicsView);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    ui->graphicsView->setMouseTracking(true);

    int x = 10;
    int y = 10;
    QGraphicsPixmapItem *pitem = scene->addPixmap(QPixmap::fromImage(QImage(QLatin1String(":/BuildingEditor/icons/icon_pencil.png"))));
//    QGraphicsPixmapItem *pitem = scene->addPixmap(QPixmap::fromImage(QImage(QLatin1String(":/images/tiled-icon-32.png"))));
    pitem->setPos(x, y + 4);
    QGraphicsTextItem *item = scene->addText(tr("BuildingEd"), QFont(QLatin1String("Helvetica"), 16, 1));
    item->setPos(x + 24 + 6, y);
    QRectF r = sceneRectOfItem(item) | sceneRectOfItem(pitem);
    r.translate(0, 12);
    QGraphicsLineItem *line = scene->addLine(r.left(), r.bottom(), 400, r.bottom());
    line->setPen(QPen(Qt::gray));

    x = 40;
    y = r.bottom() + 16;
    WelcomeModeNS::LinkItem *link;
    mNewItem = link = new WelcomeModeNS::LinkItem(tr("New Building"));
    scene->addItem(link);
    link->setPos(x + 6, y);
    connect(mNewItem, &WelcomeModeNS::LinkItem::clicked, this, &WelcomeMode::linkClicked);

    y += 30;
    mOpenItem = link = new WelcomeModeNS::LinkItem(tr("Open Building"));
    scene->addItem(link);
    link->setPos(x + 6, y);
    connect(mOpenItem, &WelcomeModeNS::LinkItem::clicked, this, &WelcomeMode::linkClicked);

    y += sceneRectOfItem(link).height() + 14;
//    QGraphicsLineItem *line2 = scene->addLine(x, y, 300, y);
//    line2->setPen(QPen(Qt::gray));

//    y += 36;
    item = scene->addText(tr("Recent Buildings"), QFont(QLatin1String("Helvetica"), 16, 1));
    item->setPos(x, y);

    y += item->boundingRect().height() + 12;
    mRecentItemsY = y;
    setRecentFiles();

    scene->setSceneRect(0, 0, 400, 600);

    setAutoSaveFiles();
    if (!mAutoSaveItems.isEmpty()) {
        QGraphicsTextItem *item2 = scene->addText(tr("Restore Autosave"), QFont(QLatin1String("Helvetica"), 16, 1));
        item2->setPos(400 + 48, sceneRectOfItem(item).y());
    }

    {
        QFileSystemModel *model = new QFileSystemModel(this);
        model->setRootPath(QDir::rootPath());
        model->setFilter(QDir::AllDirs | QDir::Dirs | QDir::Drives | QDir::NoDotAndDotDot);
        QCompleter *completer = new QCompleter(model, this);
        ui->dirEdit->setCompleter(completer);
    }

    BuildingPreferences *prefs = BuildingPreferences::instance();
    connect(prefs, &BuildingPreferences::mapsDirectoryChanged, this, &WelcomeMode::onMapsDirectoryChanged);

    QDir mapsDir(prefs->mapsDirectory());
    if (!mapsDir.exists())
        mapsDir.setPath(QDir::currentPath());

    {
        ui->treeView->setRootIsDecorated(false);
        ui->treeView->setHeaderHidden(true);
        ui->treeView->setItemsExpandable(false);
        ui->treeView->setUniformRowHeights(true);
//        ui->treeView->setDragEnabled(true);
//        ui->treeView->setDefaultDropAction(Qt::MoveAction);

        QFileSystemModel *model = mFSModel = new QFileSystemModel(this);
        model->setRootPath(mapsDir.absolutePath());

        model->setFilter(QDir::AllDirs | QDir::NoDot | QDir::Files);
        model->setNameFilters(QStringList() << QLatin1String("*.tbx"));
        model->setNameFilterDisables(false); // hide filtered files

        ui->treeView->setModel(model);

        QHeaderView* hHeader = ui->treeView->header();
        hHeader->hideSection(1); // Size
        hHeader->hideSection(2);
        hHeader->hideSection(3);

        ui->treeView->setRootIndex(model->index(mapsDir.absolutePath()));

        hHeader->setStretchLastSection(false);
#if QT_VERSION >= 0x050000
        hHeader->setSectionResizeMode(0, QHeaderView::Stretch);
        hHeader->setSectionResizeMode(1, QHeaderView::ResizeToContents);
#else
        hHeader->setResizeMode(0, QHeaderView::Stretch);
        hHeader->setResizeMode(1, QHeaderView::ResizeToContents);
#endif
        connect(ui->treeView, &QAbstractItemView::activated,
                this, &WelcomeMode::onActivated);
        connect(ui->treeView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &WelcomeMode::selectionChanged);
    }

    ui->dirEdit->setText(QDir::toNativeSeparators(mapsDir.canonicalPath()));
    connect(ui->dirEdit, &QLineEdit::returnPressed, this, &WelcomeMode::editedMapsDirectory);

    connect(ui->dirBrowse, &QAbstractButton::clicked, this, &WelcomeMode::browse);

    connect(ui->legendCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &WelcomeMode::legendIndexChanged);
    connect(ui->legendCombo, &QComboBox::editTextChanged,
            this, &WelcomeMode::legendTextChanged);

    // TODO: set valid Legend values from a .txt file
    mLegendStrings += QStringLiteral("<NONE>");
    mLegendStrings += QStringLiteral("CommunityServices");
    mLegendStrings += QStringLiteral("Hospitality");
    mLegendStrings += QStringLiteral("Industrial");
    mLegendStrings += QStringLiteral("Medical");
    mLegendStrings += QStringLiteral("Residential");
    mLegendStrings += QStringLiteral("RestaurantsAndEntertainment");
    mLegendStrings += QStringLiteral("RetailAndCommercial");
    ui->legendCombo->lineEdit()->setPlaceholderText(QStringLiteral("<NONE>"));
    ui->legendCombo->setDuplicatesEnabled(false);
    ui->legendCombo->addItems(mLegendStrings);

    setWidget(mWidget);

    connect(MapImageManager::instance(), &MapImageManager::mapImageChanged,
            this, &WelcomeMode::onMapImageChanged);
    connect(MapImageManager::instance(), &MapImageManager::mapImageFailedToLoad,
            this, &WelcomeMode::mapImageFailedToLoad);

    connect(BuildingEditorWindow::instance(), &BuildingEditorWindow::recentFilesChanged,
            this, &WelcomeMode::setRecentFiles);
}

void WelcomeMode::readSettings(QSettings &settings)
{
    Q_UNUSED(settings)
}

void WelcomeMode::writeSettings(QSettings &settings)
{
    Q_UNUSED(settings)
}

void WelcomeMode::onActivated(const QModelIndex &index)
{
    QString path = mFSModel->filePath(index);
    QFileInfo fileInfo(path);
    if (fileInfo.isDir()) {
        BuildingPreferences *prefs = BuildingPreferences::instance();
        prefs->setMapsDirectory(fileInfo.canonicalFilePath());
        return;
    }
#ifdef BUILDINGED_SA
    if (path.endsWith(QLatin1String(".tbx"))) {
        BuildingEditorWindow::instance()->openFile(path);
    }
#else
    Tiled::Internal::MainWindow::instance()->openFile(path);
#endif
}

void WelcomeMode::browse()
{
    QString f = QFileDialog::getExistingDirectory(widget()->window(), tr("Choose the Maps Folder"),
        ui->dirEdit->text());
    if (!f.isEmpty()) {
        BuildingPreferences *prefs = BuildingPreferences::instance();
        prefs->setMapsDirectory(f);
    }
}

void WelcomeMode::editedMapsDirectory()
{
    BuildingPreferences *prefs = BuildingPreferences::instance();
    prefs->setMapsDirectory(ui->dirEdit->text());
}

void WelcomeMode::onMapsDirectoryChanged()
{
    BuildingPreferences *prefs = BuildingPreferences::instance();
    QDir mapsDir(prefs->mapsDirectory());
    if (!mapsDir.exists())
        mapsDir.setPath(QDir::currentPath());
    mFSModel->setRootPath(mapsDir.canonicalPath());
    ui->treeView->setRootIndex(mFSModel->index(mapsDir.absolutePath()));
    ui->dirEdit->setText(QDir::toNativeSeparators(prefs->mapsDirectory()));
}

void WelcomeMode::selectionChanged()
{
    synchLegendCombo();

    QString path = currentFilePath();
    if (path.isEmpty()) {
        ui->label->setPixmap(QPixmap());
        mPreviewMapImage = nullptr;
        return;
    }
    MapImage *mapImage = MapImageManager::instance()->getMapImage(path);
    if (mapImage) {
        if (mapImage->isLoaded()) {
            QImage image = mapImage->image().scaled(ui->label->width(), ui->label->height(), Qt::KeepAspectRatio,
                                                    Qt::SmoothTransformation);
            ui->label->setPixmap(QPixmap::fromImage(image));
        }
    } else {
        ui->label->setPixmap(QPixmap());
    }
    mPreviewMapImage = mapImage;
}

void WelcomeMode::synchLegendCombo()
{
    mSynchLegend = true;
    ui->legendCombo->setCurrentIndex(-1);
    ui->legendCombo->setEnabled(false);

    QString path = currentFilePath();
    if (path.isEmpty()) {
        mSynchLegend = false;
        return;
    }
    if (MapInfo *mapInfo = MapManager::instance()->mapInfo(path)) {
        ui->legendCombo->setEnabled(true);
        if (mapInfo->properties().contains(QStringLiteral("Legend"))) {
            QString legend = mapInfo->properties()[QStringLiteral("Legend")].trimmed();
            if (legend.isEmpty() == false) {
                int index = ui->legendCombo->findText(legend); // mLegendStrings.indexOf(legend);
                if (index == -1) {
                    ui->legendCombo->addItem(legend);
                    //mLegendStrings += legend;
                    //index = mLegendStrings.size() - 1;
                    index = ui->legendCombo->count() - 1;
                }
                ui->legendCombo->setCurrentIndex(index);
            }
        }
    }
    mSynchLegend = false;
}

void WelcomeMode::onMapImageChanged(MapImage *mapImage)
{
    if ((mapImage == mPreviewMapImage) && mapImage->isLoaded()) {
        QImage image = mapImage->image().scaled(ui->label->width(), ui->label->height(), Qt::KeepAspectRatio,
                                                Qt::SmoothTransformation);
        ui->label->setPixmap(QPixmap::fromImage(image));

        synchLegendCombo();
    }
}

void WelcomeMode::mapImageFailedToLoad(MapImage *mapImage)
{
    if (mapImage == mPreviewMapImage) {
        ui->label->setPixmap(QPixmap());
    }
}

void WelcomeMode::setRecentFiles()
{
    qDeleteAll(mRecentItems);
    mRecentItems.clear();

    int x = 40;
    int y = mRecentItemsY;
    foreach (QString f, BuildingEditorWindow::instance()->recentFiles()) {
        WelcomeModeNS::LinkItem *link = new WelcomeModeNS::LinkItem(QFileInfo(f).fileName(), QDir::toNativeSeparators(f));
        ui->graphicsView->scene()->addItem(link);
        link->setPos(x + 6, y);
        mRecentItems += link;
        connect(link, &WelcomeModeNS::LinkItem::clicked, this, &WelcomeMode::linkClicked);
        connect(link, &WelcomeModeNS::LinkItem::hovered, this, &WelcomeMode::linkHovered);
        y += link->boundingRect().height() + 12;
    }

    QRectF sceneRect(0, 0, 400, 300);
    if (mRecentItems.size())
        sceneRect |= sceneRectOfItem(mRecentItems.last());
    if (mAutoSaveItems.size())
        sceneRect |= sceneRectOfItem(mAutoSaveItems.last());
    ui->graphicsView->setSceneRect(sceneRect);
}

void WelcomeMode::linkClicked()
{
    WelcomeModeNS::LinkItem *link = static_cast<WelcomeModeNS::LinkItem *>(sender());
    if (link == mNewItem) BuildingEditorWindow::instance()->actionIface()->actionNewBuilding->trigger();
    if (link == mOpenItem) BuildingEditorWindow::instance()->actionIface()->actionOpen->trigger();
    int index = mRecentItems.indexOf(link);
    if (index >= 0) {
        QString fileName = BuildingEditorWindow::instance()->recentFiles().at(index);
        BuildingEditorWindow::instance()->openFile(fileName);
    }
    index = mAutoSaveItems.indexOf(link);
    if (index >= 0) {
        BuildingEditorWindow::instance()->openAutoSave(link->filePath());
    }
}

void WelcomeMode::linkClickedRemove()
{
    WelcomeModeNS::LinkItem *link = static_cast<WelcomeModeNS::LinkItem *>(sender());
    QFileInfo fileInfo(link->filePath());
    if (fileInfo.exists()) {
        if (QMessageBox::question(BuildingEditorWindow::instance(), tr("Remove Autosave File"),
                                  tr("Remove this file from your computer?\n%1").arg(link->filePath()),
                                  QMessageBox::Yes, QMessageBox::No) != QMessageBox::Yes)
            return;
        if (QFile::remove(link->filePath()) == false)
            QMessageBox::warning(BuildingEditorWindow::instance(), tr("Remove File Failed"),
                                 tr("Something went wrong trying to remove this file:\n%1").arg(link->filePath()));
    }
    setAutoSaveFiles();
}

void WelcomeMode::linkHovered(bool hover)
{
    if (hover) {
        WelcomeModeNS::LinkItem *link = static_cast<WelcomeModeNS::LinkItem *>(sender());
        int index = mRecentItems.indexOf(link);
        if (index >= 0) {
            QString fileName = BuildingEditorWindow::instance()->recentFiles().at(index);
            MapImage *mapImage = MapImageManager::instance()->getMapImage(fileName);
            if (mapImage) {
                if (mapImage->isLoaded()) {
                    QImage image = mapImage->image().scaled(ui->label->width(), ui->label->height(), Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation);
                    ui->label->setPixmap(QPixmap::fromImage(image));
                }
                mPreviewMapImage = mapImage;
                return;
            }
        }
        index = mAutoSaveItems.indexOf(link);
        if (index >= 0) {
            MapImage *mapImage = MapImageManager::instance()->getMapImage(link->filePath());
            if (mapImage) {
                if (mapImage->isLoaded()) {
                    QImage image = mapImage->image().scaled(ui->label->width(), ui->label->height(), Qt::KeepAspectRatio,
                                                            Qt::SmoothTransformation);
                    ui->label->setPixmap(QPixmap::fromImage(image));
                }
                mPreviewMapImage = mapImage;
                return;
            }
        }
    }
    selectionChanged();
}

void WelcomeMode::legendIndexChanged(int index)
{
    // Called after ENTERing new text, after adding it to the combobox items.
#if !defined(QT_NO_DEBUG)
    qDebug() << "legendIndexChanged" << index;
#endif
    if (mSynchLegend)
        return;

    QString legend = (index < 1) ? QString() : ui->legendCombo->itemText(index).trimmed();

    QString path = currentFilePath();
    if (path.isEmpty())
        return;

    MapInfo *mapInfo = MapManager::instance()->mapInfo(path);
    if (mapInfo == nullptr)
        return;

    QString LEGEND = QStringLiteral("Legend");
    QString current = mapInfo->properties().value(LEGEND, QString());
    if (legend.isEmpty()) {
        if (current.isEmpty()) {
            return;
        }
    } else {
        if (current == legend) {
            return;
        }
    }

    qDebug() << "Updating Legend property in" << path;

    // 1) Read the TBX
    // 2) Set the Legend= property
    // 3) Save the TBX
    BuildingReader reader;
    if (Building *building = reader.read(path)) {
        reader.fix(building);
        if (legend.isEmpty()) {
            building->properties().remove(LEGEND);
        } else {
            building->properties().insert(LEGEND, legend);
        }
        BuildingWriter w;
        if (!w.write(building, path)) {
            QString error = w.errorString();
            QMessageBox::warning(BuildingEditorWindow::instance(), tr("Error saving building"), error);
        }
        delete building;
    }
}

void WelcomeMode::legendTextChanged(const QString &text)
{
    // Called when typing text.
#if !defined(QT_NO_DEBUG)
    qDebug() << "legendTextChanged" << text;
#endif
}

void WelcomeMode::setAutoSaveFiles()
{
    qDeleteAll(mAutoSaveItems);
    mAutoSaveItems.clear();

    QString configPath = BuildingPreferences::instance()->configPath();
    QDir configDir(configPath);
    QStringList nameFilters;
    nameFilters += QLatin1String("*.autosave");
    QFileInfoList fileInfos = configDir.entryInfoList(nameFilters);
    if (fileInfos.isEmpty())
        return;

    int x = 400 + 48;
    int y = mRecentItemsY;
    foreach (QFileInfo fileInfo, fileInfos) {
        WelcomeModeNS::LinkItem *link = new WelcomeModeNS::LinkItem(fileInfo.fileName(), QDir::toNativeSeparators(fileInfo.filePath()));
        link->allowRemove();
        ui->graphicsView->scene()->addItem(link);
        link->setPos(x + 6, y);
        mAutoSaveItems += link;
        connect(link, &WelcomeModeNS::LinkItem::clicked, this, &WelcomeMode::linkClicked);
        connect(link, &WelcomeModeNS::LinkItem::clickedRemove, this, &WelcomeMode::linkClickedRemove);
        connect(link, &WelcomeModeNS::LinkItem::hovered, this, &WelcomeMode::linkHovered);
        y += link->boundingRect().height() + 12;
    }

    QRectF sceneRect(0, 0, 400, 300);
    if (mRecentItems.size())
        sceneRect |= sceneRectOfItem(mRecentItems.last());
    if (mAutoSaveItems.size())
        sceneRect |= sceneRectOfItem(mAutoSaveItems.last());
    ui->graphicsView->setSceneRect(sceneRect);
}

QString WelcomeMode::currentFilePath()
{
    QModelIndexList selectedRows = ui->treeView->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        return QString();
    }
    QModelIndex index = selectedRows.first();
    QString path = mFSModel->filePath(index);
    if (QFileInfo(path).isDir()) {
        return QString();
    }
    return path;
}
