/*
 * tileset.cpp
 * Copyright 2008-2009, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 * Copyrigth 2009, Edward Hutchins <eah1@yahoo.com>
 *
 * This file is part of libtiled.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "tileset.h"
#include "tile.h"

#include <QBitmap>

using namespace Tiled;

Tileset::~Tileset()
{
    qDeleteAll(mTiles);
    qDeleteAll(mTiles2x);
}

Tile *Tileset::tileAt(int id) const
{
    return (id < mTiles.size()) ? mTiles.at(id) : 0;
}

bool Tileset::loadFromImage(const QImage &image, const QString &fileName)
{
    Q_ASSERT(mTileWidth > 0 && mTileHeight > 0);

    if (image.isNull())
        return false;

    const int stopWidth = image.width() - mTileWidth;
    const int stopHeight = image.height() - mTileHeight;

    int oldTilesetSize = mTiles.size();
    int tileNum = 0;
#ifdef ZOMBOID
    QImage image2 = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
#endif
    for (int y = mMargin; y <= stopHeight; y += mTileHeight + mTileSpacing) {
        for (int x = mMargin; x <= stopWidth; x += mTileWidth + mTileSpacing) {
#ifdef ZOMBOID
            QImage tileImage = image2.copy(x, y, mTileWidth, mTileHeight);

            if (mTransparentColor.isValid()) {
                for (int x = 0; x < mTileWidth; x++) {
                    for (int y = 0; y < mTileHeight; y++) {
                        if (tileImage.pixel(x, y) == mTransparentColor.rgba())
                            tileImage.setPixel(x, y, qRgba(0,0,0,0));
                    }
                }
            }

            if (tileNum < oldTilesetSize) {
                mTiles.at(tileNum)->setImage(tileImage);
            } else {
                mTiles.append(new Tile(tileImage, tileNum, this));
            }
#else
            const QImage tileImage = image.copy(x, y, mTileWidth, mTileHeight);
            QPixmap tilePixmap = QPixmap::fromImage(tileImage);

            if (mTransparentColor.isValid()) {
                const QImage mask =
                        tileImage.createMaskFromColor(mTransparentColor.rgb());
                tilePixmap.setMask(QBitmap::fromImage(mask));
            }

            if (tileNum < oldTilesetSize) {
                mTiles.at(tileNum)->setImage(tilePixmap);
            } else {
                mTiles.append(new Tile(tilePixmap, tileNum, this));
            }
#endif
            ++tileNum;
        }
    }

    // Blank out any remaining tiles to avoid confusion
    while (tileNum < oldTilesetSize) {
#ifdef ZOMBOID
        QImage tileImage = QImage(mTileWidth, mTileHeight, QImage::Format_RGB16);
        tileImage.fill(Qt::white);
        mTiles.at(tileNum)->setImage(tileImage);
#else
        QPixmap tilePixmap = QPixmap(mTileWidth, mTileHeight);
        tilePixmap.fill();
        mTiles.at(tileNum)->setImage(tilePixmap);
#endif
        ++tileNum;
    }

    mImageWidth = image.width();
    mImageHeight = image.height();
    mColumnCount = columnCountForWidth(mImageWidth);
#ifdef ZOMBOID
    mLoaded = true;
#endif
    mImageSource = fileName;
    return true;
}

#ifdef ZOMBOID
bool Tileset::loadFromCache(Tileset *cached)
{
    Q_ASSERT(mTileWidth == cached->tileWidth() && mTileHeight == cached->tileHeight());
    Q_ASSERT(mTileSpacing == cached->tileSpacing());
    Q_ASSERT(mMargin == cached->margin());
    Q_ASSERT(mTransparentColor == cached->transparentColor());

    int oldTilesetSize = mTiles.size();
    int tileNum = 0;

    for (; tileNum < cached->tileCount(); ++tileNum) {
        QImage tileImage = cached->tileAt(tileNum)->image();

        if (tileNum < oldTilesetSize) {
            mTiles.at(tileNum)->setImage(tileImage);
        } else {
            mTiles.append(new Tile(tileImage, tileNum, this));
        }
    }

    // Blank out any remaining tiles to avoid confusion
    while (tileNum < oldTilesetSize) {
        QImage tileImage = QImage(mTileWidth, mTileHeight, QImage::Format_RGB16);
        tileImage.fill(Qt::white);
        mTiles.at(tileNum)->setImage(tileImage);
        ++tileNum;
    }

    oldTilesetSize = mTiles2x.size();
    for (tileNum = 0; tileNum < cached->mTiles2x.size(); ++tileNum) {
        Tile *tile = cached->mTiles2x.at(tileNum);
        if (tileNum < oldTilesetSize)
            mTiles2x.at(tileNum)->setImage(tile->image());
        else
            mTiles2x += new Tile(tile->image(), tileNum, this);
    }
    for (; tileNum < oldTilesetSize; tileNum++) {
        mTiles2x.removeLast();
//        Tile *tile = mTiles2x.at(tileNum);
//        QImage tileImage = QImage(tile->width(), tile->height(), QImage::Format_RGB16);
//        tileImage.fill(Qt::white);
//        tile->setImage(tileImage);
    }

    mImageWidth = cached->imageWidth();
    mImageHeight = cached->imageHeight();
    mColumnCount = columnCountForWidth(mImageWidth);
    mImageSource = cached->imageSource();
    mLoaded = true;
    return true;
}

bool Tileset::loadFromNothing(const QSize &imageSize, const QString &fileName)
{
    Q_ASSERT(mTileWidth > 0 && mTileHeight > 0);

    if (imageSize.isEmpty())
        return false;

    const int stopWidth = imageSize.width() - mTileWidth;
    const int stopHeight = imageSize.height() - mTileHeight;

    int oldTilesetSize = mTiles.size();
    int tileNum = 0;

    QImage tileImage = QImage(mTileWidth, mTileHeight, QImage::Format_RGB16);
    tileImage.fill(Qt::white);

    for (int y = mMargin; y <= stopHeight; y += mTileHeight + mTileSpacing) {
        for (int x = mMargin; x <= stopWidth; x += mTileWidth + mTileSpacing) {
            if (tileNum < oldTilesetSize) {
                mTiles.at(tileNum)->setImage(tileImage);
            } else {
                mTiles.append(new Tile(tileImage, tileNum, this));
            }
            ++tileNum;
        }
    }

    // Blank out any remaining tiles to avoid confusion
    while (tileNum < oldTilesetSize) {
        mTiles.at(tileNum)->setImage(tileImage);
        ++tileNum;
    }

    mImageWidth = imageSize.width();
    mImageHeight = imageSize.height();
    mColumnCount = columnCountForWidth(mImageWidth);
    mImageSource = fileName;
//    mLoaded = true;
    return true;
}

bool Tileset::loadImage2x(const QImage &image)
{
    Q_ASSERT(mTileWidth > 0 && mTileHeight > 0);

    if (image.isNull())
        return false;

    const int mTileWidth = this->mTileWidth * 2;
    const int mTileHeight = this->mTileHeight * 2;

    const int stopWidth = image.width() - mTileWidth;
    const int stopHeight = image.height() - mTileHeight;

    QImage image2 = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);

    int oldTilesetSize = mTiles2x.size();
    int tileNum = 0;
    for (int y = mMargin; y <= stopHeight; y += mTileHeight + mTileSpacing) {
        for (int x = mMargin; x <= stopWidth; x += mTileWidth + mTileSpacing) {
            QImage tileImage = image2.copy(x, y, mTileWidth, mTileHeight);

            if (mTransparentColor.isValid()) {
                for (int x = 0; x < mTileWidth; x++) {
                    for (int y = 0; y < mTileHeight; y++) {
                        if (tileImage.pixel(x, y) == mTransparentColor.rgba())
                            tileImage.setPixel(x, y, qRgba(0,0,0,0));
                    }
                }
            }

            if (tileNum < oldTilesetSize) {
                mTiles2x.at(tileNum)->setImage(tileImage);
            } else {
                mTiles2x.append(new Tile(tileImage, tileNum, this));
            }
            ++tileNum;
        }
    }

    // Blank out any remaining tiles to avoid confusion

    while (tileNum < oldTilesetSize) {
        mTiles2x.removeLast();
//        QImage tileImage = QImage(mTileWidth, mTileHeight, QImage::Format_RGB16);
//        tileImage.fill(Qt::white);
//        mTiles2x.at(tileNum)->setImage(tileImage);
        ++tileNum;
    }

    return true;
}
#endif // ZOMBOID

Tileset *Tileset::findSimilarTileset(const QList<Tileset*> &tilesets) const
{
    foreach (Tileset *candidate, tilesets) {
        if (candidate != this
            && candidate->imageSource() == imageSource()
            && candidate->tileWidth() == tileWidth()
            && candidate->tileHeight() == tileHeight()
            && candidate->tileSpacing() == tileSpacing()
            && candidate->margin() == margin()) {
                return candidate;
        }
    }
    return 0;
}

int Tileset::columnCountForWidth(int width) const
{
    Q_ASSERT(mTileWidth > 0);
    return (width - mMargin + mTileSpacing) / (mTileWidth + mTileSpacing);
}

#ifdef ZOMBOID
Tileset *Tileset::clone() const
{
    Tileset *clone = new Tileset(*this);

    for (int i = 0; i < clone->mTiles.size(); i++) {
        clone->mTiles[i] = new Tile(mTiles[i]->image(), i, clone);
        clone->mTiles[i]->mergeProperties(mTiles[i]->properties());
    }

    for (int i = 0; i < clone->mTiles2x.size(); i++) {
        clone->mTiles2x[i] = new Tile(mTiles2x[i]->image(), i, clone);
    }

    return clone;
}

TilesetImageCache::~TilesetImageCache()
{
    qDeleteAll(mTilesets);
}

#include <QDebug>

Tileset *TilesetImageCache::addTileset(Tileset *ts)
{
    Tileset *cached = new Tileset(QLatin1String("cached"), ts->tileWidth(), ts->tileHeight(), ts->tileSpacing(), ts->margin());
    cached->mTransparentColor = ts->transparentColor();
    cached->mImageSource = ts->imageSource(); // FIXME: make canonical
    cached->mTiles.reserve(ts->tileCount());
    cached->mImageWidth = ts->imageWidth();
    cached->mImageHeight = ts->imageHeight();
    cached->mColumnCount = ts->columnCount();

    for (int tileNum = 0; tileNum < ts->tileCount(); ++tileNum) {
        Tile *tile = ts->tileAt(tileNum);
        cached->mTiles.append(new Tile(tile->image(), tileNum, cached));
    }

    cached->mTiles2x.reserve(ts->mTiles2x.size());
    for (int tileNum = 0; tileNum < ts->mTiles2x.size(); ++tileNum) {
        Tile *tile = ts->mTiles2x.at(tileNum);
        cached->mTiles2x += new Tile(tile->image(), tileNum, cached);
    }

    mTilesets.append(cached);

//    qDebug() << "added tileset image " << ts->imageSource() << " to cache";

    return cached;
}

Tileset *TilesetImageCache::findMatch(Tileset *ts, const QString &imageSource)
{
    foreach (Tileset *candidate, mTilesets) {
        if (candidate->imageSource() == imageSource
                && candidate->tileWidth() == ts->tileWidth()
                && candidate->tileHeight() == ts->tileHeight()
                && candidate->tileSpacing() == ts->tileSpacing()
                && candidate->margin() == ts->margin()
                && candidate->transparentColor() == ts->transparentColor()) {
//            qDebug() << "retrieved tileset image " << candidate->imageSource() << " from cache";
            return candidate;
        }
    }
    return NULL;
}

#endif
