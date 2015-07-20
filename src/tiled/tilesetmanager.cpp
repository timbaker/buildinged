/*
 * tilesetmanager.cpp
 * Copyright 2008-2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyright 2009, Edward Hutchins <eah1@yahoo.com>
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

#include "tilesetmanager.h"

#include "filesystemwatcher.h"
#include "tileset.h"

#include <QImage>
#ifdef ZOMBOID
#include "preferences.h"
#include "virtualtileset.h"
#include "tile.h"
#include <QDebug>
#include <QDir>
#include <QImageReader>
#include <QMetaType>
#endif

using namespace Tiled;
using namespace Tiled::Internal;

TilesetManager *TilesetManager::mInstance = 0;

TilesetManager::TilesetManager():
#ifdef ZOMBOID
    mTilesetImageCache(new TilesetImageCache),
#endif
    mWatcher(new FileSystemWatcher(this)),
    mReloadTilesetsOnChange(false)
{
#ifdef ZOMBOID
    const int TILE_WIDTH = 64;
    const int TILE_HEIGHT = 128;

    mMissingTileset = new Tileset(QLatin1String("missing"), TILE_WIDTH, TILE_HEIGHT);
    mMissingTileset->setTransparentColor(Qt::white);
    mMissingTileset->setMissing(true);
    QString fileName = QLatin1String(":/BuildingEditor/icons/missing-tile.png");
    if (!mMissingTileset->loadFromImage(QImage(fileName), fileName)) {
        QImage image(TILE_WIDTH, TILE_HEIGHT, QImage::Format_ARGB32);
        image.fill(Qt::red);
        mMissingTileset->loadFromImage(image, fileName);
    }
    mMissingTile = mMissingTileset->tileAt(0);
    mTilesets.insert(mMissingTileset, 1); //addReference(mMissingTileset);

    mNoBlendTileset = new Tileset(QLatin1String("noblend"), TILE_WIDTH, TILE_HEIGHT);
    mNoBlendTileset->setTransparentColor(Qt::white);
    mNoBlendTileset->setMissing(true);
    fileName = QLatin1String(":/images/noblend.png");
    if (!mNoBlendTileset->loadFromImage(QImage(fileName), fileName)) {
        QImage image(TILE_WIDTH, TILE_HEIGHT, QImage::Format_ARGB32);
        image.fill(Qt::red);
        mNoBlendTileset->loadFromImage(image, fileName);
    }
    mNoBlendTile = mNoBlendTileset->tileAt(0);
    mTilesets.insert(mNoBlendTileset, 1); //addReference(mNoBlendTileset);

    qRegisterMetaType<Tileset*>("Tileset*");

    mImageReaderThreads.resize(8);
    mImageReaderWorkers.resize(mImageReaderThreads.size());
    mNextThreadForJob = 0;
    for (int i = 0; i < mImageReaderWorkers.size(); i++) {
        mImageReaderThreads[i] = new InterruptibleThread;
        mImageReaderWorkers[i] = new TilesetImageReaderWorker(i, mImageReaderThreads[i]);
        mImageReaderWorkers[i]->moveToThread(mImageReaderThreads[i]);
        connect(mImageReaderWorkers[i], SIGNAL(imageLoaded(QImage*,QImage*,Tiled::Tileset*)),
                SLOT(imageLoaded(QImage*,QImage*,Tiled::Tileset*)));
        mImageReaderThreads[i]->start();
    }

    mReloadTilesetsOnChange = Preferences::instance()->reloadTilesetsOnChange();

    // Changing this setting doesn't take effect until TileZed is restarted.
    mUseVirtualTilesets = Preferences::instance()->useVirtualTilesets();
#endif

    connect(mWatcher, SIGNAL(fileChanged(QString)),
            this, SLOT(fileChanged(QString)));

    mChangedFilesTimer.setInterval(500);
    mChangedFilesTimer.setSingleShot(true);

    connect(&mChangedFilesTimer, SIGNAL(timeout()),
            this, SLOT(fileChangedTimeout()));
}

TilesetManager::~TilesetManager()
{
#ifdef ZOMBOID
    removeReference(mMissingTileset);
    removeReference(mNoBlendTileset);
    for (int i = 0; i < mImageReaderThreads.size(); i++) {
        mImageReaderThreads[i]->interrupt();
        mImageReaderThreads[i]->quit();
        mImageReaderThreads[i]->wait();
        delete mImageReaderWorkers[i];
        delete mImageReaderThreads[i];
    }

    delete mTilesetImageCache;
#endif

    // Since all MapDocuments should be deleted first, we assert that there are
    // no remaining tileset references.
    Q_ASSERT(mTilesets.size() == 0);

#ifdef ZOMBOID
    foreach (ZTileLayerNames *tln, mTileLayerNames)
        writeTileLayerNames(tln);
#endif
}

TilesetManager *TilesetManager::instance()
{
    if (!mInstance)
        mInstance = new TilesetManager;

    return mInstance;
}

void TilesetManager::deleteInstance()
{
    delete mInstance;
    mInstance = 0;
}

Tileset *TilesetManager::findTileset(const QString &fileName) const
{
    foreach (Tileset *tileset, tilesets())
        if (tileset->fileName() == fileName)
            return tileset;

    return 0;
}

Tileset *TilesetManager::findTileset(const TilesetSpec &spec) const
{
    foreach (Tileset *tileset, tilesets()) {
        if (tileset->imageSource() == spec.imageSource
            && tileset->tileWidth() == spec.tileWidth
            && tileset->tileHeight() == spec.tileHeight
            && tileset->tileSpacing() == spec.tileSpacing
            && tileset->margin() == spec.margin)
        {
            return tileset;
        }
    }

    return 0;
}

void TilesetManager::addReference(Tileset *tileset)
{
    if (mTilesets.contains(tileset)) {
        mTilesets[tileset]++;
    } else {
        mTilesets.insert(tileset, 1);
#ifdef ZOMBOID
#else
        if (!tileset->imageSource().isEmpty())
            mWatcher->addPath(tileset->imageSource());
#endif
    }
#ifdef ZOMBOID
    if (!tileset->imageSource().isEmpty() && !tileset->isMissing())
        readTileLayerNames(tileset);
#endif

#ifdef ZOMBOID
    loadTileset(tileset, tileset->imageSource());
#endif
}

void TilesetManager::removeReference(Tileset *tileset)
{
    Q_ASSERT(mTilesets.value(tileset) > 0);
    mTilesets[tileset]--;

    if (mTilesets.value(tileset) == 0) {
        mTilesets.remove(tileset);
#ifdef ZOMBOID
#else
        if (!tileset->imageSource().isEmpty())
            mWatcher->removePath(tileset->imageSource());
#endif

        delete tileset;
    }
}

void TilesetManager::addReferences(const QList<Tileset*> &tilesets)
{
    foreach (Tileset *tileset, tilesets)
        addReference(tileset);
}

void TilesetManager::removeReferences(const QList<Tileset*> &tilesets)
{
    foreach (Tileset *tileset, tilesets)
        removeReference(tileset);
}

QList<Tileset*> TilesetManager::tilesets() const
{
    return mTilesets.keys();
}

void TilesetManager::setReloadTilesetsOnChange(bool enabled)
{
    mReloadTilesetsOnChange = enabled;
    // TODO: Clear the file system watcher when disabled
}

void TilesetManager::fileChanged(const QString &path)
{
#ifndef ZOMBOID
    if (!mReloadTilesetsOnChange)
        return;
#endif

    /*
     * Use a one-shot timer since GIMP (for example) seems to generate many
     * file changes during a save, and some of the intermediate attempts to
     * reload the tileset images actually fail (at least for .png files).
     */
    mChangedFiles.insert(path);
    mChangedFilesTimer.start();
}

void TilesetManager::fileChangedTimeout()
{
#ifdef ZOMBOID
    foreach (Tileset *tileset, mTilesetImageCache->mTilesets) {
        QString fileName = tileset->imageSource();
        if (mChangedFiles.contains(fileName)) {
            VirtualTileset *vts = useVirtualTilesets()
                    ? VirtualTilesetMgr::instance().tilesetFromPath(fileName)
                    : 0;
            if (vts) {
                tileset->loadFromImage(vts->image(), fileName);
                tileset->setMissing(false);
            } else if (QImageReader(fileName).size().isValid()) {
                tileset->loadFromImage(QImage(fileName), fileName);
                tileset->setMissing(false);
            } else {
                if (tileset->tileHeight() == mMissingTile->width() && tileset->tileWidth() == mMissingTile->height()) {
                    for (int i = 0; i < tileset->tileCount(); i++)
                        tileset->tileAt(i)->setImage(mMissingTile->image());
                }
                tileset->setMissing(true);
            }
        }
    }
    foreach (Tileset *tileset, tilesets()) {
        QString fileName = tileset->imageSource();
        if (mChangedFiles.contains(fileName)) {
            if (Tileset *cached = mTilesetImageCache->findMatch(tileset, fileName)) {
                if (tileset->loadFromCache(cached)) {
                    tileset->setMissing(cached->isMissing());
                    syncTileLayerNames(tileset);
                    emit tilesetChanged(tileset);
                }
            }
        }
    }

    // Perhaps a used virtual tileset didn't exist and now it does (fringe case to be sure).
    if (useVirtualTilesets()) {
        foreach (Tileset *ts, tilesets()) {
            if (ts->isMissing()) {
                QString imageSource = ts->imageSource();
                if (VirtualTileset *vts = VirtualTilesetMgr::instance().tilesetFromPath(imageSource)) {
                    VirtualTilesetMgr::instance().resolveImageSource(imageSource);
                    ts->loadFromImage(vts->image(), imageSource);
                    ts->setMissing(false);
                    if (mTilesetImageCache->findMatch(ts, imageSource) == 0)
                        mTilesetImageCache->addTileset(ts)->setLoaded(true);
                    emit tilesetChanged(ts);
                }
            }
        }
    }
#else

    foreach (Tileset *tileset, tilesets()) {
        QString fileName = tileset->imageSource();
        if (mChangedFiles.contains(fileName))
            if (tileset->loadFromImage(QImage(fileName), fileName))
#ifdef ZOMBOID
            {
                syncTileLayerNames(tileset);
                emit tilesetChanged(tileset);
            }
#else
                emit tilesetChanged(tileset);
#endif
    }
#endif

    mChangedFiles.clear();
}

#ifdef ZOMBOID
void TilesetManager::imageLoaded(QImage *image, QImage *image2x, Tileset *tileset)
{
    // Check if the tileset belongs to TextureMgr.
    // If so, the image is a "flat" texture, not an old iso tileset image.
    if (mTextureMgrTilesets.contains(tileset)) {
        emit textureImageLoaded(image, tileset);
        delete image;
        return;
    }

    Q_ASSERT(mTilesetImageCache->mTilesets.contains(tileset));

    // This updates a tileset in the cache.
    tileset->loadFromImage(*image, tileset->imageSource());

    if (image2x != 0) {
        tileset->loadImage2x(*image2x);
        delete image2x;
    } else {
        qDeleteAll(tileset->mTiles2x);
        tileset->mTiles2x.clear();
    }

    // Watch the image file for changes.
    mWatcher->addPath(tileset->imageSource());

    // Now update every tileset using this image.
    foreach (Tileset *candidate, tilesets()) {
        if (candidate->isLoaded())
            continue;
        if (candidate->imageSource() == tileset->imageSource()
                && candidate->tileWidth() == tileset->tileWidth()
                && candidate->tileHeight() == tileset->tileHeight()
                && candidate->tileSpacing() == tileset->tileSpacing()
                && candidate->margin() == tileset->margin()
                && candidate->transparentColor() == tileset->transparentColor()) {
            candidate->loadFromCache(tileset);
            candidate->setMissing(false);
            emit tilesetChanged(candidate);
        }
    }
    delete image;
}

void TilesetManager::virtualTilesetChanged(VirtualTileset *vts)
{
    QString imageSource = VirtualTilesetMgr::instance().imageSource(vts);
    fileChanged(imageSource);
}

// If imageSource is in the Tiles directory, make it relative to Tiles2x directory.
static bool resolveImageSource(QString &imageSource)
{
    QString tiles2xDir = Preferences::instance()->tiles2xDirectory();
    if (tiles2xDir.isEmpty())
        return false;
    QFileInfo tiles2xInfo(tiles2xDir);
    if (tiles2xInfo.isRelative() || !tiles2xInfo.exists())
        return false;
    tiles2xDir = tiles2xInfo.canonicalFilePath();

    if (imageSource.isEmpty())
        return false;
    QFileInfo info(imageSource);
    if (info.isRelative())
        return false;
    QString tilesDir = Preferences::instance()->tilesDirectory();
    if (tilesDir.isEmpty() || QFileInfo(tilesDir).isRelative())
        return false;
    // FIXME: use QFileInfo::operator==() if both exist
    if (QDir::cleanPath(tilesDir) == QDir::cleanPath(info.absolutePath())) {
        QString imageSource2x = QDir(tiles2xDir).filePath(info.fileName());
        if (QFileInfo(imageSource2x).exists()) {
            imageSource = imageSource2x;
            return true;
        }
    }
    return false;
}

void TilesetManager::loadTileset(Tileset *tileset, const QString &imageSource_)
{
    // Hack to ignore TileMetaInfoMgr's tilesets that haven't been loaded,
    // their paths are relative to the Tiles Directory.
    if (QDir(imageSource_).isRelative())
        return;

#if 1
    QString imageSource = imageSource_;
    if (useVirtualTilesets()) {
        // When reading a TMX, virtual tileset image paths won't be canonical,
        // since they're saved relative to the TMX's directory.
        VirtualTilesetMgr::instance().resolveImageSource(imageSource);
    }
#endif

    if (!tileset->isLoaded() /*&& !tileset->isMissing()*/) {
        VirtualTileset *vts = useVirtualTilesets()
                ? VirtualTilesetMgr::instance().tilesetFromPath(imageSource)
                : 0;
        if (Tileset *cached = mTilesetImageCache->findMatch(tileset, imageSource)) {
            // If it !isLoaded(), a thread is reading the image.
            // FIXME: 1) load TMX with tilesets from not-TilesDirectory -> no 2x images loaded
            //        2) switch TilesDirectory to the same not-TilesDirectory in 1)
            //        3) 2x images remain unloaded
            if (cached->isLoaded()) {
                tileset->loadFromCache(cached);
                tileset->setMissing(false);
                emit tilesetChanged(tileset);
            } else {
                changeTilesetSource(tileset, imageSource, false);
            }
        } else if (vts) {
            tileset->loadFromImage(vts->image(), imageSource);
            tileset->setMissing(false);
            mTilesetImageCache->addTileset(tileset)->setLoaded(true);
            emit tilesetChanged(tileset);
        } else if (QImageReader(imageSource).size().isValid()) {
            QString imageSource2x = imageSource;
            if (resolveImageSource(imageSource2x)) {
                qDebug() << "2x YES " << imageSource;
            } else {
                qDebug() << "2x NO " << imageSource;
                imageSource2x.clear();
            }
            changeTilesetSource(tileset, imageSource, false);
            cached = mTilesetImageCache->addTileset(tileset);
            QMetaObject::invokeMethod(mImageReaderWorkers[mNextThreadForJob],
                                      "addJob", Qt::QueuedConnection,
                                      Q_ARG(Tileset*,cached),
                                      Q_ARG(QString,imageSource2x));
            mNextThreadForJob = (mNextThreadForJob + 1) % mImageReaderWorkers.size();
        } else {
            if (tileset->tileHeight() == mMissingTile->height() && tileset->tileWidth() == mMissingTile->width()) {
                for (int i = 0; i < tileset->tileCount(); i++)
                    tileset->tileAt(i)->setImage(mMissingTile->image());
            }
            changeTilesetSource(tileset, imageSource, true);
        }
    }
}

#include <QPainter>
void TilesetManager::loadTextureTileset(Tileset *tileset, const QString &imageSource)
{
    Q_ASSERT(mTextureMgrTilesets.contains(tileset));
    tileset->setImageSource(imageSource);
    if (QImageReader(imageSource).size().isValid()) {
        tileset->setMissing(false);
        QMetaObject::invokeMethod(mImageReaderWorkers[mNextThreadForJob],
                                  "addJob", Qt::QueuedConnection,
                                  Q_ARG(Tileset*,tileset));
        mNextThreadForJob = (mNextThreadForJob + 1) % mImageReaderWorkers.size();
    } else {
        tileset->setMissing(true);
        tileset->setLoaded(true);
        QImage tileImg(tileset->tileWidth(), tileset->tileHeight(), QImage::Format_ARGB32);
        tileImg.fill(Qt::transparent);
        QPainter p(&tileImg);
        for (int y = 0; y < tileImg.height(); y += mMissingTile->height())
            for (int x = 0; x < tileImg.width(); x += mMissingTile->width())
                p.drawImage(x, y, mMissingTile->image());
        for (int i = 0; i < tileset->tileCount(); i++)
            tileset->tileAt(i)->setImage(tileImg);
    }
}

void TilesetManager::waitForTilesets(const QList<Tileset *> &tilesets)
{
    foreach (Tileset *ts, tilesets) {
        if (ts->isLoaded())
            continue;
        // Missing tilesets aren't in mTilesetImageCache
        if (ts->isMissing())
            continue;
        // There may be a thread already reading or about to read this image.
        QImage *image = new QImage(ts->imageSource());
        QImage *image2x = 0;
        QString image2xFileName = ts->imageSource();
        if (resolveImageSource(image2xFileName)) {
            qDebug() << "2x YES " << ts->imageSource();
            image2x = new QImage(image2xFileName);
        } else
            qDebug() << "2x NO " << ts->imageSource();
        Tileset *cached = mTilesetImageCache->findMatch(ts, ts->imageSource());
        Q_ASSERT(cached != 0 && !cached->isLoaded());
        if (cached) {
            imageLoaded(image, image2x, cached); // deletes image
        }
    }
}

void TilesetManager::changeTilesetSource(Tileset *tileset, const QString &source,
                                         bool missing)
{
    tileset->setImageSource(source);
    tileset->setMissing(missing);
    if (!tileset->imageSource().isEmpty() && !tileset->isMissing()) {
        readTileLayerNames(tileset);
    }
    tileset->setLoaded(false);
    if (missing)
        emit tilesetChanged(tileset);
}

#include "tile.h"

#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QXmlStreamReader>

namespace Tiled {
namespace Internal {

struct ZTileLayerName
{
    ZTileLayerName()
    {}
    QString mLayerName;
};

struct ZTileLayerNames
{
    ZTileLayerNames()
        : mColumns(0)
        , mRows(0)
        , mModified(false)
    {}
    ZTileLayerNames(const QString& filePath, int columns, int rows)
        : mColumns(columns)
        , mRows(rows)
        , mFilePath(filePath)
        , mModified(false)
    {
        mTiles.resize(columns * rows);
    }
    void enforceSize(int columns, int rows)
    {
        if (columns == mColumns && rows == mRows)
            return;
        if (columns == mColumns) {
            // Number of rows changed, tile ids are still valid
            mTiles.resize(columns * rows);
            return;
        }
        // Number of columns (and maybe rows) changed.
        // Copy over the preserved part.
        QRect oldBounds(0, 0, mColumns, mRows);
        QRect newBounds(0, 0, columns, rows);
        QRect preserved = oldBounds & newBounds;
        QVector<ZTileLayerName> tiles(columns * rows);
        for (int y = 0; y < preserved.height(); y++) {
            for (int x = 0; x < preserved.width(); x++) {
                tiles[y * columns + x] = mTiles[y * mColumns + x];
            }
        }
        mColumns = columns;
        mRows = rows;
        mTiles = tiles;
    }

    int mColumns;
    int mRows;
    QString mFilePath;
    QVector<ZTileLayerName> mTiles;
    bool mModified;
};

} // namespace Internal
} // namespace Tiled

void TilesetManager::setLayerName(Tile *tile, const QString &name)
{
    Tileset *ts = tile->tileset();
    if (ZTileLayerNames *tln = layerNamesForTileset(ts)) {
        // Prevent an error if two tilesets share the same image file but have
        // different tile dimensions.
        if ((tile->id() >= 0) && (tile->id() < tln->mRows * tln->mColumns)) {
            tln->mTiles[tile->id()].mLayerName = name;
            tln->mModified = true;
            emit tileLayerNameChanged(tile);
        }
    }
}

QString TilesetManager::layerName(Tile *tile)
{
    Tileset *ts = tile->tileset();
    if (mTileLayerNames.contains(ts->imageSource())) {
        ZTileLayerNames *tln =  mTileLayerNames[ts->imageSource()];
        // Prevent an error if two tilesets share the same image file but have
        // different tile dimensions.
        if ((tile->id() >= 0) && (tile->id() < tln->mRows * tln->mColumns))
            return tln->mTiles[tile->id()].mLayerName;
    }
    return QString();
}

class ZTileLayerNamesReader
{
public:
    bool read(const QString &filePath)
    {
        mError.clear();

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            mError = QCoreApplication::translate("TileLayerNames", "Could not open file.");
            return false;
        }

        xml.setDevice(&file);

        if (xml.readNextStartElement() && xml.name() == QLatin1String("tileset")) {
            mTLN.mFilePath = filePath;
            return readTileset();
        } else {
            mError = QCoreApplication::translate("TileLayerNames", "File doesn't contain <tilesets>.");
            return false;
        }
    }

    bool readTileset()
    {
        Q_ASSERT(xml.isStartElement() && xml.name() == QLatin1String("tileset"));

        const QXmlStreamAttributes atts = xml.attributes();
        const QString tilesetName = atts.value(QLatin1String("name")).toString();
        uint columns = atts.value(QLatin1String("columns")).toString().toUInt();
        uint rows = atts.value(QLatin1String("rows")).toString().toUInt();

        mTLN.mTiles.resize(columns * rows);

        mTLN.mColumns = columns;
        mTLN.mRows = rows;

        while (xml.readNextStartElement()) {
            if (xml.name() == QLatin1String("tile")) {
                const QXmlStreamAttributes atts = xml.attributes();
                uint id = atts.value(QLatin1String("id")).toString().toUInt();
                if (id >= columns * rows) {
                    qDebug() << "<tile> " << id << " out-of-bounds: ignored";
                } else {
                    const QString layerName(atts.value(QLatin1String("layername")).toString());
                    mTLN.mTiles[id].mLayerName = layerName;
                }
            }
            xml.skipCurrentElement();
        }

        return true;
    }

    QString errorString() const { return mError; }
    ZTileLayerNames &result() { return mTLN; }

private:
    QString mError;
    QXmlStreamReader xml;
    ZTileLayerNames mTLN;
};

class ZTileLayerNamesWriter
{
public:
    bool write(const ZTileLayerNames *tln)
    {
        mError.clear();

        QFile file(tln->mFilePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            mError = QCoreApplication::translate(
                        "TileLayerNames", "Could not open file for writing.");
            return false;
        }

        QXmlStreamWriter writer(&file);

        writer.setAutoFormatting(true);
        writer.setAutoFormattingIndent(1);

        writer.writeStartDocument();
        writer.writeStartElement(QLatin1String("tileset"));
        writer.writeAttribute(QLatin1String("columns"), QString::number(tln->mColumns));
        writer.writeAttribute(QLatin1String("rows"), QString::number(tln->mRows));

        int id = 0;
        foreach (const ZTileLayerName &tile, tln->mTiles) {
            if (!tile.mLayerName.isEmpty()) {
                writer.writeStartElement(QLatin1String("tile"));
                writer.writeAttribute(QLatin1String("id"), QString::number(id));
                writer.writeAttribute(QLatin1String("layername"), tile.mLayerName);
                writer.writeEndElement();
            }
            ++id;
        }

        writer.writeEndElement();
        writer.writeEndDocument();

        if (file.error() != QFile::NoError) {
            mError = file.errorString();
            return false;
        }

        return true;
    }

    QString errorString() const { return mError; }

private:
    QString mError;
};

QFileInfo TilesetManager::tileLayerNamesFile(Tileset *ts)
{
    QString imageSource = ts->imageSource();
    QFileInfo fileInfoImgSrc(imageSource);
    QDir dir = fileInfoImgSrc.absoluteDir();
    return QFileInfo(dir, fileInfoImgSrc.completeBaseName() + QLatin1String(".tilelayers.xml"));
}

ZTileLayerNames *TilesetManager::layerNamesForTileset(Tileset *ts)
{
    QString imageSource = ts->imageSource();
    QMap<QString,ZTileLayerNames*>::iterator it = mTileLayerNames.find(imageSource);
    if (it != mTileLayerNames.end())
        return *it;

    int columns = ts->columnCount();
    int rows = columns ? ts->tileCount() / columns : 0;

    QFileInfo fileInfo = tileLayerNamesFile(ts);
    QString filePath = fileInfo.absoluteFilePath();
    return mTileLayerNames[imageSource] = new ZTileLayerNames(filePath, columns, rows);
}

void TilesetManager::readTileLayerNames(Tileset *ts)
{
    QString imageSource = ts->imageSource();
    if (mTileLayerNames.contains(imageSource))
        return;

    int columns = ts->columnCount();
    int rows = columns ? ts->tileCount() / columns : 0;

    QFileInfo fileInfo = tileLayerNamesFile(ts);
    if (fileInfo.exists()) {
        QString filePath = fileInfo.absoluteFilePath();
//        qDebug() << "Reading: " << filePath;
        ZTileLayerNamesReader reader;
        if (reader.read(filePath)) {
            mTileLayerNames[imageSource] = new ZTileLayerNames(reader.result());
            // Handle the source image being resized
            mTileLayerNames[imageSource]->enforceSize(columns, rows);
        } else {
            QMessageBox::critical(0, tr("Error Reading Tile Layer Names"),
                                  filePath + QLatin1String("\n") + reader.errorString());
        }
    }
}

void TilesetManager::writeTileLayerNames(ZTileLayerNames *tln)
{
    if (tln->mModified == false)
        return;
//    qDebug() << "Writing: " << tln->mFilePath;
    ZTileLayerNamesWriter writer;
    if (writer.write(tln) == false) {
        QMessageBox::critical(0, tr("Error Writing Tile Layer Names"),
            tln->mFilePath + QLatin1String("\n") + writer.errorString());
    }
}

void TilesetManager::syncTileLayerNames(Tileset *ts)
{
    if (mTileLayerNames.contains(ts->imageSource())) {
        ZTileLayerNames *tln = mTileLayerNames[ts->imageSource()];
        int columns = ts->columnCount();
        int rows = columns ? ts->tileCount() / columns : 0;
        tln->enforceSize(columns, rows);
    }
}
#endif // ZOMBOID

#ifdef ZOMBOID
/////

TilesetImageReaderWorker::TilesetImageReaderWorker(int id, InterruptibleThread *thread) :
    BaseWorker(thread),
    mID(id),
    mWorkPending(false)
{
}

TilesetImageReaderWorker::~TilesetImageReaderWorker()
{
}

void TilesetImageReaderWorker::work()
{
    IN_WORKER_THREAD

    mWorkPending = false;

    while (mJobs.size()) {
        if (aborted()) {
            mJobs.clear();
            return;
        }

        Job job = mJobs.takeAt(0);

        QImage *image = new QImage(job.tileset->imageSource());
        QImage *image2x = 0;
        if (!job.imageSource2x.isEmpty())
            image2x = new QImage(job.imageSource2x);
#if 0
        Sleep::msleep(500);
//        qDebug() << "TilesetImageReaderThread #" << mID << "loaded" << job.tileset->imageSource();
#endif
        emit imageLoaded(image, image2x, job.tileset);
    }
}

void TilesetImageReaderWorker::addJob(Tileset *tileset, const QString &imageSource2x)
{
    IN_WORKER_THREAD

    mJobs += Job(tileset, imageSource2x);
    scheduleWork();
}
#endif // ZOMBOID
