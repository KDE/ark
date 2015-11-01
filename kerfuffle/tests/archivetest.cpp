/*
 * Copyright (c) 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include <QtTest>

class ArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testFileName();
    void testIsPasswordProtected();
    void testOpenNonExistentFile();
};

QTEST_GUILESS_MAIN(ArchiveTest)

void ArchiveTest::testFileName()
{
    Kerfuffle::Archive *archive = Kerfuffle::Archive::create(QStringLiteral("/tmp/foo.tar.gz"), this);

    QVERIFY(archive);

    QCOMPARE(archive->fileName(), QLatin1String("/tmp/foo.tar.gz"));
}

void ArchiveTest::testIsPasswordProtected()
{
    Kerfuffle::Archive *archive;

    archive = Kerfuffle::Archive::create(QFINDTESTDATA("data/archivetest_encrypted.zip"), this);

    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle zip files. Skipping test.", SkipSingle);
    }

    QVERIFY(archive->isValid());

    QVERIFY(archive->isPasswordProtected());

    archive->deleteLater();

    archive = Kerfuffle::Archive::create(QFINDTESTDATA("data/archivetest_unencrypted.zip"), this);

    QVERIFY(!archive->isPasswordProtected());
}

void ArchiveTest::testOpenNonExistentFile()
{
    QSKIP("How should we deal with files that do not exist? Should factory() return NULL?", SkipSingle);
}

#include "archivetest.moc"
