/*
    SPDX-FileCopyrightText: 2024 Hannah von Reth <vonreth@kde.org>
    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef WINDOWS_STAT_H
#define WINDOWS_STAT_H

#include <qplatformdefs.h>

#ifdef Q_OS_WIN

#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & QT_STAT_MASK) == QT_STAT_DIR) /* directory */
#endif

// based on libarchive/archive_windows.h
#ifndef S_IRUSR
#define S_IRUSR _S_IREAD
#endif

#ifndef S_IWUSR
#define S_IWUSR _S_IWRITE
#endif

#ifndef S_IXUSR
#define S_IXUSR _S_IEXEC
#endif

#ifndef S_IRGRP
#define S_IRGRP (S_IRUSR >> 3) /* execute/search permission, group */
#endif

#ifndef S_IXGRP
#define S_IXGRP (S_IXUSR >> 3) /* read permission, group */
#endif

#ifndef S_IWGRP
#define S_IWGRP (S_IWUSR >> 3) /* write permission, group */
#endif

#ifndef S_IWOTH
#define S_IWOTH (S_IWGRP >> 3) /* write permission, other */
#endif

#ifndef S_IROTH
#define S_IROTH (S_IRGRP >> 3) /* execute/search permission, other */
#endif

#ifndef S_IXOTH
#define S_IXOTH (S_IXGRP >> 3) /* read permission, other */
#endif

#ifndef S_ISUID
#define S_ISUID 0004000 /* set user id on execution */
#endif

#ifndef S_ISGID
#define S_ISGID 0002000 /* set group id on execution */
#endif

#ifndef S_ISVTX
#define S_ISVTX 0001000 /* save swapped text even after use */
#endif

#endif

#endif // WINDOWS_STAT_H
