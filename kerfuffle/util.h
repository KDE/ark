/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2021 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
