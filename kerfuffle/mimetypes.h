/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef MIMETYPES_H
#define MIMETYPES_H

#include "kerfuffle_export.h"

#include <QMimeType>

namespace Kerfuffle
{
enum MimePreference {
    PreferContentsMime,
    PreferExtensionMime,
};

/**
 * @param filename Absolute path of a file.
 * @param mp Whether to prefer extension or contents mime when they disagree.
 * @param allowContentDetection Whether content-based detection may be used at all;
 *        pass false to avoid reading the file (e.g. on a slow/remote filesystem).
 * @return The mimetype of the given file.
 */
KERFUFFLE_EXPORT QMimeType determineMimeType(const QString &filename, MimePreference mp = PreferContentsMime, bool allowContentDetection = true);
}

#endif // MIMETYPES_H
