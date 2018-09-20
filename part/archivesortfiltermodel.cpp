/*
 * ark -- archiver for the KDE project
 *
 * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "archivesortfiltermodel.h"
#include "archiveentry.h"
#include "archivemodel.h"

using namespace Kerfuffle;

ArchiveSortFilterModel::ArchiveSortFilterModel(QObject *parent)
    : KRecursiveFilterProxyModel(parent)
{
}

ArchiveSortFilterModel::~ArchiveSortFilterModel()
{
}

bool ArchiveSortFilterModel::lessThan(const QModelIndex &leftIndex,
                                      const QModelIndex &rightIndex) const
{
    ArchiveModel *srcModel = qobject_cast<ArchiveModel*>(sourceModel());
    const int col = srcModel->shownColumns().at(leftIndex.column());
    const QByteArray property = srcModel->propertiesMap().value(col);

    const Archive::Entry *left = srcModel->entryForIndex(leftIndex);
    const Archive::Entry *right = srcModel->entryForIndex(rightIndex);

    if (left->isDir() && !right->isDir()) {
        return true;
    } else if (!left->isDir() && right->isDir()) {
        return false;
    } else {
        switch (col) {
        case Size:
        case CompressedSize:
            if (left->property(property.constData()).toULongLong() < right->property(property.constData()).toULongLong()) {
                return true;
            }
            break;
        default:
            if (left->property(property.constData()).toString() < right->property(property.constData()).toString()) {
                return true;
            }
        }
    }
    return false;
}
