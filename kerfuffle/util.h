/*
    SPDX-FileCopyrightText: 2021 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef KERFUFFLE_UTILS_H
#define KERFUFFLE_UTILS_H

namespace Kerfuffle {
namespace Util {

    // Get the name segment from a path
    // e.g. /foo/bar/bla -> bla
    //      /foo/bar/ -> bar
    QString lastPathSegment(const QString &path)
    {
        if (path.endsWith(QLatin1Char('/'))) {
            const int index = path.lastIndexOf(QLatin1Char('/'), -2);
            return path.mid(index + 1).chopped(1);
        } else {
            const int index = path.lastIndexOf(QLatin1Char('/'));
            return path.mid(index + 1);
        }
    }
}
}
#endif
