/*
 * main.cpp
 * Copyright 2008-2011, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2011, Ben Longbons <b.r.longbons@gmail.com>
 * Copyright 2011, Stefan Beller <stefanbeller@googlemail.com>
 *
 * This file is part of Tiled.
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

#include "commandlineparser.h"
#include "languagemanager.h"
#include "preferences.h"
#include "tiledapplication.h"
#include "zprogress.h"

#include "texturemanager.h"
#include "tilemetainfomgr.h"
#include "virtualtileset.h"
#include "BuildingEditor/buildingeditorwindow.h"
#include "BuildingEditor/buildingtemplates.h"
#include "BuildingEditor/buildingtiles.h"
#include "BuildingEditor/buildingtmx.h"
#include "BuildingEditor/furnituregroups.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QtPlugin>

#ifdef STATIC_BUILD
Q_IMPORT_PLUGIN(qgif)
Q_IMPORT_PLUGIN(qjpeg)
Q_IMPORT_PLUGIN(qtiff)
#endif

#define STRINGIFY(x) #x
#define AS_STRING(x) STRINGIFY(x)

using namespace Tiled::Internal;
using namespace BuildingEditor;

bool gStartupBlockRendering = true;

namespace {

class CommandLineHandler : public CommandLineParser
{
public:
    CommandLineHandler();

    bool quit;
    bool showedVersion;
    bool disableOpenGL;

private:
    void showVersion();
    void justQuit();
    void setDisableOpenGL();

    // Convenience wrapper around registerOption
    template <void (CommandLineHandler::*memberFunction)()>
    void option(QChar shortName,
                const QString &longName,
                const QString &help)
    {
        registerOption<CommandLineHandler, memberFunction>(this,
                                                           shortName,
                                                           longName,
                                                           help);
    }
};

} // anonymous namespace


CommandLineHandler::CommandLineHandler()
    : quit(false)
    , showedVersion(false)
    , disableOpenGL(false)
{
    option<&CommandLineHandler::showVersion>(
                QLatin1Char('v'),
                QLatin1String("--version"),
                QLatin1String("Display the version"));

    option<&CommandLineHandler::justQuit>(
                QChar(),
                QLatin1String("--quit"),
                QLatin1String("Only check validity of arguments, "
                              "don't actually load any files"));

    option<&CommandLineHandler::setDisableOpenGL>(
                QChar(),
                QLatin1String("--disable-opengl"),
                QLatin1String("Disable hardware accelerated rendering"));
}

void CommandLineHandler::showVersion()
{
    if (!showedVersion) {
        showedVersion = true;
        qWarning() << "Tiled (Qt) Map Editor"
                   << qPrintable(QApplication::applicationVersion());
        quit = true;
    }
}

void CommandLineHandler::justQuit()
{
    quit = true;
}

void CommandLineHandler::setDisableOpenGL()
{
    disableOpenGL = true;
}

#if !defined(QT_NO_DEBUG) && defined(ZOMBOID) && defined(_MSC_VER)
static void __cdecl invalid_parameter_handler(
   const wchar_t * expression,
   const wchar_t * function,
   const wchar_t * file,
   unsigned int line,
   uintptr_t pReserved)
{
    qDebug() << expression << function << file << line;
}

#endif

static QString tr(const char *s)
{
    return BuildingEditorWindow::instance()->tr(s);
}

static bool InitConfigFiles()
{
    PROGRESS progress(tr("Loading config files"), BuildingEditorWindow::instance());

    // Refresh the ui before blocking while loading tilesets etc
    qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

    // Create ~/.TileZed if needed.
    QString configPath = Preferences::instance()->configPath();
    QDir dir(configPath);
    if (!dir.exists()) {
        if (!dir.mkpath(configPath)) {
            QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                                  tr("Failed to create config directory:\n%1")
                                  .arg(QDir::toNativeSeparators(configPath)));
            return false;
        }
    }

    // Copy config files from the application directory to ~/.TileZed if they
    // don't exist there.
    QStringList configFiles;
    configFiles += TileMetaInfoMgr::instance()->txtName();
    configFiles += BuildingTemplates::instance()->txtName();
    configFiles += BuildingTilesMgr::instance()->txtName();
    configFiles += BuildingTMX::instance()->txtName();
    configFiles += FurnitureGroups::instance()->txtName();
    configFiles += TextureMgr::instance().txtName();
    configFiles += VirtualTilesetMgr::instance().txtName();
    configFiles += QLatin1String("TileShapes.txt");

    foreach (QString configFile, configFiles) {
        QString fileName = configPath + QLatin1Char('/') + configFile;
        if (!QFileInfo(fileName).exists()) {
            QString source = Preferences::instance()->appConfigPath(configFile);
            if (QFileInfo(source).exists()) {
                if (!QFile::copy(source, fileName)) {
                    QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                                          tr("Failed to copy file:\nFrom: %1\nTo: %2")
                                          .arg(source).arg(fileName));
                    return false;
                }
            }
        }
    }

    // Read Tilesets.txt before TMXConfig.txt in case we are upgrading
    // TMXConfig.txt from VERSION0 to VERSION1.
    if (!TileMetaInfoMgr::instance()->readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("%1\n(while reading %2)")
                              .arg(TileMetaInfoMgr::instance()->errorString())
                              .arg(TileMetaInfoMgr::instance()->txtName()));
        return false;
    }

    if (!BuildingTMX::instance()->readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(BuildingTMX::instance()->txtName())
                              .arg(BuildingTMX::instance()->errorString()));
        return false;
    }

    if (!BuildingTilesMgr::instance()->readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(BuildingTilesMgr::instance()->txtName())
                              .arg(BuildingTilesMgr::instance()->errorString()));
        return false;
    }

    if (!FurnitureGroups::instance()->readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(FurnitureGroups::instance()->txtName())
                              .arg(FurnitureGroups::instance()->errorString()));
        return false;
    }

    if (!BuildingTemplates::instance()->readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(BuildingTemplates::instance()->txtName())
                              .arg(BuildingTemplates::instance()->errorString()));
        return false;
    }

    if (!TextureMgr::instance().readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(TextureMgr::instance().txtName())
                              .arg(TextureMgr::instance().errorString()));
        return false;
    }

    if (!VirtualTilesetMgr::instance().readTxt()) {
        QMessageBox::critical(BuildingEditorWindow::instance(), tr("It's no good, Jim!"),
                              tr("Error while reading %1\n%2")
                              .arg(VirtualTilesetMgr::instance().txtName())
                              .arg(VirtualTilesetMgr::instance().errorString()));
        return false;
    }

    return true;
}

int main(int argc, char *argv[])
{
#if !defined(QT_NO_DEBUG) && defined(ZOMBOID) && defined(_MSC_VER)
    _set_invalid_parameter_handler(invalid_parameter_handler);
#endif

    /*
     * On X11, Tiled uses the 'raster' graphics system by default, because the
     * X11 native graphics system has performance problems with drawing the
     * tile grid.
     */
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QLatin1String("raster"));
#endif

    TiledApplication a(argc, argv);

    Q_INIT_RESOURCE(buildingeditor);

    a.setOrganizationDomain(QLatin1String("mapeditor.org"));
    a.setApplicationName(QLatin1String("Tiled"));
#ifdef BUILD_INFO_VERSION
    a.setApplicationVersion(QLatin1String(AS_STRING(BUILD_INFO_VERSION)));
#else
    a.setApplicationVersion(QLatin1String("0.8.1"));
#endif

#ifdef Q_WS_MAC
    a.setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

    LanguageManager *languageManager = LanguageManager::instance();
    languageManager->installTranslators();

    CommandLineHandler commandLine;

    if (!commandLine.parse(QCoreApplication::arguments()))
        return 0;
    if (commandLine.quit)
        return 0;
    if (commandLine.disableOpenGL)
        Preferences::instance()->setUseOpenGL(false);

    if (a.isRunning()) {
        if (!commandLine.filesToOpen().isEmpty()) {
            foreach (const QString &fileName, commandLine.filesToOpen())
                a.sendMessage(fileName);
            return 0;
        }
    }

    BuildingEditorWindow w;
    ZProgressManager::instance()->setMainWindow(&w);

    new TextureMgr;
    new VirtualTilesetMgr;

    w.show();

    a.setActivationWindow(&w);
    w.connect(&a, SIGNAL(messageReceived(QString)), SLOT(openFile(QString)));
    w.readSettings();

    if (!InitConfigFiles())
        return 0;

    QObject::connect(&a, SIGNAL(fileOpenRequest(QString)),
                     &w, SLOT(openFile(QString)));

    w.Startup();

    if (!commandLine.filesToOpen().isEmpty()) {
        gStartupBlockRendering = false;
        foreach (const QString &fileName, commandLine.filesToOpen())
            w.openFile(fileName);
    } else {
//        w.openLastFiles();
    }

    return a.exec();
}
