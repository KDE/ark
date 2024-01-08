/*
    SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include <KFileMetaData/UserMetaData>

namespace Kerfuffle
{

/**
 * @class MetadataBackup
 * @brief A backup of the user metadata for an archive.
 *
 * When an archive is modified, its user metadata is removed. This class
 * holds a backup of the user metadata so that it can be restored after
 * the archive has been modified.
 */
class MetadataBackup
{
public:
    MetadataBackup(const QString &filePath);

    /**
     * @brief The comment for the archive.
     */
    const QString comment() const;

    /**
     * @brief The rating for the archive.
     */
    int rating() const;

    /**
     * @brief The tags for the archive.
     */
    const QStringList tags() const;

    /**
     * @brief Restores the user metadata to the given file.
     */
    void restore(const QString &filePath);

private:
    QString m_comment;
    int m_rating;
    QStringList m_tags;
};

} // namespace Kerfuffle
