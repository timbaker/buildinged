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

#ifndef BUILDINGDOCUMENT_H
#define BUILDINGDOCUMENT_H

#include "properties.h"

#include <QObject>
#include <QRect>
#include <QRegion>
#include <QSet>

class QUndoStack;

namespace BuildingEditor {

class BuildingObject;
class Building;
class BuildingFloor;
class BuildingTileEntry;
class Door;
class FurnitureObject;
class FurnitureTile;
class FurnitureTiles;
class RoofObject;
class Room;
class FloorTileGrid;
class WallObject;
class Window;

class BuildingDocument : public QObject
{
    Q_OBJECT
public:
    explicit BuildingDocument(Building *building, const QString &fileName);
    ~BuildingDocument();

    Building *building() const
    { return mBuilding; }

    QString fileName() const
    { return mFileName; }

    QString displayName() const;

    static BuildingDocument *read(const QString &fileName, QString &error);
    bool write(const QString &fileName, QString &error);

    void setCurrentFloor(BuildingFloor *floor);

    BuildingFloor *currentFloor() const
    { return mCurrentFloor; }

    int currentLevel() const;

    bool currentFloorIsTop();
    bool currentFloorIsBottom();

    void setCurrentRoom(Room *room);
    Room *currentRoom() const
    { return mCurrentRoom; }

    void setCurrentLayer(const QString &layerName);

    QString currentLayer() const
    { return mCurrentLayerName; }

    void setLayerOpacity(BuildingFloor *floor, const QString &layerName,
                         qreal opacity);
    void setLayerVisibility(BuildingFloor *floor, const QString &layerName,
                            bool visible);

    QUndoStack *undoStack() const
    { return mUndoStack; }

    bool isModified() const;

    void setSelectedObjects(const QSet<BuildingObject*> &selection);

    const QSet<BuildingObject*> &selectedObjects() const
    { return mSelectedObjects; }

    const QRegion &roomSelection() const
    { return mRoomSelection; }

    const QRegion &tileSelection() const
    { return mTileSelection; }

    void emitBuildingResized()
    { emit buildingResized(); }

    void emitBuildingRotated()
    { emit buildingRotated(); }

    void emitObjectChanged(BuildingObject *object)
    { emit objectChanged(object); }

    void emitObjectPicked(BuildingObject *object)
    { emit objectPicked(object); }

    // Clipboard
    void setClipboardTiles(FloorTileGrid *tiles, const QRegion &rgn);

    FloorTileGrid *clipboardTiles() const
    { return mClipboardTiles; }

    QRegion clipboardTilesRgn() const
    { return mClipboardTilesRgn; }

    // +UNDO/REDO
    Room *changeRoomAtPosition(BuildingFloor *floor, const QPoint &pos, Room *room);
    BuildingEditor::BuildingTileEntry *changeBuildingTile(int tileEnum, BuildingTileEntry *tile);
    BuildingTileEntry *changeRoomTile(Room *room, int tileEnum, BuildingTileEntry *tile);

    void insertFloor(int index, BuildingFloor *floor);
    BuildingFloor *removeFloor(int index);
    void reorderFloor(int oldIndex, int newIndex);

    void insertObject(BuildingFloor *floor, int index, BuildingObject *object);
    BuildingObject *removeObject(BuildingFloor *floor, int index);
    QPoint moveObject(BuildingObject *object, const QPoint &pos);
    BuildingTileEntry *changeObjectTile(BuildingObject *object,
                                        BuildingTileEntry *tile, int alternate);

    void insertRoom(int index, Room *room);
    Room *removeRoom(int index);
    int reorderRoom(int index, Room *room);
    Room *changeRoom(Room *room, const Room *data);

    QVector<QVector<Room *> > swapFloorGrid(BuildingFloor *floor,
                                            const QVector<QVector<Room*> > &grid);

    QMap<QString,FloorTileGrid*> swapFloorTiles(BuildingFloor *floor,
                                                const QMap<QString,FloorTileGrid*> &grid,
                                                bool emitSignal);

    FloorTileGrid *swapFloorTiles(BuildingFloor *floor,
                                  const QString &layerName,
                                  const QRegion &rgn,
                                  const QPoint &pos,
                                  const FloorTileGrid *tiles);

    QSize resizeBuilding(const QSize &newSize);
    QVector<QVector<Room *> > resizeFloor(BuildingFloor *floor,
                                          const QVector<QVector<Room*> > &grid,
                                          QMap<QString,FloorTileGrid*> &grime);
    void rotateBuilding(bool right);
    void flipBuilding(bool horizontal);

    FurnitureTile *changeFurnitureTile(FurnitureObject *object, FurnitureTile *ftile);

    void resizeRoof(RoofObject *roof, int &width, int &height, bool &halfDepth);

    int resizeWall(WallObject *wall, int length);

    QList<BuildingTileEntry*> changeUsedTiles(const QList<BuildingTileEntry*> &tiles);
    QList<FurnitureTiles *> changeUsedFurniture(const QList<FurnitureTiles *> &tiles);

    QRegion setRoomSelection(const QRegion &selection);
    QRegion setTileSelection(const QRegion &selection);

    Tiled::Properties changeBuildingProperties(const Tiled::Properties& properties);
    // -UNDO/REDO

signals:
    void currentFloorChanged();
    void currentRoomChanged();
    void currentLayerChanged();

    void roomAtPositionChanged(BuildingEditor::BuildingFloor *floor, const QPoint &pos);
    void roomDefinitionChanged();

    void floorAdded(BuildingEditor::BuildingFloor *floor);
    void floorRemoved(BuildingEditor::BuildingFloor *floor);
    void floorEdited(BuildingEditor::BuildingFloor *floor);

    void floorTilesChanged(BuildingEditor::BuildingFloor *floor);
    void floorTilesChanged(BuildingEditor::BuildingFloor *floor, const QString &layerName,
                           const QRect &bounds);

    void layerOpacityChanged(BuildingEditor::BuildingFloor *floor, const QString &layerName);
    void layerVisibilityChanged(BuildingEditor::BuildingFloor *floor, const QString &layerName);

    void objectAdded(BuildingEditor::BuildingObject *object);
    void objectAboutToBeRemoved(BuildingEditor::BuildingObject *object);
    void objectRemoved(BuildingEditor::BuildingObject *object);
    void objectMoved(BuildingEditor::BuildingObject *object);
    void objectTileChanged(BuildingEditor::BuildingObject *object);
    void objectChanged(BuildingEditor::BuildingObject *object);
    void objectPicked(BuildingEditor::BuildingObject *object);

    void roomAdded(BuildingEditor::Room *room);
    void roomAboutToBeRemoved(BuildingEditor::Room *room);
    void roomRemoved(BuildingEditor::Room *room);
    void roomsReordered();
    void roomChanged(BuildingEditor::Room *room);

    void buildingResized();
    void buildingRotated();

    void selectedObjectsChanged();

    void roomSelectionChanged(const QRegion &old);

    void tileSelectionChanged(const QRegion &old);
    void clipboardTilesChanged();

    void fileNameChanged();
    void cleanChanged();

    void usedTilesChanged();
    void usedFurnitureChanged();

private:
    void checkUsedTile(BuildingTileEntry *entry);
    void checkUsedFurniture(FurnitureTiles *ftiles);

private slots:
    void entryTileChanged(BuildingEditor::BuildingTileEntry *entry);
    void furnitureTileChanged(BuildingEditor::FurnitureTile *ftile);
    void furnitureLayerChanged(BuildingEditor::FurnitureTiles *ftiles);
    
private:
    Building *mBuilding;
    QString mFileName;
    QUndoStack *mUndoStack;
    bool mTileChanges;
    BuildingFloor *mCurrentFloor;
    Room *mCurrentRoom;
    QString mCurrentLayerName;
    QSet<BuildingObject*> mSelectedObjects;
    QRegion mRoomSelection;
    QRegion mTileSelection;
    FloorTileGrid *mClipboardTiles;
    QRegion mClipboardTilesRgn;
};

} // namespace BuildingEditor

#endif // BUILDINGDOCUMENT_H
