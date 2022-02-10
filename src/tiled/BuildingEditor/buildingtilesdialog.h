/*
 * Copyright 2012, Tim Baker <treectrl@users.sf.net>
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

#ifndef BUILDINGTILESDIALOG_H
#define BUILDINGTILESDIALOG_H

#include <QDialog>
#include <QModelIndex>

class QCheckBox;
class QComboBox;
class QListWidgetItem;
class QSpinBox;
class QSplitter;
class QToolButton;
class QUndoGroup;
class QUndoStack;

namespace Ui {
class BuildingTilesDialog;
}

namespace Tiled {
class Tile;
class Tileset;
namespace Internal {
class Zoomable;
}
}

namespace BuildingEditor {

class BuildingTile;
class BuildingTileEntry;
class BuildingTileCategory;
class FurnitureGroup;
class FurnitureTile;
class FurnitureTiles;

class BuildingTilesDialog : public QDialog
{
    Q_OBJECT
    
public:
    static BuildingTilesDialog *instance();
    static void deleteInstance();

    explicit BuildingTilesDialog(QWidget *parent = 0);
    ~BuildingTilesDialog();

    bool changes();

#ifdef BUILDINGED_SA
    void afterInitConfigFiles();
#endif

    void selectCategory(BuildingTileCategory *category);
    void selectCategory(FurnitureGroup *furnitureGroup);
    void reparent(QWidget *parent);

    //+ UNDO/REDO
    void addTile(BuildingTileCategory *category, int index, BuildingTileEntry *entry);
    BuildingTileEntry *removeTile(BuildingTileCategory *category, int index);
    QString renameTileCategory(BuildingTileCategory *category, const QString &name);

    void addCategory(int index, FurnitureGroup *category);
    FurnitureGroup *removeCategory(int index);
    QString changeEntryTile(BuildingTileEntry *entry, int e, const QString &tileName);
    QPoint changeEntryOffset(BuildingTileEntry *entry, int e, const QPoint &offset);
    void reorderEntry(BuildingTileCategory *category, int oldIndex, int newIndex);

    void insertFurnitureTiles(FurnitureGroup *category, int index,
                              FurnitureTiles *ftiles);
    FurnitureTiles* removeFurnitureTiles(FurnitureGroup *category, int index);
    QString changeFurnitureTile(FurnitureTile *ftile, int x, int y,
                                const QString &tileName);
    void reorderFurniture(FurnitureGroup *category, int oldIndex, int newIndex);
    void toggleCorners(FurnitureTiles *ftiles);
    int changeFurnitureLayer(FurnitureTiles *ftiles, int layer);
    bool changeFurnitureGrime(FurnitureTile *ftile, bool allow);

    QString renameFurnitureCategory(FurnitureGroup *category, const QString &name);
    void reorderCategory(int oldIndex, int newIndex);
    //- UNDO/REDO

signals:
    void edited();

private:
    void setCategoryList();
    void setCategoryTiles();
    void setFurnitureTiles();
    void setTilesetList();
    void saveSplitterSizes(QSplitter *splitter);
    void restoreSplitterSizes(QSplitter *splitter);
    void displayTileInTileset(Tiled::Tile *tile);
    void displayTileInTileset(BuildingTile *tile);

    BuildingTileCategory *categoryAt(int row);
    FurnitureGroup *furnitureGroupAt(int row);

    typedef Tiled::Tileset Tileset;

private slots:
    void categoryChanged(int index);
    void tilesetFilterEdited(const QString &text);
    void tilesetSelectionChanged();
    void synchUI();

    void addTiles();
    void removeTiles();
    void clearTiles();

    void setExpertMode(bool expert);

    void tileDropped(const QString &tilesetName, int tileId);
    void entryTileDropped(BuildingTileEntry *entry, int e, const QString &tileName);

    void furnitureTileDropped(FurnitureTile *ftile, int x, int y,
                              const QString &tileName);

    void categoryNameEdited(QListWidgetItem *item);

    void newCategory();
    void removeCategory();

    void moveCategoryUp();
    void moveCategoryDown();

    void moveTileUp();
    void moveTileDown();

    void toggleCorners();

    void manageTilesets();
    void tilesetAdded(Tiled::Tileset *tileset);
    void tilesetAboutToBeRemoved(Tiled::Tileset *tileset);
    void tilesetRemoved(Tiled::Tileset *tileset);

    void tilesetChanged(Tileset *tileset);

    void undoTextChanged(const QString &text);
    void redoTextChanged(const QString &text);

    void tileSelectionChanged();
    void entrySelectionChanged();
    void furnitureSelectionChanged();

    void tileActivated(const QModelIndex &index);
    void entryActivated(const QModelIndex &index);
    void furnitureActivated(const QModelIndex &index);

    void entryOffsetChanged();

    void furnitureLayerChanged(int index);
    void furnitureGrimeChanged(bool allow);

    void accept();
    void reject();

private:
    static BuildingTilesDialog *mInstance;

    Ui::BuildingTilesDialog *ui;
    Tiled::Internal::Zoomable *mZoomable;
    BuildingTileCategory *mCategory;
    BuildingTileEntry *mCurrentEntry;
    int mCurrentEntryEnum;
    FurnitureGroup *mFurnitureGroup;
    FurnitureTile *mCurrentFurniture;
    Tiled::Tileset *mCurrentTileset;
    int mRowOfFirstCategory;
    int mRowOfFirstFurnitureGroup;
    QUndoGroup *mUndoGroup;
    QUndoStack *mUndoStack;
    QToolButton *mUndoButton;
    QToolButton *mRedoButton;

    QWidget *mEntryOffsetUI;
    QSpinBox *mEntryOffsetSpinX;
    QSpinBox *mEntryOffsetSpinY;
    bool mSynching;

    QWidget *mFurnitureLayerUI;
    QComboBox *mFurnitureLayerComboBox;
    QCheckBox *mFurnitureGrimeCheckBox;

    bool mExpertMode;
    bool mChanges;
};

} // namespace BuildingEditor

#endif // BUILDINGTILESDIALOG_H
