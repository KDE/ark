/*

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)

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
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef ARARCH_H
#define ARARCH_H

/*
#ifdef __cplusplus
extern "C" {
#endif

void strshort( char *start, int num_rem );

#ifdef __cplusplus
}
#endif
*/

// Qt includes
#include <qstring.h>
#include <qstrlist.h>

// ark includes
#include "arch.h"
#include "arksettings.h"
#include "filelistview.h"

#define UNSUPDIR 1

class ArArch : public Arch {

public:
	ArArch( ArkSettings *d );
	virtual ~ArArch();
	virtual unsigned char setOptions( bool p, bool l, bool o );
	virtual void openArch( const QString &, FileListView *flw );
	virtual void createArch( const QString & );
	virtual int addFile( QStringList *);
	//	virtual void extractTo( const QString &);
	virtual const QStringList *getListing();
	virtual QString unarchFile(QStringList * _fileList);
	virtual void deleteFile( int );

private:
	ArkSettings *data;
	QStringList *listing;
	bool perms, tolower, overwrite;
	void strshort( char *start, int num_rem );
};

#endif /* ARARCH_H */
