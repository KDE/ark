/*
    SPDX-FileCopyrightText: 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
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
    QTest::addColumn<int>("numberOfEntries");
    QTest::addColumn<QString>("expectedSubfolderName");
    QTest::addColumn<QString>("expectedComment");

    // Test non-existent tar archive.
    QTest::newRow("non-existent tar archive")
            << QStringLiteral("/tmp/foo.tar.gz")
            << QStringLiteral("foo")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 0
            << QString()
            << QString();

    // Test non-archive file
    QTest::newRow("not an archive")
            << QStringLiteral("/tmp/foo.pdf")
            << QString()
            << false << false << false << false << false << 0 << Archive::Unencrypted << 0
            << QString()
            << QString();

    // Test dummy source code tarball.
    QTest::newRow("dummy source code tarball")
            << QFINDTESTDATA("data/code-x.y.z.tar.gz")
            << QStringLiteral("code-x.y.z")
            << false << false << false << true << false << 0 << Archive::Unencrypted << 3
            << QStringLiteral("awesome_project")
            << QString();

    QTest::newRow("simple compressed tar archive")
            << QFINDTESTDATA("data/simplearchive.tar.gz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 4
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("simple C++ static library")
            << QFINDTESTDATA("data/libdummy.a")
            << QStringLiteral("libdummy")
            // NOTE: there are 2 objects in this library, but libarchive also reports the "archive symbol table"
            // as a separate entry with path "/", which is then skipped by the ArchiveModel.
            << true << false << false << false << false << 0 << Archive::Unencrypted << 2+1
            << QStringLiteral("libdummy")
            << QString();

    QTest::newRow("encrypted zip, single entry")
            << QFINDTESTDATA("data/archivetest_encrypted.zip")
            << QStringLiteral("archivetest_encrypted")
            << false << true << true << false << false << 0 << Archive::Encrypted << 1
            << QStringLiteral("archivetest_encrypted")
            << QString();

    QTest::newRow("simple zip, one unencrypted entry")
            << QFINDTESTDATA("data/archivetest_unencrypted.zip")
            << QStringLiteral("archivetest_unencrypted")
            << false << true << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("archivetest_unencrypted")
            << QString();

    QTest::newRow("rpm archive, no single folder")
            << QFINDTESTDATA("data/wget.rpm")
            << QStringLiteral("wget")
            // NOTE: there are 49 files in this RPM, but for some reason libarchive reports "./usr/share/doc/wget" as separate entry...
            << true << false << false << false << false << 0 << Archive::Unencrypted << 49+1
            << QStringLiteral("wget")
            << QString();

    QTest::newRow("bzip2-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.bz2")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 5
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("xz-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.xz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 5
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("lzma-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lzma")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 5
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("compress (.Z) tarball")
            << QFINDTESTDATA("data/simplearchive.tar.Z")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 7
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("lzipped tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 5
            << QStringLiteral("simplearchive")
            << QString();

    // Only run tests for lzop compressed files if tar.lzo format is available
    if (PluginManager().supportedMimeTypes().contains(QLatin1String("application/x-tzo"))) {
        QTest::newRow("lzop-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lzo")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted << 5
                << QStringLiteral("simplearchive")
                << QString();

        QTest::newRow("single-file lzop compressed")
                << QFINDTESTDATA("data/test.png.lzo")
                << QStringLiteral("test.png")
                << true << false << true << false << false << 0 << Archive::Unencrypted << 1
                << QStringLiteral("test.png")
                << QString();
    } else {
        qDebug() << "tar.lzo format not available. Skipping lzo test.";
    }

    // Only run tests for lrzipped files if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        QTest::newRow("lrzipped tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lrz")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted << 5
                << QStringLiteral("simplearchive")
                << QString();

        QTest::newRow("single-file lrzip compressed")
                << QFINDTESTDATA("data/test.txt.lrz")
                << QStringLiteral("test.txt")
                << true << false << true << false << false << 0 << Archive::Unencrypted << 1
                << QStringLiteral("test.txt")
                << QString();
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }

    // Only run tests for lz4-compressed files if lz4 executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        QTest::newRow("lz4-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lz4")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted << 5
                << QStringLiteral("simplearchive")
                << QString();

        QTest::newRow("single-file lz4 compressed")
                << QFINDTESTDATA("data/test.txt.lz4")
                << QStringLiteral("test.txt")
                << true << false << true << false << false << 0 << Archive::Unencrypted << 1
                << QStringLiteral("test.txt")
                << QString();
    } else {
        qDebug() << "lz4 executable not found in path. Skipping lz4 test.";
    }

    QTest::newRow("xar archive")
            << QFINDTESTDATA("data/simplearchive.xar")
            << QStringLiteral("simplearchive")
            << true << false << false << false << false << 0 << Archive::Unencrypted << 6
            << QStringLiteral("simplearchive")
            << QString();

    QTest::newRow("mimetype child of application/zip")
            << QFINDTESTDATA("data/test.odt")
            << QStringLiteral("test")
            << false << true << false << false << false << 0 << Archive::Unencrypted << 17
            << QStringLiteral("test")
            << QString();

    QTest::newRow("AppImage")
            << QFINDTESTDATA("data/hello-1.0-x86_64.AppImage")
            << QStringLiteral("hello-1.0-x86_64")
            // NOTE: there are 7 files in this AppImage, but libarchive reports "." as separate entry which is then skipped by the ArchiveModel.
            << true << false << false << false << false << 0 << Archive::Unencrypted << 7+1
            << QStringLiteral("hello-1.0-x86_64")
            << QString();

    QTest::newRow("7z multivolume")
            << QFINDTESTDATA("data/archive-multivolume.7z.001")
            << QStringLiteral("archive-multivolume")
            << true << false << false << false << true << 3 << Archive::Unencrypted << 3
            << QStringLiteral("archive-multivolume")
            << QString();

    QTest::newRow("zip with only an empty folder")
            << QFINDTESTDATA("data/single-empty-folder.zip")
            << QStringLiteral("single-empty-folder")
            << false << true << false << true << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("empty")
            << QString();

    QTest::newRow("zip created by lineageos with comment")
            << QFINDTESTDATA("data/addonsu-remove-14.1-x86-signed.zip")
            << QStringLiteral("addonsu-remove-14.1-x86-signed")
            << false << true << false << false << false << 0 << Archive::Unencrypted << 7
            << QStringLiteral("addonsu-remove-14.1-x86-signed")
            << QStringLiteral("signed by SignApk");

    // Only run tests for zstd-compressed files if zstd executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("zstd")).isEmpty()) {
        QTest::newRow("zstd-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.zst")
                << QStringLiteral("simplearchive")
                << false << false << false << false << false << 0 << Archive::Unencrypted << 8
                << QStringLiteral("simplearchive")
                << QString();

        QTest::newRow("single-file zstd compressed")
                << QFINDTESTDATA("data/test.txt.zst")
                << QStringLiteral("test.txt")
                << true << false << true << false << false << 0 << Archive::Unencrypted << 1
                << QStringLiteral("test.txt")
                << QString();
    } else {
        qDebug() << "zstd executable not found in path. Skipping zstd test.";
    }

    QTest::newRow("arj unencrypted archive with comment")
            << QFINDTESTDATA("data/test.arj")
            << QStringLiteral("test")
            << false << false << false << false << false << 0 << Archive::Unencrypted << 13
            << QStringLiteral("test")
            << QStringLiteral("Arj archive");

    QTest::newRow("arj encrypted archive")
            << QFINDTESTDATA("data/test_encrypted.arj")
            << QStringLiteral("test_encrypted")
            << false << false << false << false << false << 0 << Archive::Encrypted << 9
            << QStringLiteral("test_encrypted")
            << QString();

    QTest::newRow("single-file UNIX-compressed")
            << QFINDTESTDATA("data/test.z")
            << QStringLiteral("test")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test")
            << QString();

    QTest::newRow("single-file zlib compressed")
            << QFINDTESTDATA("data/test.zz")
            << QStringLiteral("test")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test")
            << QString();

    QTest::newRow("single-file gz compressed")
            << QFINDTESTDATA("data/test.txt.gz")
            << QStringLiteral("test.txt")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test.txt")
            << QString();

    QTest::newRow("single-file bzip compressed")
            << QFINDTESTDATA("data/test.txt.bz2")
            << QStringLiteral("test.txt")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test.txt")
            << QString();

    QTest::newRow("single-file lzma compressed")
            << QFINDTESTDATA("data/test.png.lzma")
            << QStringLiteral("test.png")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test.png")
            << QString();

    QTest::newRow("single-file compressed SVG")
            << QFINDTESTDATA("data/test.svgz")
            << QStringLiteral("test")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test")
            << QString();

    QTest::newRow("stuffit unencrypted archive")
            << QFINDTESTDATA("data/test.sit")
            << QStringLiteral("test")
            << true << false << true << false << false << 0 << Archive::Unencrypted << 1
            << QStringLiteral("test")
            << QString();
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

    QFETCH(int, numberOfEntries);
    QCOMPARE(archive->numberOfEntries(), numberOfEntries);

    loadJob->deleteLater();
    archive->deleteLater();
}


#include "loadtest.moc"
