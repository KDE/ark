/*

 $Id $

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org

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


#ifndef ZIPARCH_H
#define ZIPARCH_H

// Qt includes
#include <qobject.h>

// KDE includes
#include <kprocess.h>

// ark includes
#include "arch.h"
#include "waitDlg.h"


class ArkWidget;

class ZipArch : public Arch
{

 Q_OBJECT

public:
	ZipArch( ArkData*, ArkWidget*, QString );
	virtual ~ZipArch();
	
	virtual void open();
	virtual void create();
	
	virtual int addFile( QStringList* );
	virtual void remove();
	virtual void extract();
	virtual QString unarchFile( int , QString );
	
	virtual int actionFlag();
	void add( QString , int , QString , bool , bool , bool , bool );
	void testIntegrity();
	
	enum AddMode { Update = 1, Freshen, Move };

protected:
	KProcess *m_kp;
	bool perms;
	WaitDlg *m_wd;
	bool m_header_removed, m_finished, m_error;
		
protected slots:
	void slotCancel();
	void slotStoreDataStdout(KProcess*, char*, int);
	void slotStoreDataStderr(KProcess*, char*, int);
	
	void slotOpenDataStdout(KProcess*, char*, int);
	void slotOpenExited(KProcess*);
	
	void slotExtractExited(KProcess*);
	void slotIntegrityExited(KProcess*);
	
private:
	void processLine( char* );	
	void initExtract( bool, bool, bool );
	void initListView();
	void initOpen();
	bool stderrIsError();
	void removeSelectedItems();	
};

#endif /* ZIPARCH_H */
