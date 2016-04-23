/*
 * Copyright (c) 2015 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include "kerfuffle/archive_kerfuffle.h"
#include "mimetypes.h"

#include <QTest>

using namespace Kerfuffle;

class MimeTypeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testMimeTypeDetection_data();
    void testMimeTypeDetection();
};

QTEST_GUILESS_MAIN(MimeTypeTest)

void MimeTypeTest::testMimeTypeDetection_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("expectedMimeType");

    const QString compressedGzipTarMime = QStringLiteral("application/x-compressed-tar");
    const QString compressedBzip2TarMime = QStringLiteral("application/x-bzip-compressed-tar");
    const QString compressedXzTarMime = QStringLiteral("application/x-xz-compressed-tar");
    const QString compressedLzmaTarMime = QStringLiteral("application/x-lzma-compressed-tar");
    const QString compressedZTarMime = QStringLiteral("application/x-tarz");
    const QString compressedLzipTarMime = QStringLiteral("application/x-lzip");
    const QString compressedLzopTarMime = QStringLiteral("application/x-tzo");
    const QString compressedLrzipTarMime = QStringLiteral("application/x-lrzip-compressed-tar");
    const QString isoMimeType = QStringLiteral("application/x-cd-image");

    QTest::newRow("empty name") << QString() << QStringLiteral("application/octet-stream");
    QTest::newRow("tar.gz") << QFINDTESTDATA("data/simplearchive.tar.gz") << compressedGzipTarMime;
    QTest::newRow("tar.bz2") << QFINDTESTDATA("data/simplearchive.tar.bz2") << compressedBzip2TarMime;
    QTest::newRow("tar.xz") << QFINDTESTDATA("data/simplearchive.tar.xz") << compressedXzTarMime;
    QTest::newRow("tar.lzma") << QFINDTESTDATA("data/simplearchive.tar.lzma") << compressedLzmaTarMime;
    QTest::newRow("tar.Z") << QFINDTESTDATA("data/simplearchive.tar.Z") << compressedZTarMime;
    QTest::newRow("tar.lz") << QFINDTESTDATA("data/simplearchive.tar.lz") << compressedLzipTarMime;
    QTest::newRow("tar.lzo") << QFINDTESTDATA("data/simplearchive.tar.lzo") << compressedLzopTarMime;
    QTest::newRow("tar.lrz") << QFINDTESTDATA("data/simplearchive.tar.lrz") << compressedLrzipTarMime;

    QTest::newRow("zip with wrong extension") << QFINDTESTDATA("data/zip_with_wrong_extension.rar") << QStringLiteral("application/zip");
    QTest::newRow("tar with special char in the extension") << QStringLiteral("foo.tar~1.gz") << compressedGzipTarMime;
    QTest::newRow("another tar with special char in the extension") << QStringLiteral("foo.ta4r.gz") << compressedGzipTarMime;
    QTest::newRow("tar downloaded by wget") << QFINDTESTDATA("data/wget-download.tar.gz.1") << compressedGzipTarMime;

    // This ISO file may be detected-by-content as text/plain. See https://bugs.freedesktop.org/show_bug.cgi?id=80877
    QTest::newRow("archlinux truncated ISO") << QFINDTESTDATA("data/archlinux-2015.09.01-dual_truncated.iso") << isoMimeType;

    // This ISO may not bet detected-by-content. See https://bugs.freedesktop.org/show_bug.cgi?id=80877
    QTest::newRow("kubuntu truncated ISO") << QFINDTESTDATA("data/kubuntu-14.04.1-desktop-amd64_truncated.iso") << isoMimeType;

    // Some mimetypes (e.g. tar-v7 archives, see #355955) cannot be detected by content (as of shared-mime-info 1.5).
    QTest::newRow("tar-v7") << QFINDTESTDATA("data/tar-v7.tar") << QStringLiteral("application/x-tar");
}

void MimeTypeTest::testMimeTypeDetection()
{
    QFETCH(QString, archiveName);
    QFETCH(QString, expectedMimeType);

    QCOMPARE(determineMimeType(archiveName).name(), expectedMimeType);
}

#include "mimetypetest.moc"
