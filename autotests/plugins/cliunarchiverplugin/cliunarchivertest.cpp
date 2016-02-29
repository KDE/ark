/*
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cliunarchivertest.h"
#include "kerfuffle/jobs.h"

#include <QDirIterator>
#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

QTEST_GUILESS_MAIN(CliUnarchiverTest)

using namespace Kerfuffle;

void CliUnarchiverTest::initTestCase()
{
    qRegisterMetaType<ArchiveEntry>();
}

void CliUnarchiverTest::testArchive_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<QString>("expectedSubfolderName");


    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("archive with one top-level folder")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << true << false
            << QStringLiteral("A");

    archivePath = QFINDTESTDATA("data/multiple_toplevel_entries.rar");
    QTest::newRow("archive with multiple top-level entries")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << false << false
            << QStringLiteral("multiple_toplevel_entries");

    archivePath = QFINDTESTDATA("data/encrypted_entries.rar");
    QTest::newRow("archive with encrypted entries")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << true << true
            << QStringLiteral("A");
}

void CliUnarchiverTest::testArchive()
{
    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle rar files. Skipping test.", SkipSingle);
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
}

void CliUnarchiverTest::testList_data()
{
    QTest::addColumn<QString>("jsonFilePath");
    QTest::addColumn<int>("expectedEntriesCount");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<qulonglong>("expectedCompressedSize");
    QTest::addColumn<QString>("expectedTimestamp");

    QTest::newRow("archive with one top-level folder")
            << QFINDTESTDATA("data/one_toplevel_folder.json") << 9
            << 6 << QStringLiteral("A/B/C/") << true << false << (qulonglong) 0 << (qulonglong) 0 << QStringLiteral("2015-12-21 16:57:20 +0100");
    QTest::newRow("archive with multiple top-level entries")
            << QFINDTESTDATA("data/multiple_toplevel_entries.json") << 12
            << 4 << QStringLiteral("data/A/B/test1.txt") << false << false << (qulonglong) 7 << (qulonglong) 7 << QStringLiteral("2015-12-21 16:56:28 +0100");
    QTest::newRow("archive with encrypted entries")
            << QFINDTESTDATA("data/encrypted_entries.json") << 9
            << 5 << QStringLiteral("A/test1.txt") << false << true << (qulonglong) 7 << (qulonglong) 32 << QStringLiteral("2015-12-21 16:56:28 +0100");
    QTest::newRow("huge archive")
            << QFINDTESTDATA("data/huge_archive.json") << 250
            << 8 << QStringLiteral("PsycOPacK/Base Dictionnaries/att800") << false << false << (qulonglong) 593687 << (qulonglong) 225219 << QStringLiteral("2011-08-14 03:10:10 +0200");
}

void CliUnarchiverTest::testList()
{
    CliPlugin *unarPlugin = new CliPlugin(this, {QStringLiteral("dummy.rar")});
    QSignalSpy signalSpy(unarPlugin, SIGNAL(entry(ArchiveEntry)));

    QFETCH(QString, jsonFilePath);
    QFETCH(int, expectedEntriesCount);

    QFile jsonFile(jsonFilePath);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));

    QTextStream stream(&jsonFile);
    unarPlugin->setJsonOutput(stream.readAll());

    QCOMPARE(signalSpy.count(), expectedEntriesCount);

    QFETCH(int, someEntryIndex);
    QVERIFY(someEntryIndex < signalSpy.count());
    ArchiveEntry entry = qvariant_cast<ArchiveEntry>(signalSpy.at(someEntryIndex).at(0));

    QFETCH(QString, expectedName);
    QCOMPARE(entry[FileName].toString(), expectedName);

    QFETCH(bool, isDirectory);
    QCOMPARE(entry[IsDirectory].toBool(), isDirectory);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(entry[IsPasswordProtected].toBool(), isPasswordProtected);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry[Size].toULongLong(), expectedSize);

    QFETCH(qulonglong, expectedCompressedSize);
    QCOMPARE(entry[CompressedSize].toULongLong(), expectedCompressedSize);

    QFETCH(QString, expectedTimestamp);
    QCOMPARE(entry[Timestamp].toString(), expectedTimestamp);

    unarPlugin->deleteLater();
}

void CliUnarchiverTest::testExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QVariantList>("entriesToExtract");
    QTest::addColumn<ExtractionOptions>("extractionOptions");
    QTest::addColumn<int>("expectedExtractedEntriesCount");

    ExtractionOptions options;
    options[QStringLiteral("AlwaysUseTmpDir")] = true;

    ExtractionOptions optionsPreservePaths = options;
    optionsPreservePaths[QStringLiteral("PreservePaths")] = true;

    // Just for clarity.
    ExtractionOptions dragAndDropOptions = optionsPreservePaths;

    QString archivePath = QFINDTESTDATA("data/multiple_toplevel_entries.rar");
    QTest::newRow("extract the whole multiple_toplevel_entries.rar")
            << archivePath
            << QVariantList()
            << optionsPreservePaths
            << 12;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("extract selected entries from a rar, without paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")))
               }
            << options
            << 2;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("extract selected entries from a rar, preserve paths")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")))
               }
            << optionsPreservePaths
            << 4;

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("extract selected entries from a rar, drag-and-drop")
            << archivePath
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/C/"), QStringLiteral("A/B/"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/test2.txt"), QStringLiteral("A/"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/")))
               }
            << dragAndDropOptions
            << 4;
}

// TODO: we can remove this test (which is duplicated from kerfuffle/archivetest)
// if we ever ends up using a temp dir for any cliplugin, instead of only for cliunarchiver.
void CliUnarchiverTest::testExtraction()
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
