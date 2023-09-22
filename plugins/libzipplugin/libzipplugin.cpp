/*
    SPDX-FileCopyrightText: 2017 Ragnar Thomsen <rthomsen6@gmail.com>
    SPDX-FileCopyrightText: 2023 Ilya Pominov <ipominov@astralinux.ru>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "libzipplugin.h"
#include "../config.h"
#include "ark_debug.h"
#include "queries.h"

#include <KIO/Global>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <qplatformdefs.h>
#include <QThread>

#include <utime.h>
#include <zlib.h>
#include <memory>

K_PLUGIN_CLASS_WITH_JSON(LibzipPlugin, "kerfuffle_libzip.json")

template <auto fn>
using deleter_from_fn = std::integral_constant<decltype(fn), fn>;
template <typename T, auto fn>
using ark_unique_ptr = std::unique_ptr<T, deleter_from_fn<fn>>;

class ZipSource
{
public:
    ZipSource(const QString &fileName)
    {
        const auto &file = m_files.emplace_back(std::make_unique<QFile>(fileName));
        m_length = static_cast<zip_uint64_t>(file->size());
        m_multiVolumeName = fileName;
        zip_error_init(&m_error);

        // Multi-volume zip's are named name.zip.001, try find other parts
        if (fileName.endsWith(QStringLiteral(".zip.001"), Qt::CaseInsensitive)) {
            m_multiVolumeName.resize(m_multiVolumeName.size() - 4);
            auto fNameSize = fileName.size();
            for (int i = 2; i <= 999; ++i) {
                auto partFileName = fileName;
                partFileName.replace(fNameSize - 3, 3, QStringLiteral("%1").arg(i, 3, 10, QLatin1Char('0')));
                if (!QFileInfo::exists(partFileName)) {
                    break;
                }
                const auto &part = m_files.emplace_back(std::make_unique<QFile>(partFileName));
                m_length += static_cast<zip_uint64_t>(part->size());
            }
        }
    }

    int numberOfVolumes() const
    {
        return static_cast<int>(m_files.size());
    }

    QString multiVolumeName() const
    {
        return m_multiVolumeName;
    }

    zip_int64_t stat(zip_stat_t *info)
    {
        zip_stat_init(info);
        if (!info) {
            zip_error_set(&m_error, ZIP_ER_ZLIB, 0);
            return -1;
        }

        info->size = m_length;
        info->valid = ZIP_STAT_SIZE;
        return sizeof(struct zip_stat);
    }

    zip_int64_t seek(void *data, zip_uint64_t len)
    {
        zip_int64_t newOffset = zip_source_seek_compute_offset(m_offset, m_length, data, len, &m_error);
        if (newOffset < 0) {
            zip_error_set(&m_error, ZIP_ER_SEEK, 0);
            return -1;
        }

        m_offset = static_cast<zip_uint64_t>(newOffset);
        return 0;
    }

    zip_int64_t read(void *data, zip_uint64_t len)
    {
        if (len == 0 || m_offset >= m_length) {
            return 0;
        }

        zip_int64_t ret = 0;
        zip_uint64_t offset = m_offset;
        for (auto &file : m_files) {
            const auto length = static_cast<zip_uint64_t>(file->size());
            if (offset >= length) {
                offset -= length;
                continue;
            }

            if (!file->isOpen() && !file->open(QIODevice::ReadOnly)) {
                qCDebug(ARK) << "ZipSource error: Can't open" << file->fileName();
                break;
            }

            const auto available = (length - offset) < len ? length - offset : len;
            if (!file->seek(static_cast<qint64>(offset))) {
                qCDebug(ARK) << "ZipSource error: Can't seek to" << available << "in file" << file->fileName();
                break;
            }

            const auto readed = file->read(reinterpret_cast<char *>(data), static_cast<qint64>(available));
            if (static_cast<zip_uint64_t>(readed) != available) {
                qCDebug(ARK) << "ZipSource error: Read" << readed << "bytes instead" << available << "in file" << file->fileName();
                break;
            }

            ret += static_cast<zip_int64_t>(available);
            m_offset += available;
            offset = 0;
            len -= available;
            data = reinterpret_cast<char *>(data) + available;
            if (len == 0) {
                return ret;
            }
        }

        zip_error_set(&m_error, ZIP_ER_READ, 0);
        return -1;
    }

    // Commands should return -1 on error. ZIP_SOURCE_ERROR will be called to retrieve the error code.
    // On success, commands return 0, unless specified otherwise in the description above.
    // See https://libzip.org/documentation/zip_source_function_create.html for more details.
    static zip_int64_t callbackFn(void *userdata, void *data, zip_uint64_t len, zip_source_cmd_t cmd)
    {
        auto source = reinterpret_cast<ZipSource *>(userdata);
        switch(cmd)
        {
            case ZIP_SOURCE_OPEN:
                return 0;
            case ZIP_SOURCE_READ:
                return source->read(data, len);
            case ZIP_SOURCE_CLOSE:
                return 0;
            case ZIP_SOURCE_STAT:
                return source->stat(reinterpret_cast<zip_stat_t *>(data));
            case ZIP_SOURCE_ERROR:
                return zip_error_to_data(&source->m_error, data, len);
            case ZIP_SOURCE_FREE:
                return 0;
            case ZIP_SOURCE_SEEK:
                return source->seek(data, len);
            case ZIP_SOURCE_TELL:
                return static_cast<zip_int64_t>(source->m_offset);
            case ZIP_SOURCE_SUPPORTS:
                return ZIP_SOURCE_SUPPORTS_SEEKABLE;
            default:
                zip_error_set(&source->m_error, ZIP_ER_INVAL, 0);
                break;
        }
        return -1;
    }

    static ark_unique_ptr<zip_t, zip_discard> create(LibzipPlugin *plugin, ZipSource &zipSource, int zipOpenFlags)
    {
        zip_error_t err;
        zip_error_init(&err);
        ark_unique_ptr<zip_t, zip_discard> archive;
        if (plugin->isMultiVolume()) {
            auto source = zip_source_function_create(&ZipSource::callbackFn, &zipSource, nullptr);
            archive.reset(zip_open_from_source(source, zipOpenFlags, &err));
            if (!archive) {
                zip_source_free(source);
            }
        } else {
            int errcode = 0;
            archive.reset(zip_open(QFile::encodeName(plugin->filename()).constData(), zipOpenFlags, &errcode));
            zip_error_init_with_code(&err, errcode);
        }
        if (!archive) {
            qCCritical(ARK) << "Failed to open archive. Code:" << zip_error_code_zip(&err);
            Q_EMIT plugin->error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        }
        return archive;
    }

private:
    std::vector<std::unique_ptr<QFile> > m_files;
    QString m_multiVolumeName;
    zip_error_t m_error;
    zip_uint64_t m_length = 0;
    zip_uint64_t m_offset = 0;
};

void LibzipPlugin::progressCallback(zip_t *, double progress, void *that)
{
    static_cast<LibzipPlugin *>(that)->emitProgress(progress);
}

int LibzipPlugin::cancelCallback(zip_t *, void * /* unused that*/)
{
    return QThread::currentThread()->isInterruptionRequested();
}

LibzipPlugin::LibzipPlugin(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
    , m_overwriteAll(false)
    , m_skipAll(false)
    , m_listAfterAdd(false)
    , m_backslashedZip(false)
    , m_zipSource(std::make_unique<ZipSource>(filename()))
{
    qCDebug(ARK) << "Initializing libzip plugin";

    if (m_zipSource->numberOfVolumes() > 1) {
        m_numberOfVolumes = m_zipSource->numberOfVolumes();
        setMultiVolume(true);
        m_multiVolumeName = m_zipSource->multiVolumeName();
    }
}

LibzipPlugin::~LibzipPlugin()
{
    for (const auto e : std::as_const(m_emittedEntries)) {
        // Entries might be passed to pending slots, so we just schedule their deletion.
        e->deleteLater();
    }
}

bool LibzipPlugin::list()
{
    qCDebug(ARK) << "Listing archive contents for:" << QFile::encodeName(filename());
    m_numberOfEntries = 0;

    // Open archive.
    auto archive = ZipSource::create(this, *m_zipSource, ZIP_RDONLY);
    if (!archive) {
        return false;
    }

    // Fetch archive comment.
    m_comment = QString::fromUtf8(zip_get_archive_comment(archive.get(), nullptr, ZIP_FL_ENC_GUESS));

    // Get number of archive entries.
    const auto nofEntries = zip_get_num_entries(archive.get(), 0);
    qCDebug(ARK) << "Found entries:" << nofEntries;

    // Loop through all archive entries.
    for (int i = 0; i < nofEntries; i++) {

        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        emitEntryForIndex(archive.get(), i);
        if (m_listAfterAdd) {
            // Start at 50%.
            Q_EMIT progress(0.5 + (0.5 * float(i + 1) / nofEntries));
        } else {
            Q_EMIT progress(float(i + 1) / nofEntries);
        }
    }

    m_listAfterAdd = false;
    return true;
}

bool LibzipPlugin::addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options, uint numberOfEntriesToAdd)
{
    Q_UNUSED(numberOfEntriesToAdd)
    int errcode = 0;
    zip_error_t err;

    // Open archive and don't write changes in unique_ptr destructor but instead call zip_close manually when needed.
    ark_unique_ptr<zip_t, zip_discard> archive { zip_open(QFile::encodeName(filename()).constData(), ZIP_CREATE, &errcode) };
    zip_error_init_with_code(&err, errcode);
    if (!archive) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        Q_EMIT error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    uint i = 0;
    for (const Archive::Entry* e : files) {

        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        // If entry is a directory, traverse and add all its files and subfolders.
        if (QFileInfo(e->fullPath()).isDir()) {

            if (!writeEntry(archive.get(), e->fullPath(), destination, options, true)) {
                return false;
            }

            QDirIterator it(e->fullPath(),
                            QDir::AllEntries | QDir::Readable |
                            QDir::Hidden | QDir::NoDotAndDotDot,
                            QDirIterator::Subdirectories);

            while (!QThread::currentThread()->isInterruptionRequested() && it.hasNext()) {
                const QString path = it.next();

                if (QFileInfo(path).isDir()) {
                    if (!writeEntry(archive.get(), path, destination, options, true)) {
                        return false;
                    }
                } else {
                    if (!writeEntry(archive.get(), path, destination, options)) {
                        return false;
                    }
                }
                i++;
            }
        } else {
            if (!writeEntry(archive.get(), e->fullPath(), destination, options)) {
                return false;
            }
        }
        i++;
    }
    qCDebug(ARK) << "Writing " << i << "entries to disk...";

    // Register the callback function to get progress feedback and cancelation.
    zip_register_progress_callback_with_state(archive.get(), 0.001, progressCallback, nullptr, this);
#if LIBZIP_CANCELATION
    zip_register_cancel_callback_with_state(archive.get(), cancelCallback, nullptr, this);
#endif

    // Write and close archive manually.
    zip_close(archive.get());
    // Release unique pointer as it set to NULL via zip_close.
    archive.release();
    if (errcode > 0) {
        qCCritical(ARK) << "Failed to write archive";
        Q_EMIT error(xi18n("Failed to write archive."));
        return false;
    }

    if (QThread::currentThread()->isInterruptionRequested()) {
        return false;
    }

    // We list the entire archive after adding files to ensure entry
    // properties are up-to-date.
    m_listAfterAdd = true;
    list();

    return true;
}

void LibzipPlugin::emitProgress(double percentage)
{
    // Go from 0 to 50%. The second half is the subsequent listing.
    Q_EMIT progress(0.5 * percentage);
}

bool LibzipPlugin::writeEntry(zip_t *archive, const QString &file, const Archive::Entry* destination, const CompressionOptions& options, bool isDir)
{
    Q_ASSERT(archive);

    QByteArray destFile;
    if (destination) {
        destFile = fromUnixSeparator(QString(destination->fullPath() + file)).toUtf8();
    } else {
        destFile = fromUnixSeparator(file).toUtf8();
    }

    qlonglong index;
    if (isDir) {
        index = zip_dir_add(archive, destFile.constData(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            // If directory already exists in archive, we get an error.
            qCWarning(ARK) << "Failed to add dir " << file << ":" << zip_strerror(archive);
            return true;
        }
    } else {
        zip_source_t *src = zip_source_file(archive, QFile::encodeName(file).constData(), 0, -1);
        Q_ASSERT(src);

        index = zip_file_add(archive, destFile.constData(), src, ZIP_FL_ENC_GUESS | ZIP_FL_OVERWRITE);
        if (index == -1) {
            zip_source_free(src);
            qCCritical(ARK) << "Could not add entry" << file << ":" << zip_strerror(archive);
            Q_EMIT error(xi18n("Failed to add entry: %1", QString::fromUtf8(zip_strerror(archive))));
            return false;
        }
    }

#ifndef Q_OS_WIN
    // Set permissions.
    QT_STATBUF result;
    if (QT_STAT(QFile::encodeName(file).constData(), &result) != 0) {
        qCWarning(ARK) << "Failed to read permissions for:" << file;
    } else {
        zip_uint32_t attributes = result.st_mode << 16;
        if (zip_file_set_external_attributes(archive, index, ZIP_FL_UNCHANGED, ZIP_OPSYS_UNIX, attributes) != 0) {
            qCWarning(ARK) << "Failed to set external attributes for:" << file;
        }
    }
#endif

    if (!password().isEmpty()) {
        Q_ASSERT(!options.encryptionMethod().isEmpty());
        if (options.encryptionMethod() == QLatin1String("AES128")) {
            zip_file_set_encryption(archive, index, ZIP_EM_AES_128, password().toUtf8().constData());
        } else if (options.encryptionMethod() == QLatin1String("AES192")) {
            zip_file_set_encryption(archive, index, ZIP_EM_AES_192, password().toUtf8().constData());
        } else if (options.encryptionMethod() == QLatin1String("AES256")) {
            zip_file_set_encryption(archive, index, ZIP_EM_AES_256, password().toUtf8().constData());
        }
    }

    // Set compression level and method.
    zip_int32_t compMethod = ZIP_CM_DEFAULT;
    if (!options.compressionMethod().isEmpty()) {
        if (options.compressionMethod() == QLatin1String("Deflate")) {
            compMethod = ZIP_CM_DEFLATE;
        } else if (options.compressionMethod() == QLatin1String("BZip2")) {
            compMethod = ZIP_CM_BZIP2;
#ifdef ZIP_CM_ZSTD
        } else if (options.compressionMethod() == QLatin1String("Zstd")) {
            compMethod = ZIP_CM_ZSTD;
#endif
#ifdef ZIP_CM_LZMA
        } else if (options.compressionMethod() == QLatin1String("LZMA")) {
            compMethod = ZIP_CM_LZMA;
#endif
#ifdef ZIP_CM_XZ
        } else if (options.compressionMethod() == QLatin1String("XZ")) {
            compMethod = ZIP_CM_XZ;
#endif
        } else if (options.compressionMethod() == QLatin1String("Store")) {
            compMethod = ZIP_CM_STORE;
        }
    }
    const int compLevel = options.isCompressionLevelSet() ? options.compressionLevel() : 6;
    if (zip_set_file_compression(archive, index, compMethod, compLevel) != 0) {
        qCCritical(ARK) << "Could not set compression options for" << file << ":" << zip_strerror(archive);
        Q_EMIT error(xi18n("Failed to set compression options for entry: %1", QString::fromUtf8(zip_strerror(archive))));
        return false;
    }

    return true;
}

bool LibzipPlugin::emitEntryForIndex(zip_t *archive, qlonglong index)
{
    Q_ASSERT(archive);

    zip_stat_t statBuffer;
    if (zip_stat_index(archive, index, ZIP_FL_ENC_GUESS, &statBuffer)) {
        qCCritical(ARK) << "Failed to read stat for index" << index;
        return false;
    }

    auto e = new Archive::Entry();
    auto name = toUnixSeparator(QString::fromUtf8(statBuffer.name));

    if (statBuffer.valid & ZIP_STAT_NAME) {
        e->setFullPath(name);
    }

    if (e->fullPath(PathFormat::WithTrailingSlash).endsWith(QDir::separator())) {
        e->setProperty("isDirectory", true);
    }

    if (statBuffer.valid & ZIP_STAT_MTIME) {
        e->setProperty("timestamp", QDateTime::fromSecsSinceEpoch(statBuffer.mtime));
    }
    if (statBuffer.valid & ZIP_STAT_SIZE) {
        e->setProperty("size", (qulonglong)statBuffer.size);
    }
    if (statBuffer.valid & ZIP_STAT_COMP_SIZE) {
        e->setProperty("compressedSize", (qlonglong)statBuffer.comp_size);
    }
    if (statBuffer.valid & ZIP_STAT_CRC) {
        if (!e->isDir()) {
            e->setProperty("CRC", QStringLiteral("%1").arg((qulonglong)statBuffer.crc, /*fieldWidth*/ 8, /*base*/ 16, QLatin1Char('0')).toUpper());
        }
    }
    if (statBuffer.valid & ZIP_STAT_COMP_METHOD) {
        switch(statBuffer.comp_method) {
            case ZIP_CM_STORE:
                e->setProperty("method", QStringLiteral("Store"));
                Q_EMIT compressionMethodFound(QStringLiteral("Store"));
                break;
            case ZIP_CM_DEFLATE:
                e->setProperty("method", QStringLiteral("Deflate"));
                Q_EMIT compressionMethodFound(QStringLiteral("Deflate"));
                break;
            case ZIP_CM_DEFLATE64:
                e->setProperty("method", QStringLiteral("Deflate64"));
                Q_EMIT compressionMethodFound(QStringLiteral("Deflate64"));
                break;
            case ZIP_CM_BZIP2:
                e->setProperty("method", QStringLiteral("BZip2"));
                Q_EMIT compressionMethodFound(QStringLiteral("BZip2"));
                break;
#ifdef ZIP_CM_ZSTD
            case ZIP_CM_ZSTD:
                e->setProperty("method", QStringLiteral("Zstd"));
                Q_EMIT compressionMethodFound(QStringLiteral("Zstd"));
                break;
#endif
#ifdef ZIP_CM_LZMA
            case ZIP_CM_LZMA:
                e->setProperty("method", QStringLiteral("LZMA"));
                Q_EMIT compressionMethodFound(QStringLiteral("LZMA"));
                break;
#endif
#ifdef ZIP_CM_XZ
            case ZIP_CM_XZ:
                e->setProperty("method", QStringLiteral("XZ"));
                Q_EMIT compressionMethodFound(QStringLiteral("XZ"));
                break;
#endif
        }
    }
    if (statBuffer.valid & ZIP_STAT_ENCRYPTION_METHOD) {
        if (statBuffer.encryption_method != ZIP_EM_NONE) {
            e->setProperty("isPasswordProtected", true);
            switch(statBuffer.encryption_method) {
                case ZIP_EM_TRAD_PKWARE:
                    Q_EMIT encryptionMethodFound(QStringLiteral("ZipCrypto"));
                    break;
                case ZIP_EM_AES_128:
                    Q_EMIT encryptionMethodFound(QStringLiteral("AES128"));
                    break;
                case ZIP_EM_AES_192:
                    Q_EMIT encryptionMethodFound(QStringLiteral("AES192"));
                    break;
                case ZIP_EM_AES_256:
                    Q_EMIT encryptionMethodFound(QStringLiteral("AES256"));
                    break;
            }
        }
    }

    // Read external attributes, which contains the file permissions.
    zip_uint8_t opsys;
    zip_uint32_t attributes;
    if (zip_file_get_external_attributes(archive, index, ZIP_FL_UNCHANGED, &opsys, &attributes) == -1) {
        qCCritical(ARK) << "Could not read external attributes for entry:" << name;
        Q_EMIT error(xi18n("Failed to read metadata for entry: %1", name));
        return false;
    }

    // Set permissions.
    switch (opsys) {
    case ZIP_OPSYS_UNIX:
        // Unix permissions are stored in the leftmost 16 bits of the external file attribute.
        e->setProperty("permissions", permissionsToString(attributes >> 16));
        break;
    default:    // TODO: non-UNIX.
        break;
    }

    Q_EMIT entry(e);
    m_emittedEntries << e;

    return true;
}

bool LibzipPlugin::deleteFiles(const QVector<Archive::Entry*> &files)
{
    int errcode = 0;
    zip_error_t err;

    // Open archive and don't write changes in unique_ptr destructor but instead call zip_close manually when needed.
    ark_unique_ptr<zip_t, zip_discard> archive { zip_open(QFile::encodeName(filename()).constData(), 0, &errcode) };
    zip_error_init_with_code(&err, errcode);
    if (archive.get() == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        Q_EMIT error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    qulonglong i = 0;
    for (const Archive::Entry* e : files) {

        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }

        const qlonglong index = zip_name_locate(archive.get(), fromUnixSeparator(e->fullPath()).toUtf8().constData(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            qCCritical(ARK) << "Could not find entry to delete:" << e->fullPath();
            Q_EMIT error(xi18n("Failed to delete entry: %1", e->fullPath()));
            return false;
        }
        if (zip_delete(archive.get(), index) == -1) {
            qCCritical(ARK) << "Could not delete entry" << e->fullPath() << ":" << zip_strerror(archive.get());
            Q_EMIT error(xi18n("Failed to delete entry: %1", QString::fromUtf8(zip_strerror(archive.get()))));
            return false;
        }
        Q_EMIT entryRemoved(e->fullPath());
        Q_EMIT progress(float(++i) / files.size());
    }
    qCDebug(ARK) << "Deleted" << i << "entries";

    // Write and close archive manually.
    zip_close(archive.get());
    // Release unique pointer as it set to NULL via zip_close.
    archive.release();
    if (errcode > 0) {
        qCCritical(ARK) << "Failed to write archive";
        Q_EMIT error(xi18n("Failed to write archive."));
        return false;
    }
    return true;
}

bool LibzipPlugin::addComment(const QString& comment)
{
    int errcode = 0;
    zip_error_t err;

    // Open archive and don't write changes in unique_ptr destructor but instead call zip_close manually when needed.
    ark_unique_ptr<zip_t, zip_discard> archive { zip_open(QFile::encodeName(filename()).constData(), 0, &errcode) };
    zip_error_init_with_code(&err, errcode);
    if (archive.get() == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        Q_EMIT error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    // Set archive comment.
    if (zip_set_archive_comment(archive.get(), comment.toUtf8().constData(), comment.length())) {
        qCCritical(ARK) << "Failed to set comment:" << zip_strerror(archive.get());
        return false;
    }

    // Write comment to archive.
    zip_close(archive.get());
    // Release unique pointer as it set to NULL via zip_close.
    archive.release();
    if (errcode > 0) {
        qCCritical(ARK) << "Failed to write archive";
        Q_EMIT error(xi18n("Failed to write archive."));
        return false;
    }
    return true;
}

bool LibzipPlugin::testArchive()
{
    qCDebug(ARK) << "Testing archive";

    // Open archive performing extra consistency checks, free memory using zip_discard as no write oprations needed.
    auto archive = ZipSource::create(this, *m_zipSource, ZIP_RDONLY | ZIP_CHECKCONS);
    if (!archive) {
        return false;
    }

    // Check CRC-32 for each archive entry.
    const int nofEntries = zip_get_num_entries(archive.get(), 0);
    for (int i = 0; i < nofEntries; i++) {

        if (QThread::currentThread()->isInterruptionRequested()) {
            return false;
        }

        // Get statistic for entry. Used to get entry size.
        zip_stat_t statBuffer;
        int stat_index = zip_stat_index(archive.get(), i, 0, &statBuffer);
        auto name = toUnixSeparator(QString::fromUtf8(statBuffer.name));
        if (stat_index != 0) {
            qCCritical(ARK) << "Failed to read stat for" << name;
            return false;
        }

        ark_unique_ptr<zip_file, zip_fclose> zipFile { zip_fopen_index(archive.get(), i, 0) };
        std::unique_ptr<uchar[]> buf(new uchar[statBuffer.size]);
        const int len = zip_fread(zipFile.get(), buf.get(), statBuffer.size);
        if (len == -1 || uint(len) != statBuffer.size) {
            qCCritical(ARK) << "Failed to read data for" << name;
            return false;
        }
        if (statBuffer.crc != crc32(0, &buf.get()[0], len)) {
            qCCritical(ARK) << "CRC check failed for" << name;
            return false;
        }

        Q_EMIT progress(float(i) / nofEntries);
    }

    Q_EMIT testSuccess();
    return true;
}

bool LibzipPlugin::doKill()
{
    return false;
}

bool LibzipPlugin::extractFiles(const QVector<Archive::Entry*> &files, const QString& destinationDirectory, const ExtractionOptions& options)
{
    qCDebug(ARK) << "Extracting files to:" << destinationDirectory;
    const bool extractAll = files.isEmpty();
    const bool removeRootNode = options.isDragAndDropEnabled();

    // Open archive, free memory using zip_discard as no write oprations needed.
    auto archive = ZipSource::create(this, *m_zipSource, ZIP_RDONLY);
    if (!archive) {
        return false;
    }

    // Set password if known.
    if (!password().isEmpty()) {
        qCDebug(ARK) << "Password already known. Setting...";
        zip_set_default_password(archive.get(), password().toUtf8().constData());
    }

    // Get number of archive entries.
    const qlonglong nofEntries = extractAll ? zip_get_num_entries(archive.get(), 0) : files.size();

    // Extract entries.
    m_overwriteAll = false; // Whether to overwrite all files
    m_skipAll = false; // Whether to skip all files
    if (extractAll) {
        // We extract all entries.
        for (qlonglong i = 0; i < nofEntries; i++) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }
            if (!extractEntry(archive.get(),
                              toUnixSeparator(QString::fromUtf8(zip_get_name(archive.get(), i, ZIP_FL_ENC_GUESS))),
                              QString(),
                              destinationDirectory,
                              options.preservePaths(),
                              removeRootNode)) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            Q_EMIT progress(float(i + 1) / nofEntries);
        }
    } else {
        // We extract only the entries in files.
        qulonglong i = 0;
        for (const Archive::Entry* e : files) {
            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }
            if (!extractEntry(archive.get(),
                              e->fullPath(),
                              e->rootNode,
                              destinationDirectory,
                              options.preservePaths(),
                              removeRootNode)) {
                qCDebug(ARK) << "Extraction failed";
                return false;
            }
            Q_EMIT progress(float(++i) / nofEntries);
        }
    }

    return true;
}

bool LibzipPlugin::extractEntry(zip_t *archive, const QString &entry, const QString &rootNode, const QString &destDir, bool preservePaths, bool removeRootNode)
{
    const bool isDirectory = entry.endsWith(QDir::separator());

    // Add trailing slash to destDir if not present.
    QString destDirCorrected(destDir);
    if (!destDir.endsWith(QDir::separator())) {
        destDirCorrected.append(QDir::separator());
    }

    // Remove rootnode if supplied and set destination path.
    QString destination;
    if (preservePaths) {
        if (!removeRootNode || rootNode.isEmpty()) {
            destination = destDirCorrected + entry;
        } else {
            QString truncatedEntry = entry;
            truncatedEntry.remove(0, rootNode.size());
            destination = destDirCorrected + truncatedEntry;
        }
    } else {
        if (isDirectory) {
            qCDebug(ARK) << "Skipping directory:" << entry;
            return true;
        }
        destination = destDirCorrected + QFileInfo(entry).fileName();
    }

    // Store parent mtime.
    QString parentDir;
    if (isDirectory) {
        QDir pDir = QFileInfo(destination).dir();
        pDir.cdUp();
        parentDir = pDir.path();
    } else {
        parentDir = QFileInfo(destination).path();
    }
    // For top-level items, don't restore parent dir mtime.
    const bool restoreParentMtime = (parentDir + QDir::separator() != destDirCorrected);

    time_t parent_mtime;
    if (restoreParentMtime) {
        parent_mtime = QFileInfo(parentDir).lastModified().toMSecsSinceEpoch() / 1000;
    }

    // Create parent directories for files. For directories create them.
    if (!QDir().mkpath(QFileInfo(destination).path())) {
        qCDebug(ARK) << "Failed to create directory:" << QFileInfo(destination).path();
        Q_EMIT error(xi18n("Failed to create directory: %1", QFileInfo(destination).path()));
        return false;
    }

    // Get statistic for entry. Used to get entry size and mtime.
    zip_stat_t statBuffer;
    if (zip_stat(archive, fromUnixSeparator(entry).toUtf8().constData(), 0, &statBuffer) != 0) {
        if (isDirectory && zip_error_code_zip(zip_get_error(archive)) == ZIP_ER_NOENT) {
            qCWarning(ARK) << "Skipping folder without entry:" << entry;
            return true;
        }
        qCCritical(ARK) << "Failed to read stat for entry" << entry;
        return false;
    }

    if (!isDirectory) {

        // Handle existing destination files.
        QString renamedEntry = entry;
        while (!m_overwriteAll && QFileInfo::exists(destination)) {
            if (m_skipAll) {
                return true;
            } else {
                Kerfuffle::OverwriteQuery query(renamedEntry);
                Q_EMIT userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    Q_EMIT cancelled();
                    return false;
                } else if (query.responseSkip()) {
                    return true;
                } else if (query.responseAutoSkip()) {
                    m_skipAll = true;
                    return true;
                } else if (query.responseRename()) {
                    const QString newName(query.newFilename());
                    destination = QFileInfo(destination).path() + QDir::separator() + QFileInfo(newName).fileName();
                    renamedEntry = QFileInfo(entry).path() + QDir::separator() + QFileInfo(newName).fileName();
                } else if (query.responseOverwriteAll()) {
                    m_overwriteAll = true;
                    break;
                } else if (query.responseOverwrite()) {
                    break;
                }
            }
        }

        // Handle password-protected files.
        ark_unique_ptr<zip_file, zip_fclose> zipFile { nullptr };
        bool firstTry = true;
        while (!zipFile) {
            zipFile.reset(zip_fopen(archive, fromUnixSeparator(entry).toUtf8().constData(), 0));
            if (zipFile) {
                break;
            } else if (zip_error_code_zip(zip_get_error(archive)) == ZIP_ER_NOPASSWD ||
                       zip_error_code_zip(zip_get_error(archive)) == ZIP_ER_WRONGPASSWD) {
                Kerfuffle::PasswordNeededQuery query(filename(), !firstTry);
                Q_EMIT userQuery(&query);
                query.waitForResponse();

                if (query.responseCancelled()) {
                    Q_EMIT cancelled();
                    return false;
                }
                setPassword(query.password());

                if (zip_set_default_password(archive, password().toUtf8().constData())) {
                    qCDebug(ARK) << "Failed to set password for:" << entry;
                }
                firstTry = false;
            } else {
                qCCritical(ARK) << "Failed to open file:" << zip_strerror(archive);
                Q_EMIT error(xi18n("Failed to open '%1':<nl/>%2", entry, QString::fromUtf8(zip_strerror(archive))));
                return false;
            }
        }

        QFile file(destination);
        if (!file.open(QIODevice::WriteOnly)) {
            qCCritical(ARK) << "Failed to open file for writing";
            Q_EMIT error(xi18n("Failed to open file for writing: %1", destination));
            return false;
        }

        QDataStream out(&file);

        // Write archive entry to file. We use a read/write buffer of 1000 chars.
        qulonglong sum = 0;
        char buf[1000];
        while (sum != statBuffer.size) {
            const auto readBytes = zip_fread(zipFile.get(), buf, 1000);
            if (readBytes < 0) {
                qCCritical(ARK) << "Failed to read data";
                Q_EMIT error(xi18n("Failed to read data for entry: %1", entry));
                return false;
            }
            if (out.writeRawData(buf, readBytes) != readBytes) {
                qCCritical(ARK) << "Failed to write data";
                Q_EMIT error(xi18n("Failed to write data for entry: %1", entry));
                return false;
            }

            sum += readBytes;
        }

        const auto index = zip_name_locate(archive, fromUnixSeparator(entry).toUtf8().constData(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            qCCritical(ARK) << "Could not locate entry:" << entry;
            Q_EMIT error(xi18n("Failed to locate entry: %1", entry));
            return false;
        }

        zip_uint8_t opsys;
        zip_uint32_t attributes;
        if (zip_file_get_external_attributes(archive, index, ZIP_FL_UNCHANGED, &opsys, &attributes) == -1) {
            qCCritical(ARK) << "Could not read external attributes for entry:" << entry;
            Q_EMIT error(xi18n("Failed to read metadata for entry: %1", entry));
            return false;
        }

        // Inspired by fuse-zip source code: fuse-zip/lib/fileNode.cpp
        switch (opsys) {
        case ZIP_OPSYS_UNIX:
            if (attributes != 0) {
                // Unix permissions are stored in the leftmost 16 bits of the external file attribute.
                file.setPermissions(KIO::convertPermissions(attributes >> 16));
            }
            break;
        default:    // TODO: non-UNIX.
            break;
        }

        file.close();
    }

    // Set mtime for entry (also access time otherwise it's "uninitilized")
    utimbuf times;
    times.actime = statBuffer.mtime;
    times.modtime = statBuffer.mtime;
    if (utime(destination.toUtf8().constData(), &times) != 0) {
        qCWarning(ARK) << "Failed to restore mtime:" << destination;
    }

    if (restoreParentMtime) {
        // Restore mtime for parent dir.
        times.actime = parent_mtime;
        times.modtime = parent_mtime;
        if (utime(parentDir.toUtf8().constData(), &times) != 0) {
            qCWarning(ARK) << "Failed to restore mtime for parent dir of:" << destination;
        }
    }

    return true;
}

bool LibzipPlugin::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options)
    int errcode = 0;
    zip_error_t err;

    // Open archive.
    ark_unique_ptr<zip_t, zip_close> archive { zip_open(QFile::encodeName(filename()).constData(), 0, &errcode) };
    zip_error_init_with_code(&err, errcode);
    if (archive.get() == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        Q_EMIT error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    QStringList filePaths = entryFullPaths(files);
    filePaths.sort();
    const QStringList destPaths = entryPathsFromDestination(filePaths, destination, entriesWithoutChildren(files).count());

    int i;
    for (i = 0; i < filePaths.size(); ++i) {

        const int index = zip_name_locate(archive.get(), filePaths.at(i).toUtf8().constData(), ZIP_FL_ENC_GUESS);
        if (index == -1) {
            qCCritical(ARK) << "Could not find entry to move:" << filePaths.at(i);
            Q_EMIT error(xi18n("Failed to move entry: %1", filePaths.at(i)));
            return false;
        }

        if (zip_file_rename(archive.get(), index, destPaths.at(i).toUtf8().constData(), ZIP_FL_ENC_GUESS) == -1) {
            qCCritical(ARK) << "Could not move entry:" << filePaths.at(i);
            Q_EMIT error(xi18n("Failed to move entry: %1", filePaths.at(i)));
            return false;
        }

        Q_EMIT entryRemoved(filePaths.at(i));
        emitEntryForIndex(archive.get(), index);
        Q_EMIT progress(i/filePaths.count());
    }

    // Write and close archive manually.
    zip_close(archive.get());
    // Release unique pointer as it set to NULL via zip_close.
    archive.release();
    if (errcode > 0) {
        qCCritical(ARK) << "Failed to write archive";
        Q_EMIT error(xi18n("Failed to write archive."));
        return false;
    }

    qCDebug(ARK) << "Moved" << i << "entries";

    return true;
}

bool LibzipPlugin::copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options)
    int errcode = 0;
    zip_error_t err;

    // Open archive and don't write changes in unique_ptr destructor but instead call zip_close manually when needed.
    ark_unique_ptr<zip_t, zip_discard> archive { zip_open(QFile::encodeName(filename()).constData(), 0, &errcode) };
    zip_error_init_with_code(&err, errcode);
    if (archive.get() == nullptr) {
        qCCritical(ARK) << "Failed to open archive. Code:" << errcode;
        Q_EMIT error(xi18n("Failed to open archive: %1", QString::fromUtf8(zip_error_strerror(&err))));
        return false;
    }

    const QStringList filePaths = entryFullPaths(files);
    const QStringList destPaths = entryPathsFromDestination(filePaths, destination, 0);

    int i;
    for (i = 0; i < filePaths.size(); ++i) {

        QString dest = destPaths.at(i);

        if (dest.endsWith(QDir::separator())) {
            if (zip_dir_add(archive.get(), dest.toUtf8().constData(), ZIP_FL_ENC_GUESS) == -1) {
                // If directory already exists in archive, we get an error.
                qCWarning(ARK) << "Failed to add dir " << dest << ":" << zip_strerror(archive.get());
                continue;
            }
        }

        const int srcIndex = zip_name_locate(archive.get(), filePaths.at(i).toUtf8().constData(), ZIP_FL_ENC_GUESS);
        if (srcIndex == -1) {
            qCCritical(ARK) << "Could not find entry to copy:" << filePaths.at(i);
            Q_EMIT error(xi18n("Failed to copy entry: %1", filePaths.at(i)));
            return false;
        }

        zip_source_t *src = zip_source_zip(archive.get(), archive.get(), srcIndex, 0, 0, -1);
        if (!src) {
            qCCritical(ARK) << "Failed to create source for:" << filePaths.at(i);
            return false;
        }

        const int destIndex = zip_file_add(archive.get(), dest.toUtf8().constData(), src, ZIP_FL_ENC_GUESS | ZIP_FL_OVERWRITE);
        if (destIndex == -1) {
            zip_source_free(src);
            qCCritical(ARK) << "Could not add entry" << dest << ":" << zip_strerror(archive.get());
            Q_EMIT error(xi18n("Failed to add entry: %1", QString::fromUtf8(zip_strerror(archive.get()))));
            return false;
        }

        // Get permissions from source entry.
        zip_uint8_t opsys;
        zip_uint32_t attributes;
        if (zip_file_get_external_attributes(archive.get(), srcIndex, ZIP_FL_UNCHANGED, &opsys, &attributes) == -1) {
            qCCritical(ARK) << "Failed to read external attributes for source:" << filePaths.at(i);
            Q_EMIT error(xi18n("Failed to read metadata for entry: %1", filePaths.at(i)));
            return false;
        }

        // Set permissions on dest entry.
        if (zip_file_set_external_attributes(archive.get(), destIndex, ZIP_FL_UNCHANGED, opsys, attributes) != 0) {
            qCCritical(ARK) << "Failed to set external attributes for destination:" << dest;
            Q_EMIT error(xi18n("Failed to set metadata for entry: %1", dest));
            return false;
        }
    }

    // Register the callback function to get progress feedback and cancelation.
    zip_register_progress_callback_with_state(archive.get(), 0.001, progressCallback, nullptr, this);
#if LIBZIP_CANCELATION
    zip_register_cancel_callback_with_state(archive.get(), cancelCallback, nullptr, this);
#endif

    // Write and close archive manually before using list() function.
    zip_close(archive.get());
    // Release unique pointer as it set to NULL via zip_close.
    archive.release();
    if (errcode > 0) {
        qCCritical(ARK) << "Failed to write archive";
        Q_EMIT error(xi18n("Failed to write archive."));
        return false;
    }

    if (QThread::currentThread()->isInterruptionRequested()) {
        return false;
    }

    // List the archive to update the model.
    m_listAfterAdd = true;
    list();

    qCDebug(ARK) << "Copied" << i << "entries";

    return true;
}

QString LibzipPlugin::fromUnixSeparator(const QString& path)
{
    if (!m_backslashedZip) {
        return path;
    }
    return QString(path).replace(QLatin1Char('/'), QLatin1Char('\\'));
}

QString LibzipPlugin::toUnixSeparator(const QString& path)
{
    // Even though the two contains may look similar they are not, the first is the \ char
    // that needs to be escaped, the second is the string with two \ that doesn't need escaping
    // so they look similar but they aren't
    if (path.contains(QLatin1Char('\\')) && !path.contains(QLatin1String("\\"))) {
        m_backslashedZip = true;
        return QString(path).replace(QLatin1Char('\\'), QLatin1Char('/'));
    }
    return path;
}

bool LibzipPlugin::hasBatchExtractionProgress() const
{
    return true;
}

bool LibzipPlugin::isReadOnly() const
{
    return isMultiVolume() || ReadWriteArchiveInterface::isReadOnly();
}

QString LibzipPlugin::multiVolumeName() const
{
    return m_multiVolumeName.isEmpty() ? filename() : m_multiVolumeName;
}

#include "libzipplugin.moc"
#include "moc_libzipplugin.cpp"
