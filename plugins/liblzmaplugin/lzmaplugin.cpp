/***************************************************************************
 *   Copyright (C) 2009 by Wang Hoi <zealot.hoi@gmail.com>                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#include "kerfuffle/archiveinterface.h"
#include "kerfuffle/archivefactory.h"

#include <lzma.h>
#include <stdio.h>
#include <errno.h>

#include <KLocale>
#include <KDebug>

#include <QDateTime>
#include <QString>
#include <QFileInfo>
#include <QByteArray>
#include <QFile>
#include <QDir>

using namespace Kerfuffle;

class LibLZMAInterface: public ReadOnlyArchiveInterface
{
    Q_OBJECT
public:
        LibLZMAInterface( const QString & filename, QObject *parent )
            : ReadOnlyArchiveInterface( filename, parent )
        {
        }

        ~LibLZMAInterface()
        {
            kDebug( 1601 ) ;
        }

        QString uncompressedFilename()
        {
            QString fn = QFileInfo(filename()).fileName();
            if (fn.toUpper().endsWith(".XZ")) {
                fn.chop(3);
                return fn;
            }
            if (fn.toUpper().endsWith(".LZMA")) {
                fn.chop(5);
                return fn;
            }

            //we need to return something...
            return filename() + ".xzUncompressed";
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
        bool lzma_uncompress(lzma_stream *strm, FILE *file, FILE *out_file)
        {
            lzma_ret ret;

            // use 100 Mib as the default limit.
            quint64 memlimit = (100U << 23);

            // Initialize the decoder
            ret = lzma_auto_decoder(strm, memlimit, 0);

            // The only reasonable error here is LZMA_MEM_ERROR.
            // FIXME: Maybe also LZMA_MEMLIMIT_ERROR in future?
            if (ret != LZMA_OK) {
                if (ret == LZMA_MEM_ERROR)
                    error(QString(strerror(ENOMEM)));
                else
                    error("Internal program error (bug)");

                return false;
            }

            // Input and output buffers
            uint8_t in_buf[BUFSIZ];
            uint8_t out_buf[BUFSIZ];

            strm->avail_in = 0;
            strm->next_out = out_buf;
            strm->avail_out = BUFSIZ;

            lzma_action action = LZMA_RUN;

            while (true) {
                if (strm->avail_in == 0) {
                    strm->next_in = in_buf;
                    strm->avail_in = fread(in_buf, 1, BUFSIZ, file);

                    if (ferror(file)) {
                        // POSIX says that fread() sets errno if
                        // an error occurred. ferror() doesn't
                        // touch errno.
                        error(QString("%1: Error reading input file: %2")
                                .arg(filename()).arg(strerror(errno)));
                        return false;
                    }

                    if (! filename().toUpper().endsWith(".LZMA")) {
                        // When using LZMA_CONCATENATED, we need to tell
                        // liblzma when it has got all the input.
                        if (feof(file))
                            action = LZMA_FINISH;
                    }
                }

                ret = lzma_code(strm, action);

                // Write and check write error before checking decoder error.
                // This way as much data as possible gets written to output
                // even if decoder detected an error.
                if (strm->avail_out == 0 || ret != LZMA_OK) {
                    const size_t write_size = BUFSIZ - strm->avail_out;

                    if (fwrite(out_buf, 1, write_size, out_file)
                            != write_size) {
                        // Wouldn't be a surprise if writing to stderr
                        // would fail too but at least try to show an
                        // error message.
                        error(QString("Cannot write to standard output: %1").arg(strerror(errno)));
                        return false;
                    }

                    strm->next_out = out_buf;
                    strm->avail_out = BUFSIZ;
                }

                if (ret != LZMA_OK) {
                    if (ret == LZMA_STREAM_END) {
                        if (filename().toUpper().endsWith("LZMA")) {
                            // Check that there's no trailing garbage.
                            if (strm->avail_in != 0
                                    || fread(in_buf, 1, 1, file)
                                    != 0
                                    || !feof(file))
                                ret = LZMA_DATA_ERROR;
                            else {
                                return true;
                            }
                        } else {
                            return true;
                        }
                    }

                    QString msg;
                    switch (ret) {
                    case LZMA_MEM_ERROR:
                        msg = strerror(ENOMEM);
                        break;

                    case LZMA_MEMLIMIT_ERROR:
                        msg = "Memory usage limit reached";
                        break;

                    case LZMA_FORMAT_ERROR:
                        msg = "File format not recognized";
                        break;

                    case LZMA_OPTIONS_ERROR:
                        // FIXME: Better message?
                        msg = "Unsupported compression options";
                        break;

                    case LZMA_DATA_ERROR:
                        msg = "File is corrupt";
                        break;

                    case LZMA_BUF_ERROR:
                        msg = "Unexpected end of input";
                        break;

                    default:
                        msg = "Internal program error (bug)";
                        break;
                    }

                    error(QString("%1: %2").arg(filename()).arg(msg));

                    return false;
                }
            }
        }

        bool overwriteCheck(QString& filename)
        {
            while (QFile::exists(filename))
            {
                Kerfuffle::OverwriteQuery query(filename);
                query.setMultiMode(false);	// for single file mode
                userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled() || query.responseSkip())
                {
                    return false;
                }
                else if (query.responseOverwrite())
                {
                    break;
                }
                else if (query.responseRename())
                {
                    filename = query.newFilename();
                }
            }

            return true;
        }

        bool copyFiles( const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options )
        {
            kDebug( 1601 ) ;

            Q_UNUSED(files);
            Q_UNUSED(options);

            QString outputFilename = destinationDirectory;
            if (!destinationDirectory.endsWith('/'))
                outputFilename += '/';
            outputFilename += uncompressedFilename();

            if (!overwriteCheck(outputFilename))
                return true;	// just return as success

            FILE *in;
            FILE *out;

            in = fopen(QFile::encodeName(filename()).data(), "rb");
            if (in == NULL) {
                return false;
            }

            out = fopen(QFile::encodeName(outputFilename).data(), "wb");
            if (out == NULL) {
                return false;
            }

            lzma_stream strm = LZMA_STREAM_INIT;

            bool ret = lzma_uncompress(&strm, in, out);

            fclose(in);
            fclose(out);

            return ret;
        }

};

#include "lzmaplugin.moc"

KERFUFFLE_PLUGIN_FACTORY( LibLZMAInterface )
