/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "archivesortfiltermodel.h"
#include "archiveentry.h"
#include "archivemodel.h"

using namespace Kerfuffle;

namespace
{
/**
 * Performs a natural string comparison.
 * This function compares strings in a way that is similar to natural human sorting order.
 * It is adapted from the Dolphin KFileItemModel::stringCompare implementation.
 *
 * @note Consider refactoring this logic to a framework-level utility
 * for improved maintainability and reusability across multiple projects.
 *
 * @return Integer less than, equal to, or greater than zero if the first argument is
 * found, respectively, to be less than, to match, or be greater than the second.
 */
int naturalStringCompare(const QString &a, const QString &b, const QCollator &collator)
{
    // Split extension, taking into account it can be empty
    constexpr QString::SectionFlags flags = QString::SectionSkipEmpty | QString::SectionIncludeLeadingSep;

    // Sort by baseName first
    const QString aBaseName = a.section(QLatin1Char('.'), 0, 0, flags);
    const QString bBaseName = b.section(QLatin1Char('.'), 0, 0, flags);

    const int res = collator.compare(aBaseName, bBaseName);
    if (res != 0 || (aBaseName.length() == a.length() && bBaseName.length() == b.length())) {
        return res;
    }

    // sliced() has undefined behavior when pos < 0 or pos > size().
    Q_ASSERT(aBaseName.length() <= a.length() && aBaseName.length() >= 0);
    Q_ASSERT(bBaseName.length() <= b.length() && bBaseName.length() >= 0);

    // baseNames were equal, sort by extension
    return collator.compare(a.sliced(aBaseName.length()), b.sliced(bBaseName.length()));
}
}

ArchiveSortFilterModel::ArchiveSortFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    // always enable recursive fitlering
    setRecursiveFilteringEnabled(true);
    m_collator.setNumericMode(true);
}

ArchiveSortFilterModel::~ArchiveSortFilterModel()
{
}

bool ArchiveSortFilterModel::lessThan(const QModelIndex &leftIndex, const QModelIndex &rightIndex) const
{
    ArchiveModel *srcModel = qobject_cast<ArchiveModel *>(sourceModel());
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
        case DisplayName: {
            const auto leftFullName = left->property(property.constData()).toString();
            const auto rightFullName = right->property(property.constData()).toString();
            return naturalStringCompare(leftFullName, rightFullName, m_collator) < 0;
        }
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

#include "moc_archivesortfiltermodel.cpp"
