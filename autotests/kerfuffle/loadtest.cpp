/*
 * Copyright (c) 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
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

#include "archive_kerfuffle.h"
#include "jobs.h"
#include "pluginmanager.h"
#include "testhelper.h"

#include <QStandardPaths>
#include <QTest>

using namespace Kerfuffle;

class LoadTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProperties_data();
    void testProperties();
};

QTEST_GUILESS_MAIN(LoadTest)

void LoadTest::testProperties_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedBaseName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("canFallbackOnReadOnly");
    QTest::addColumn<bool>("isSingleFile");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<bool>("isMultiVolume");
    QTest::addColumn<int>("numberOfVolumes");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");
    QTest::addColumn<QString>("expectedComment");

    // Test non-existent tar archive.
    QTest::newRow("non-existent tar archive")
            << QStringLiteral("/tmp/foo.tar.gz")
            << QStringLiteral("foo")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QString()
            << QString();

    // Test non-archive file
    QTest::newRow("not an archive")
            << QStringLiteral("/tmp/foo.pdf")
            << QString()
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QString()
            << QString();

    // Test dummy source code tarball.
    QTest::newRow("dummy source code tarball")
            << QFINDTESTDATA("data/code-x.y.z.tar.gz")
            << QStringLiteral("code-x.y.z")
            << false << false << false << true << false << 0 << Archive::Unencrypted
            << QStringLiteral("awesome_project")
            << QString();

    QTest::newRow("simple compressed tar archive")
            << QFINDTESTDATA("data/simplearchive.tar.gz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("encrypted zip, single entry")
            << QFINDTESTDATA("data/archivetest_encrypted.zip")
            << QStringLiteral("archivetest_encrypted")
            << false << true << true << false << false << 0 << Archive::Encrypted
            << QStringLiteral("archivetest_encrypted")
            << QString();

    QTest::newRow("simple zip, one unencrypted entry")
            << QFINDTESTDATA("data/archivetest_unencrypted.zip")
            << QStringLiteral("archivetest_unencrypted")
            << false << true << true << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("archivetest_unencrypted")
            << QString();

    QTest::newRow("rpm archive, no single folder")
            << QFINDTESTDATA("data/wget.rpm")
            << QStringLiteral("wget")
            << true << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("wget")
            << QString();

    QTest::newRow("bzip2-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.bz2")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("xz-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.xz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("lzma-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lzma")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("compress (.Z) tarball")
            << QFINDTESTDATA("data/simplearchive.tar.Z")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("lzipped tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    if (PluginManager().supportedMimeTypes().contains(QStringLiteral("application/x-tzo"))) {
        QTest::newRow("lzop-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lzo")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted
                << QStringLiteral("simplearchive")
                << QString();
    } else {
        qDebug() << "tar.lzo format not available. Skipping lzo test.";
    }

    // Only run test for lrzipped tar if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        QTest::newRow("lrzipped tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lrz")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted
                << QStringLiteral("simplearchive")
                << QString();
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }

    // Only run test for lz4-compressed tar if lz4 executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        QTest::newRow("lz4-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lz4")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted
                << QStringLiteral("simplearchive")
                << QString();
    } else {
        qDebug() << "lz4 executable not found in path. Skipping lz4 test.";
    }

    QTest::newRow("xar archive")
            << QFINDTESTDATA("data/simplearchive.xar")
            << QStringLiteral("simplearchive")
            << true << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("mimetype child of application/zip")
            << QFINDTESTDATA("data/test.odt")
            << QStringLiteral("test")
            << false << true << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("test")
            << QString();

    QTest::newRow("AppImage")
            << QFINDTESTDATA("data/hello-1.0-x86_64.AppImage")
            << QStringLiteral("hello-1.0-x86_64")
            << true << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("hello-1.0-x86_64")
            << QString();

    QTest::newRow("7z multivolume")
            << QFINDTESTDATA("data/archive-multivolume.7z.001")
            << QStringLiteral("archive-multivolume")
            << true << false << false << false << true << 3 << Archive::Unencrypted
            << QStringLiteral("archive-multivolume")
            << QString();

    QTest::newRow("rar multivolume")
            << QFINDTESTDATA("data/archive-multivolume.part1.rar")
            << QStringLiteral("archive-multivolume")
            << true << false << false << false << true << 3 << Archive::Unencrypted
            << QStringLiteral("archive-multivolume")
            << QString();

    QTest::newRow("zip with only an empty folder")
            << QFINDTESTDATA("data/single-empty-folder.zip")
            << QStringLiteral("single-empty-folder")
            << false << true << false << true << false << 0 << Archive::Unencrypted
            << QStringLiteral("empty")
            << QString();

    QTest::newRow("zip created by lineageos with comment")
            << QFINDTESTDATA("data/addonsu-remove-14.1-x86-signed.zip")
            << QStringLiteral("addonsu-remove-14.1-x86-signed")
            << false << true << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("addonsu-remove-14.1-x86-signed")
            << QStringLiteral("signed by SignApk");

    // Only run test for zstd-compressed tar if zstd executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("zstd")).isEmpty()) {
        QTest::newRow("zstd-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.zst")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted
                << QStringLiteral("simplearchive")
                << QString();
    } else {
        qDebug() << "zstd executable not found in path. Skipping zstd test.";
    }
}

void LoadTest::testProperties()
{
    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, this);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();

    QVERIFY(archive);

    if (!archive->isValid()) {
        QVERIFY(archive->fileName().isEmpty());
        QVERIFY(!archive->hasComment());
        QVERIFY(archive->error() != NoError);
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QFETCH(QString, expectedBaseName);
    QCOMPARE(archive->completeBaseName(), expectedBaseName);

    QFETCH(bool, isReadOnly);
    QFETCH(bool, canFallbackOnReadOnly);

    // If the plugin supports fallback on read-only mode, we cannot be sure at this point
    // if the archive is going to be read-write or read-only.
    if (!canFallbackOnReadOnly) {
        QCOMPARE(archive->isReadOnly(), isReadOnly);
    }

    QFETCH(bool, isSingleFile);
    QCOMPARE(archive->isSingleFile(), isSingleFile);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolder(), isSingleFolder);

    if (isSingleFile || isSingleFolder) {
        QVERIFY(!archive->hasMultipleTopLevelEntries());
    }

    QFETCH(bool, isMultiVolume);
    QCOMPARE(archive->isMultiVolume(), isMultiVolume);

    QFETCH(int, numberOfVolumes);
    QCOMPARE(archive->numberOfVolumes(), numberOfVolumes);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);

    QFETCH(QString, expectedComment);
    QCOMPARE(archive->hasComment(), !expectedComment.isEmpty());
    QCOMPARE(archive->comment(), expectedComment);

    loadJob->deleteLater();
    archive->deleteLater();
}


#include "loadtest.moc"
