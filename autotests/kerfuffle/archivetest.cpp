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
#include "kerfuffle/jobs.h"

#include <QDirIterator>
#include <QStandardPaths>
#include <QTest>

using namespace Kerfuffle;

class ArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testProperties_data();
    void testProperties();
    void testExtraction_data();
    void testExtraction();
    void testCreateEncryptedArchive();
};

QTEST_GUILESS_MAIN(ArchiveTest)

void ArchiveTest::testProperties_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedBaseName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("canFallbackOnReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");

    // Test non-existent tar archive.
    QString archivePath = QStringLiteral("/tmp/foo.tar.gz");
    QTest::newRow("non-existent tar archive")
            << archivePath
            << QStringLiteral("foo")
            << false << false << false << Archive::Unencrypted
            << QString();

    // Test non-archive file
    QTest::newRow("not an archive")
            << QStringLiteral("/tmp/foo.pdf")
            << QString()
            << false << false << false << Archive::Unencrypted
            << QString();

    // Test dummy source code tarball.
    archivePath = QFINDTESTDATA("data/code-x.y.z.tar.gz");
    QTest::newRow("dummy source code tarball")
            << archivePath
            << QStringLiteral("code-x.y.z")
            << false << false << true << Archive::Unencrypted
            << QStringLiteral("awesome_project");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("simple compressed tar archive")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/archivetest_encrypted.zip");
    QTest::newRow("encrypted zip, single entry")
            << archivePath
            << QStringLiteral("archivetest_encrypted")
            << false << true << false << Archive::Encrypted
            << QStringLiteral("archivetest_encrypted");

    archivePath = QFINDTESTDATA("data/archivetest_unencrypted.zip");
    QTest::newRow("simple zip, one unencrypted entry")
            << archivePath
            << QStringLiteral("archivetest_unencrypted")
            << false << true << false << Archive::Unencrypted
            << QStringLiteral("archivetest_unencrypted");

    archivePath = QFINDTESTDATA("data/wget.rpm");
    QTest::newRow("rpm archive, no single folder")
            << archivePath
            << QStringLiteral("wget")
            << true << false << false << Archive::Unencrypted
            << QStringLiteral("wget");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    QTest::newRow("bzip2-compressed tarball")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    QTest::newRow("xz-compressed tarball")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    QTest::newRow("lzma-compressed tarball")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    QTest::newRow("compress (.Z) tarball")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    QTest::newRow("lzipped tarball")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
    QTest::newRow("lzop-compressed tarball")
            << archivePath
            << QStringLiteral("simplearchive")
            << false << false << false << Archive::Unencrypted
            << QStringLiteral("simplearchive");

    // Only run test for lrzipped tar if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        QTest::newRow("lrzipped tarball")
                << archivePath
                << QStringLiteral("simplearchive")
                << false << false << false << Archive::Unencrypted
                << QStringLiteral("simplearchive");
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }

    QTest::newRow("mimetype child of application/zip")
            << QFINDTESTDATA("data/test.odt")
            << QStringLiteral("test")
            << false << true << false << Archive::Unencrypted
            << QStringLiteral("test");
}

void ArchiveTest::testProperties()
{
    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, this);
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
    QCOMPARE(archive->isSingleFolderArchive(), isSingleFolder);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);

    archive->deleteLater();
}

void ArchiveTest::testExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QVariantList>("entriesToExtract");
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
            << QVariantList()
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract selected entries from a tar.gz, without paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract selected entries from a tar.gz, preserve paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << optionsPreservePaths
            << 3;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    QTest::newRow("extract selected entries from a tar.gz, drag-and-drop")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir/")))
               }
            << dragAndDropOptions
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract the whole one_toplevel_folder.zip")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 9;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract selected entries from a zip, without paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract selected entries from a zip, preserve paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")))
               }
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    QTest::newRow("extract selected entries from a zip, drag-and-drop")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A/"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/C/"), QStringLiteral("A/B/"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/")))
               }
            << dragAndDropOptions
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract the whole one_toplevel_folder.7z")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 9;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract selected entries from a 7z, without paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract selected entries from a 7z, preserve paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")))
               }
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("extract selected entries from a 7z, drag-and-drop")
            << archivePath
            << QVariantList {QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test2.txt"), QStringLiteral("A/B/")))}
            << dragAndDropOptions
            << 1;

    archivePath = QFINDTESTDATA("data/empty_folders.zip");
    QTest::newRow("zip with empty folders")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 5;

    archivePath = QFINDTESTDATA("data/empty_folders.tar.gz");
    QTest::newRow("tar with empty folders")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 5;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    QTest::newRow("extract selected entries from a bzip2-compressed tarball without path")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    QTest::newRow("extract all entries from a bzip2-compressed tarball with path")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    QTest::newRow("extract selected entries from a xz-compressed tarball without path")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    QTest::newRow("extract all entries from a xz-compressed tarball with path")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    QTest::newRow("extract selected entries from a lzma-compressed tarball without path")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    QTest::newRow("extract all entries from a lzma-compressed tarball with path")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    QTest::newRow("extract selected entries from a compress (.Z)-compressed tarball without path")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    QTest::newRow("extract all entries from a compress (.Z)-compressed tarball with path")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    QTest::newRow("extract selected entries from a lzipped tarball without path")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    QTest::newRow("extract all entries from a lzipped tarball with path")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 7;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
    QTest::newRow("extract selected entries from a lzop-compressed tarball without path")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
               }
            << ExtractionOptions()
            << 2;

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
    QTest::newRow("extract all entries from a lzop-compressed tarball with path")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 7;

    // Only run test for lrzipped tar if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        QTest::newRow("extract selected entries from a lrzip-compressed tarball without path")
                << archivePath
                << QVariantList {
                       QVariant::fromValue(fileRootNodePair(QStringLiteral("file3.txt"), QString())),
                       QVariant::fromValue(fileRootNodePair(QStringLiteral("dir2/file22.txt"), QString()))
                   }
                << ExtractionOptions()
                << 2;

        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        QTest::newRow("extract all entries from a lrzip-compressed tarball with path")
                << archivePath
                << QVariantList()
                << optionsPreservePaths
                << 7;
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }
}

void ArchiveTest::testExtraction()
{
    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    QFETCH(QVariantList, entriesToExtract);
    QFETCH(ExtractionOptions, extractionOptions);
    auto extractionJob = archive->copyFiles(entriesToExtract, destDir.path(), extractionOptions);

    QEventLoop eventLoop(this);
    connect(extractionJob, &KJob::result, &eventLoop, &QEventLoop::quit);
    extractionJob->start();
    eventLoop.exec();

    // We need to wait for the QTemporaryDir in CliInterface::copyFiles() to autodelete itself.
    // TODO: find a better solution. Possibly related to task T1771 ?
    QTest::qSleep(250);

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    QDirIterator dirIt(destDir.path(), QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        extractedEntriesCount++;
        dirIt.next();
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);

    archive->deleteLater();
}

void ArchiveTest::testCreateEncryptedArchive()
{
    Archive *archive = Archive::create(QStringLiteral("foo.zip"));
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QCOMPARE(archive->encryptionType(), Archive::Unencrypted);

    archive->encrypt(QStringLiteral("1234"), false);
    QCOMPARE(archive->encryptionType(), Archive::Encrypted);

    archive->deleteLater();
}

#include "archivetest.moc"
