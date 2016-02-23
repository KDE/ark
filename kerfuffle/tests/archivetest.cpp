/*
 * Copyright (c) 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

class ArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProperties_data();
    void testProperties();
};

QTEST_GUILESS_MAIN(ArchiveTest)

void ArchiveTest::testProperties_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<QString>("expectedSubfolderName");

    // Test non-existent tar archive.
    QString archivePath = QStringLiteral("/tmp/foo.tar.gz");
    QTest::newRow("non-existent tar archive")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << true << false
            << QStringLiteral("foo");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("simple compressed tar archive")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << false << false
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/archivetest_encrypted.zip");
    QTest::newRow("encrypted zip, single entry")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << false << true
            // FIXME: possibly a bug? I was expecting to get "archivetest_encrypted" as subfolder name...
            << QStringLiteral("foo.txt");

    archivePath = QFINDTESTDATA("data/archivetest_unencrypted.zip");
    QTest::newRow("simple zip, one unencrypted entry")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << false << false
            // FIXME: possibly a bug? I was expecting to get "archivetest_encrypted" as subfolder name...
            << QStringLiteral("foo.txt");
}

void ArchiveTest::testProperties()
{
    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QFETCH(QString, expectedFileName);
    QCOMPARE(QFileInfo(archive->fileName()).fileName(), expectedFileName);

    QFETCH(bool, isReadOnly);
    QCOMPARE(archive->isReadOnly(), isReadOnly);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolderArchive(), isSingleFolder);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(archive->isPasswordProtected(), isPasswordProtected);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);

    archive->deleteLater();
}

#include "archivetest.moc"
