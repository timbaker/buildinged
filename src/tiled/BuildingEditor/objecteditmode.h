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

#ifndef OBJECTEDITMODE_H
#define OBJECTEDITMODE_H

#include "imode.h"

#include <QMap>

class QAction;
class QComboBox;
class QGraphicsView;
class QMainWindow;
class QTabWidget;

namespace BuildingEditor {

class Building;
class BuildingDocument;
class BuildingIsoView;
class CategoryDock;
class EditModeStatusBar;
class EmbeddedMainWindow;
class Room;

class ObjectEditModePerDocumentStuff;
class ObjectEditModeToolBar;

class ObjectEditMode : public IMode
{
    Q_OBJECT
public:
    ObjectEditMode(QObject *parent = 0);

    Building *currentBuilding() const;
    Room *currentRoom() const;

    void readSettings(QSettings &settings);
    void writeSettings(QSettings &settings);

#ifdef BUILDINGED_SA
    void afterInitConfigFiles();
#endif

protected slots:
    void onActiveStateChanged(bool active);

    virtual void documentAdded(BuildingEditor::BuildingDocument *doc);
    void currentDocumentChanged(BuildingEditor::BuildingDocument *doc);
    void documentAboutToClose(int index, BuildingEditor::BuildingDocument *doc);

    void currentDocumentTabChanged(int index);
    void documentTabCloseRequested(int index);

    void updateActions();

protected:
    EmbeddedMainWindow *mMainWindow;
    QTabWidget *mTabWidget;
    ObjectEditModeToolBar *mToolBar;
    EditModeStatusBar *mStatusBar;
    CategoryDock *mCategoryDock;

    BuildingDocument *mCurrentDocument;
    ObjectEditModePerDocumentStuff *mCurrentDocumentStuff;

    friend class ObjectEditModePerDocumentStuff;
    QMap<BuildingDocument*,ObjectEditModePerDocumentStuff*> mDocumentStuff;
    virtual ObjectEditModePerDocumentStuff *createPerDocumentStuff(BuildingDocument *doc) = 0;

    QString mSettingsPrefix;
};

class OrthoObjectEditMode : public ObjectEditMode
{
public:
    OrthoObjectEditMode(QObject *parent = 0);

private:
    ObjectEditModePerDocumentStuff *createPerDocumentStuff(BuildingDocument *doc);
};

class IsoObjectEditMode : public ObjectEditMode
{
    Q_OBJECT
public:
    IsoObjectEditMode(QObject *parent = 0);

signals:
    void viewAddedForDocument(BuildingEditor::BuildingDocument *doc, BuildingEditor::BuildingIsoView *view);

private slots:
    void documentAdded(BuildingEditor::BuildingDocument *doc);

private:
    ObjectEditModePerDocumentStuff *createPerDocumentStuff(BuildingDocument *doc);
};

} // namespace BuildingEditor

#endif // OBJECTEDITMODE_H
