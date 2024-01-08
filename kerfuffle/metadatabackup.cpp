/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "metadatabackup.h"

namespace Kerfuffle
{

MetadataBackup::MetadataBackup(const QString &filePath)
{
    KFileMetaData::UserMetaData metaData(filePath);
    m_tags = metaData.tags();
    m_rating = metaData.rating();
    m_comment = metaData.userComment();
}

const QStringList MetadataBackup::tags() const
{
    return m_tags;
}

int MetadataBackup::rating() const
{
    return m_rating;
}

const QString MetadataBackup::comment() const
{
    return m_comment;
}

void MetadataBackup::restore(const QString &filePath)
{
    KFileMetaData::UserMetaData metaData(filePath);
    metaData.setTags(m_tags);
    metaData.setRating(m_rating);
    metaData.setUserComment(m_comment);
}

} // namespace Kerfuffle
