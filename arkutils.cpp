/*

 ark -- archiver for the KDE project

 Copyright (C) 2005-2007 Henrique Pinto <henrique.pinto@kdemail.net>
 Copyright (C) 2003 Hans Petter Bieker <bieker@kde.org>
 Copyright (C) 2002 Helio Chissini de Castro <helio@conectiva.com.br>
 Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
 Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
 Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>
 Copyright (C) 1997-1999 Rob Palmbos <palm9744@kettering.edu>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

#include "arkutils.h"

// C includes
#include <stdlib.h>
#include <time.h>

#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <unistd.h>

// for statfs:
#ifdef BSD4_4
#include <sys/mount.h>
#elif defined(__linux__) || defined(_HPUX_SOURCE)
#include <sys/vfs.h>
#elif defined(__sun)
#include <sys/statvfs.h>
#define STATFS statvfs
#elif defined(_AIX)
#include <sys/statfs.h>
#endif

#ifndef STATFS
#define STATFS statfs
#endif

// KDE includes
#include <KDebug>
#include <KMessageBox>
#include <KLocale>
#include <kde_file.h>

// Qt includes
#include <QFile>
#include <QFileInfo>


bool ArkUtils::haveDirPermissions( const QString &strFile )
{
	return QFileInfo( QFile::encodeName( strFile ) ).isWritable();
}

bool ArkUtils::diskHasSpace( const QString &dir, KIO::filesize_t size )
{
	// TODO: Do we need this function at all?
	// TODO port to KDiskFreeSpace
	struct STATFS buf;
	if ( STATFS( QFile::encodeName( dir ), &buf ) == 0 )
	{
		double nAvailable = (double)buf.f_bavail * buf.f_bsize;
		if ( nAvailable < (double)size )
		{
			KMessageBox::error( 0, i18n("You have run out of disk space.") );
			return false;
		}
	}
	else
	{
		// something bad happened
		kWarning( 1601 ) << "diskHasSpace() failed" << endl;
		// Q_ASSERT(0);
	}
	return true;
}

KIO::filesize_t ArkUtils::getSizes( const QStringList &list )
{
	KIO::filesize_t sum = 0;
	KDE_struct_stat st;

	foreach( QString str, list )
	{
		str = str.right( str.length()-5 );
		if ( KDE_stat( QFile::encodeName( str ), &st ) < 0 )
			continue;
		sum += st.st_size;
	}
	return sum;
}
