/*
 * Copyright 2013, Tim Baker <treectrl@users.sf.net>
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

#include "addtilesetsdialog.h"
#include "ui_addtilesetsdialog.h"

#include "preferences.h"
#include "virtualtileset.h"

#include <QDir>
#include <QFileDialog>
#include <QImageReader>

using namespace Tiled::Internal;

AddTilesetsDialog::AddTilesetsDialog(const QString &dir,
                                     const QStringList &ignore,
                                     bool ignoreIsPaths, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddTilesetsDialog),
    mDirectory(dir),
    mIgnore(ignore),
    mIgnoreIsPaths(ignoreIsPaths)
{
    ui->setupUi(this);

    connect(ui->checkAll, SIGNAL(clicked()), SLOT(checkAll()));
    connect(ui->uncheckAll, SIGNAL(clicked()), SLOT(uncheckAll()));

    connect(ui->vCheckAll, SIGNAL(clicked()), SLOT(vCheckAll()));
    connect(ui->vUncheckAll, SIGNAL(clicked()), SLOT(vUncheckAll()));

    setPrompt(QString());
    setAllowBrowse(false);

    setFilesList();
    setVirtualsList();
}

AddTilesetsDialog::~AddTilesetsDialog()
{
    delete ui;
}

void AddTilesetsDialog::setAllowBrowse(bool browse)
{
    if (browse) {
        ui->path->show();
        ui->browse->show();
        ui->path->setText(QDir::toNativeSeparators(mDirectory));
        connect(ui->browse, SIGNAL(clicked()), SLOT(browse()));
    } else {
        ui->path->hide();
        ui->browse->hide();
    }
}

void AddTilesetsDialog::setPrompt(const QString &prompt)
{
    if (prompt.isEmpty())
        ui->prompt->hide();
    else
        ui->prompt->setText(prompt);
}

QStringList AddTilesetsDialog::fileNames()
{
    QStringList ret;
    for (int i = 0; i < ui->files->count(); i++) {
        QListWidgetItem *item = ui->files->item(i);
        if (item->checkState() == Qt::Checked) {
            QString fileName = QDir(mDirectory).filePath(item->text());
            ret += QFileInfo(fileName).canonicalFilePath();
        }
    }
    for (int i = 0; i < ui->virtualList->count(); i++) {
        QListWidgetItem *item = ui->virtualList->item(i);
        if (item->checkState() == Qt::Checked) {
            if (VirtualTileset *vts = VirtualTilesetMgr::instance().tileset(item->text()))
                ret += VirtualTilesetMgr::instance().imageSource(vts);
        }
    }
    return ret;
}

void AddTilesetsDialog::setFilesList()
{
    ui->files->clear();

    QDir dir(mDirectory);
    dir.setFilter(QDir::Files);
    dir.setSorting(QDir::Name);
    QStringList nameFilters;
    foreach (QByteArray format, QImageReader::supportedImageFormats())
        nameFilters += QLatin1String("*.") + QString::fromLatin1(format);

    QFileInfoList fileInfoList = dir.entryInfoList(nameFilters);
    foreach (QFileInfo fileInfo, fileInfoList) {
        QString fileName = fileInfo.fileName();
        if (mIgnoreIsPaths) {
            bool ignore = false;
            foreach (const QString &path, mIgnore) {
                // 'path' may not exist but that's ok here
                if (QFileInfo(path) == fileInfo) {
                    ignore = true;
                    break;
                }
            }
            if (ignore)
                continue;
        } else {
            if (mIgnore.contains(fileInfo.completeBaseName()))
                continue;
        }
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(fileName);
        item->setCheckState(Qt::Unchecked);
        ui->files->addItem(item);
    }
}

void AddTilesetsDialog::setVirtualsList()
{
    ui->virtualList->clear();

    foreach (VirtualTileset *vts, VirtualTilesetMgr::instance().tilesets()) {
        if (mIgnoreIsPaths) {
            bool ignore = false;
            foreach (const QString &path, mIgnore) {
                // 'path' may not exist but that's ok here
                if (path == VirtualTilesetMgr::instance().imageSource(vts)) {
                    ignore = true;
                    break;
                }
            }
            if (ignore)
                continue;
        } else {
            if (mIgnore.contains(vts->name()))
                continue;
        }
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(vts->name());
        item->setCheckState(Qt::Unchecked);
        ui->virtualList->addItem(item);
    }
}

void AddTilesetsDialog::browse()
{
    QString f = QFileDialog::getExistingDirectory(this, QString(),
                                                  ui->path->text());
    if (!f.isEmpty()) {
        mDirectory = f;
        ui->path->setText(QDir::toNativeSeparators(f));
        setFilesList();
    }
}

void AddTilesetsDialog::checkAll()
{
    for (int i = 0; i < ui->files->count(); i++)
        ui->files->item(i)->setCheckState(Qt::Checked);
}

void AddTilesetsDialog::uncheckAll()
{
    for (int i = 0; i < ui->files->count(); i++)
        ui->files->item(i)->setCheckState(Qt::Unchecked);
}

void AddTilesetsDialog::vCheckAll()
{
    for (int i = 0; i < ui->virtualList->count(); i++)
        ui->virtualList->item(i)->setCheckState(Qt::Checked);
}

void AddTilesetsDialog::vUncheckAll()
{
    for (int i = 0; i < ui->virtualList->count(); i++)
        ui->virtualList->item(i)->setCheckState(Qt::Unchecked);
}

void AddTilesetsDialog::accept()
{
    QDialog::accept();
}
