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


#include "kerfuffle/archiveinterface.h"
#include "kerfuffle/archivefactory.h"

#include <zlib.h>
#include <stdio.h>

#include <KLocale>
#include <KDebug>

#include <QDateTime>
#include <QString>
#include <QFileInfo>
#include <QByteArray>
#include <QFile>
#include <QDir>

using namespace Kerfuffle;

class LibGzipInterface: public ReadOnlyArchiveInterface
{
	Q_OBJECT
	public:
		LibGzipInterface( const QString & filename, QObject *parent )
			: ReadOnlyArchiveInterface( filename, parent )
		{
		}

		~LibGzipInterface()
		{
			kDebug( 1601 ) ;
		}

		QString uncompressedFilename()
		{
			QString fn = QFileInfo(filename()).fileName();
			if (fn.right(3).toUpper() == ".GZ") {
				fn.chop(3);
				return fn;
			}

			//we need to return something...
			return filename() + ".gzUncompressed";
		}

		bool list()
		{
			kDebug( 1601 );
			QString file = uncompressedFilename();

			Q_ASSERT(!file.isEmpty());

			ArchiveEntry e;

			e[ FileName ]       = file;
			e[ InternalID ]     = file;
			//e[ Size ]           = static_cast<qulonglong>( stat.size );
			//e[ Timestamp ]      = QDateTime::fromTime_t( stat.mtime );
			//e[ CompressedSize ] = static_cast<qulonglong>( stat.comp_size );

			entry( e );

			return true;
		}
		void gz_uncompress(gzFile in, FILE* out)
		{
			char buf[16384];
			int len;
			int err;

			for (;;) {
				len = gzread(in, buf, sizeof(buf));
				if (len < 0) error (gzerror(in, &err));
				if (len == 0) break;

				if ((int)fwrite(buf, 1, (unsigned)len, out) != len) {
					error("failed fwrite");
				}
			}
			if (fclose(out)) error("failed fclose");

			if (gzclose(in) != Z_OK) error("failed gzclose");
		}

		bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options )
		{
			kDebug( 1601 ) ;

			Q_UNUSED(files);
			QString outputFilename = destinationDirectory;
			if (!destinationDirectory.endsWith('/'))
				outputFilename += '/';
			outputFilename += uncompressedFilename();


			FILE  *out;
			gzFile in;

			in = gzopen(QFile::encodeName(filename()).data(), "rb");
			if (in == NULL) {
				return false;
			}

			out = fopen(QFile::encodeName(outputFilename).data(), "wb");
			if (out == NULL) {
				return false;
			}
			gz_uncompress(in, out);



			return true;
		}

};

#include "gzplugin.moc"

KERFUFFLE_PLUGIN_FACTORY( LibGzipInterface )
