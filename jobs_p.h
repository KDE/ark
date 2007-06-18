/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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

#ifndef JOBS_P_H
#define JOBS_P_H

#include <ThreadWeaver/Job>
#include <ThreadWeaver/Weaver>

#include "archiveinterface.h"
#include <QList>

class ArchiveJobHelper: public QObject, public ArchiveObserver
{
	Q_OBJECT
	public:
		ArchiveJobHelper( ReadOnlyArchiveInterface *archive, QObject *parent = 0 );
		~ArchiveJobHelper();

		bool getTheListing();

		void onError( const QString & message, const QString & details );
		void onEntry( const ArchiveEntry & archiveEntry );
		void onProgress( double );

	signals:
		void entry( const ArchiveEntry & );
		void progress( double );

	private slots:
		void entryslot( const ArchiveEntry & );

	private:
		ReadOnlyArchiveInterface *m_archive;
};

#endif // JOBS_P_H
