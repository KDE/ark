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

#include "mimetypetest.moc"
