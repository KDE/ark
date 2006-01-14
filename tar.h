/*

ark -- archiver for the KDE project

Copyright (C)

1997-1999: Rob Palmbos palm9744@kettering.edu
1999: Francois-Xavier Duranceau duranceau@kde.org
1999-2000: Corel Corporation (author: Emily Ezust, emilye@corel.com)
2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
2001: Roberto Selbach Teixeira <maragato@conectiva.com>
2003: Georg Robbers <Georg.Robbers@urz.uni-hd.de>
2006: Henrique Pinto <henrique.pinto@kdemail.net>

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

#ifndef TAR_H
#define TAR_H

#include <unistd.h>


class QString;
class QStrList;

class KProcess;
class KTempDir;
class KTarDirectory;
class KTar;

class ArkWidget;
class Arch;
class TarListingThread;

// TarArch can read Tar files and Tar files compressed with gzip.
// It doesn't yet know how to list Tar files compressed with other
// compressors because the listing part is done through KTar.
// If it could be listed though, the rest would work.
// The reason we use KTar for listing is that the output tar -tvf is
// of unreliable format.

class TarArch : public Arch
{
	Q_OBJECT
	public:
		TarArch( ArkWidget *_gui, const QString & _filename,
		         const QString & _openAsMimeType );
		virtual ~TarArch();

		virtual void open();
		virtual void create();

		virtual void addFile( const QStringList & );
		virtual void addDir( const QString & );
		virtual void remove( QStringList* );
		virtual void unarchFileInternal();

		virtual int getEditFlag();

		QString getCompressor();
		QString getUnCompressor();

	public slots:
		void updateProgress( KProcess *_kp, char *_buffer, int _bufflen );
		void openFinished( KProcess * );
		void updateFinished( KProcess * );
		void createTmpFinished( KProcess * );
		void createTmpProgress( KProcess *_kp, char *_buffer, int _bufflen );
		void slotAddFinished( KProcess * );
		void slotListingDone( KProcess * );
		void slotDeleteExited( KProcess * );

	signals:
		void removeDone();
		void createTempDone();
		void updateDone();

	private slots:
		void openFirstCreateTempDone();
		void deleteOldFilesDone();
		void addFileCreateTempDone();
		void addFinishedUpdateDone();
		void removeCreateTempDone();
		void removeUpdateDone();
		
	protected:
		void customEvent( QCustomEvent * );

	private:  // methods
		void updateArch();
		void createTmp();
		void setHeaders();
		void processDir( const KTarDirectory *tardir, const QString & root );
		void deleteOldFiles( const QStringList &list, bool bAddOnlyNew );
		QString getEntry( const QString & filename );

	private: // data
		// if the tar is compressed, this is the temporary uncompressed tar.
		KTempDir * m_tmpDir;
		QString tmpfile;
		QString m_fileMimeType;
		bool compressed;

		// for use with createTmp and updateArch
		bool createTmpInProgress;
		bool updateInProgress;

		// for use with deleteOldFiles
		bool deleteInProgress;
		FILE *fd;
		QStringList m_filesToAdd;
		QStringList m_filesToRemove;
		KProcess * m_pTmpProc;
		KProcess * m_pTmpProc2;
		bool failed;
		bool m_dotslash;
		TarListingThread *m_listingThread;
};

#endif /* TAR_H */

