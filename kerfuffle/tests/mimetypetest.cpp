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

#include <QTest>

using namespace Kerfuffle;

class MimeTypeTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testEmptyFilename();
    void testTarDetection();
    void testWrongZipExtension();
    void testSpecialCharsTarExtension();
    void testIsoDetection();
};

QTEST_GUILESS_MAIN(MimeTypeTest)

void MimeTypeTest::testEmptyFilename()
{
    QCOMPARE(Archive::determineMimeType(QString()), QStringLiteral("application/octet-stream"));
}

void MimeTypeTest::testTarDetection()
{
    const QString testFile = QFINDTESTDATA("data/simplearchive.tar.gz");
    QCOMPARE(Archive::determineMimeType(testFile), QStringLiteral("application/x-compressed-tar"));
}

void MimeTypeTest::testWrongZipExtension()
{
    const QString testFile = QFINDTESTDATA("data/zip_with_wrong_extension.rar");
    QCOMPARE(Archive::determineMimeType(testFile), QStringLiteral("application/zip"));
}

void MimeTypeTest::testSpecialCharsTarExtension()
{
    const QString tarMimeType = QStringLiteral("application/x-compressed-tar");
    QCOMPARE(Archive::determineMimeType(QStringLiteral("foo.tar~1.gz")), tarMimeType);
    QCOMPARE(Archive::determineMimeType(QStringLiteral("foo.ta4r.gz")), tarMimeType);
}

void MimeTypeTest::testIsoDetection()
{
    const QString isoMimeType = QStringLiteral("application/x-cd-image");

    // Test workaround for https://bugs.freedesktop.org/show_bug.cgi?id=80877
    // 1. This ISO file may be detected-by-content as text/plain.
    const QString archIso = QFINDTESTDATA("data/archlinux-2015.09.01-dual_truncated.iso");
    QCOMPARE(Archive::determineMimeType(archIso), isoMimeType);
    // 2. This ISO may not bet detected-by-content.
    const QString kubuntuIso = QFINDTESTDATA("data/kubuntu-14.04.1-desktop-amd64_truncated.iso");
    QCOMPARE(Archive::determineMimeType(kubuntuIso), isoMimeType);
}

#include "mimetypetest.moc"
