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

#include "testhelper.h"

#include <QDirIterator>
#include <QStandardPaths>
#include <QTest>

using namespace Kerfuffle;

class ExtractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProperties_data();
    void testProperties();
    void testExtraction_data();
    void testExtraction();
};

QTEST_GUILESS_MAIN(ExtractTest)

void ExtractTest::testProperties_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedBaseName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("canFallbackOnReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<bool>("isMultiVolume");
    QTest::addColumn<int>("numberOfVolumes");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");

    // Test non-existent tar archive.
    QTest::newRow("non-existent tar archive")
            << QStringLiteral("/tmp/foo.tar.gz")
            << QStringLiteral("foo")
            << false << false << true << false << 0 << Archive::Unencrypted
            << QStringLiteral("foo");

    // Test non-archive file
    QTest::newRow("not an archive")
            << QStringLiteral("/tmp/foo.pdf")
            << QString()
            << false << false << false << false << 0 << Archive::Unencrypted
            << QString();

    // Test dummy source code tarball.
    QTest::newRow("dummy source code tarball")
            << QFINDTESTDATA("data/code-x.y.z.tar.gz")
            << QStringLiteral("code-x.y.z")
            << false << false << true << false << 0 << Archive::Unencrypted
            << QStringLiteral("awesome_project");

    QTest::newRow("simple compressed tar archive")
            << QFINDTESTDATA("data/simplearchive.tar.gz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("encrypted zip, single entry")
            << QFINDTESTDATA("data/archivetest_encrypted.zip")
            << QStringLiteral("archivetest_encrypted")
            << false << true << false << false << 0 << Archive::Encrypted
            << QStringLiteral("archivetest_encrypted");

    QTest::newRow("simple zip, one unencrypted entry")
            << QFINDTESTDATA("data/archivetest_unencrypted.zip")
            << QStringLiteral("archivetest_unencrypted")
            << false << true << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("archivetest_unencrypted");

    QTest::newRow("rpm archive, no single folder")
            << QFINDTESTDATA("data/wget.rpm")
            << QStringLiteral("wget")
            << true << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("wget");

    QTest::newRow("bzip2-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.bz2")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("xz-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.xz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("lzma-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lzma")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("compress (.Z) tarball")
            << QFINDTESTDATA("data/simplearchive.tar.Z")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("lzipped tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lz")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("lzop-compressed tarball")
            << QFINDTESTDATA("data/simplearchive.tar.lzo")
            << QStringLiteral("simplearchive")
            << false << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    // Only run test for lrzipped tar if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        QTest::newRow("lrzipped tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lrz")
                << QStringLiteral("simplearchive")
                << false << false << false << false << 0 << Archive::Unencrypted
                << QStringLiteral("simplearchive");
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }

    // Only run test for lz4-compressed tar if lz4 executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        QTest::newRow("lz4-compressed tarball")
                << QFINDTESTDATA("data/simplearchive.tar.lz4")
                << QStringLiteral("simplearchive")
                << false << false << false << false << 0 << Archive::Unencrypted
                << QStringLiteral("simplearchive");
    } else {
        qDebug() << "lz4 executable not found in path. Skipping lz4 test.";
    }

    QTest::newRow("xar archive")
            << QFINDTESTDATA("data/simplearchive.xar")
            << QStringLiteral("simplearchive")
            << true << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    QTest::newRow("mimetype child of application/zip")
            << QFINDTESTDATA("data/test.odt")
            << QStringLiteral("test")
            << false << true << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("test");

    QTest::newRow("AppImage")
            << QFINDTESTDATA("data/hello-1.0-x86_64.AppImage")
            << QStringLiteral("hello-1.0-x86_64")
            << true << false << false << false << 0 << Archive::Unencrypted
            << QStringLiteral("hello-1.0-x86_64");

    QTest::newRow("7z multivolume")
            << QFINDTESTDATA("data/archive-multivolume.7z.001")
            << QStringLiteral("archive-multivolume")
            << true << false << false << true << 3 << Archive::Unencrypted
            << QStringLiteral("archive-multivolume");

    QTest::newRow("rar multivolume")
            << QFINDTESTDATA("data/archive-multivolume.part1.rar")
            << QStringLiteral("archive-multivolume")
            << true << false << false << true << 3 << Archive::Unencrypted
            << QStringLiteral("archive-multivolume");
}

void ExtractTest::testProperties()
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

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolder(), isSingleFolder);

    QFETCH(bool, isMultiVolume);
    QCOMPARE(archive->isMultiVolume(), isMultiVolume);

    QFETCH(int, numberOfVolumes);
    QCOMPARE(archive->numberOfVolumes(), numberOfVolumes);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);

    loadJob->deleteLater();
    archive->deleteLater();
}

void ExtractTest::testExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QList<Archive::Entry*>>("entriesToExtract");
    QTest::addColumn<ExtractionOptions>("extractionOptions");
    QTest::addColumn<int>("expectedExtractedEntriesCount");

    ExtractionOptions optionsPreservePaths;
    optionsPreservePaths[QStringLiteral("PreservePaths")] = true;

    ExtractionOptions dragAndDropOptions = optionsPreservePaths;
    dragAndDropOptions[QStringLiteral("DragAndDrop")] = true;
    dragAndDropOptions[QStringLiteral("RemoveRootNode")] = true;

    QString archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract the whole simplearchive.tar.gz")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract selected entries from a tar.gz, without paths")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract selected entries from a tar.gz, preserve paths")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << optionsPreservePaths
            << 3;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract selected entries from a tar.gz, drag-and-drop")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir/"))
               }
            << dragAndDropOptions
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract the whole one_toplevel_folder.zip")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 9;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract selected entries from a zip, without paths")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                   new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract selected entries from a zip, preserve paths")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                   new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))
               }
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract selected entries from a zip, drag-and-drop")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A/")),
                   new Archive::Entry(this, QStringLiteral("A/B/C/"), QStringLiteral("A/B/")),
                   new Archive::Entry(this, QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/")),
                   new Archive::Entry(this, QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/"))
               }
            << dragAndDropOptions
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract the whole one_toplevel_folder.7z")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 9;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract selected entries from a 7z, without paths")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                   new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract selected entries from a 7z, preserve paths")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                   new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))
               }
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract selected entries from a 7z, drag-and-drop")
            << archivePath
            << QList<Archive::Entry*> {new Archive::Entry(this, QStringLiteral("A/B/test2.txt"), QStringLiteral("A/B/"))}
            << dragAndDropOptions
            << 1;

    archivePath = QFINDTESTDATA("data/empty_folders.zip");
    QTest::newRow("zip with empty folders")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 5;

    archivePath = QFINDTESTDATA("data/empty_folders.tar.gz");
    QTest::newRow("tar with empty folders")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 5;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    QTest::newRow("extract selected entries from a bzip2-compressed tarball without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    QTest::newRow("extract all entries from a bzip2-compressed tarball with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    QTest::newRow("extract selected entries from a xz-compressed tarball without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    QTest::newRow("extract all entries from a xz-compressed tarball with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    QTest::newRow("extract selected entries from a lzma-compressed tarball without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    QTest::newRow("extract all entries from a lzma-compressed tarball with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    QTest::newRow("extract selected entries from a compress (.Z)-compressed tarball without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    QTest::newRow("extract all entries from a compress (.Z)-compressed tarball with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    QTest::newRow("extract selected entries from a lzipped tarball without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    QTest::newRow("extract all entries from a lzipped tarball with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
    QTest::newRow("extract selected entries from a lzop-compressed tarball without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
    QTest::newRow("extract all entries from a lzop-compressed tarball with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    // Only run test for lrzipped tar if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        QTest::newRow("extract selected entries from a lrzip-compressed tarball without path")
                << archivePath
                << QList<Archive::Entry*> {
                       new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                       new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
                   }
                << ExtractionOptions()
                << 2;

        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        QTest::newRow("extract all entries from a lrzip-compressed tarball with path")
                << archivePath
                << QList<Archive::Entry*>()
                << optionsPreservePaths
                << 7;
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }

    // Only run test for lz4-compressed tar if lz4 executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lz4");
        QTest::newRow("extract selected entries from a lz4-compressed tarball without path")
                << archivePath
                << QList<Archive::Entry*> {
                       new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                       new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString())
                   }
                << ExtractionOptions()
                << 2;

        archivePath = QFINDTESTDATA("data/simplearchive.tar.lz4");
        QTest::newRow("extract all entries from a lz4-compressed tarball with path")
                << archivePath
                << QList<Archive::Entry*>()
                << optionsPreservePaths
                << 7;
    } else {
        qDebug() << "lz4 executable not found in path. Skipping lz4 test.";
    }

    archivePath = QFINDTESTDATA("data/simplearchive.xar");
    QTest::newRow("extract selected entries from a xar archive without path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("dir1/file11.txt"), QString()),
                   new Archive::Entry(this, QStringLiteral("file4.txt"), QString())
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.xar");
    QTest::newRow("extract all entries from a xar archive with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 6;

    archivePath = QFINDTESTDATA("data/hello-1.0-x86_64.AppImage");
    QTest::newRow("extract all entries from an AppImage with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/hello-1.0-x86_64.AppImage");
    QTest::newRow("extract selected entries from an AppImage with path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("usr/bin/hello"), QString())
               }
            << optionsPreservePaths
            << 3;

    archivePath = QFINDTESTDATA("data/archive-multivolume.7z.001");
    QTest::newRow("extract all entries from a multivolume 7z archive with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 3;

    archivePath = QFINDTESTDATA("data/archive-multivolume.part1.rar");
    QTest::newRow("extract all entries from a multivolume rar archive with path")
            << archivePath
            << QList<Archive::Entry*>()
            << optionsPreservePaths
            << 3;

    archivePath = QFINDTESTDATA("data/firmware-pine64-20160329-6.1.aarch64.rpm");
    QTest::newRow("extract selected entries from rpm with path")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("boot/sunxi-spl.bin"), QString())
               }
            << optionsPreservePaths
            << 2;

    archivePath = QFINDTESTDATA("data/firmware-pine64-20160329-6.1.aarch64.rpm");
    QTest::newRow("#369535: broken drag-and-drop from rpm")
            << archivePath
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("boot/sunxi-spl.bin"), QStringLiteral("boot/")),
                   new Archive::Entry(this, QStringLiteral("boot/u-boot.img"), QStringLiteral("boot/"))
               }
            << dragAndDropOptions
            << 2;
}

void ExtractTest::testExtraction()
{
    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, this);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    Archive *archive = Q_NULLPTR;
    TestHelper::startAndWaitForResult(loadJob);
    archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    QFETCH(QList<Archive::Entry*>, entriesToExtract);
    QFETCH(ExtractionOptions, extractionOptions);
    auto extractionJob = archive->extractFiles(entriesToExtract, destDir.path(), extractionOptions);
    QVERIFY(extractionJob);
    extractionJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(extractionJob);

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    QDirIterator dirIt(destDir.path(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        extractedEntriesCount++;
        dirIt.next();
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);

    loadJob->deleteLater();
    extractionJob->deleteLater();
    archive->deleteLater();
}

#include "extracttest.moc"
