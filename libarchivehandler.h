#ifndef LIBARCHIVEHANDLER_H
#define LIBARCHIVEHANDLER_H

#include "arch.h"

namespace ThreadWeaver
{
	class Job;
} // namespace ThreadWeaver

class LibArchiveHandler: public Arch
{
	Q_OBJECT
	public:
		LibArchiveHandler( ArkWidget *gui, const QString &filename );
		virtual ~LibArchiveHandler();

		virtual void open();
		virtual void create();

		virtual void addFile( const QStringList & );
		virtual void addDir( const QString & );
		virtual void remove( QStringList* );
		virtual void unarchFileInternal();
		virtual void unarchFile( QStringList *files, const QString& destinationDir, bool viewFriendly = false );

	private slots:
		void listingDone( ThreadWeaver::Job* );
		void extractionDone( ThreadWeaver::Job* );
};

#endif // LIBARCHIVEHANDLER_H
