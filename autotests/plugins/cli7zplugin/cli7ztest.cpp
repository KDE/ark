/*
  * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "cli7ztest.h"
#include "cliplugin.h"
#include "archive_kerfuffle.h"
#include "jobs.h"
#include "testhelper.h"

#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#include <KPluginLoader>

QTEST_GUILESS_MAIN(Cli7zTest)

using namespace Kerfuffle;

void Cli7zTest::initTestCase()
{
    m_plugin = new Plugin(this);
    foreach (Plugin *plugin, m_pluginManger.availablePlugins()) {
        if (plugin->metaData().pluginId() == QStringLiteral("kerfuffle_cli7z")) {
            m_plugin = plugin;
            return;
        }
    }
}

void Cli7zTest::testArchive_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");

    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    QTest::newRow("archive with one top-level folder")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << true << Archive::Unencrypted
            << QStringLiteral("A");
}

void Cli7zTest::testArchive()
{
    if (!m_plugin->isValid()) {
        QSKIP("cli7z plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cli7z plugin. Skipping test.", SkipSingle);
    }

    QFETCH(QString, expectedFileName);
    QCOMPARE(QFileInfo(archive->fileName()).fileName(), expectedFileName);

    QFETCH(bool, isReadOnly);
    QCOMPARE(archive->isReadOnly(), isReadOnly);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolder(), isSingleFolder);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(QString, expectedSubfolderName);
    QCOMPARE(archive->subfolderName(), expectedSubfolderName);
}

void Cli7zTest::testList_data()
{
    QTest::addColumn<QString>("outputTextFile");
    QTest::addColumn<int>("expectedEntriesCount");
    QTest::addColumn<bool>("isMultiVolume");
    // Is zero for non-multi-volume archives:
    QTest::addColumn<int>("numberOfVolumes");
    QTest::addColumn<QStringList>("compressionMethods");
    QTest::addColumn<QString>("encryptionMethod");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<QString>("expectedTimestamp");

    // p7zip version 16.02 tests

    QTest::newRow("normal-file-1602")
            << QFINDTESTDATA("data/archive-with-symlink-1602.txt") << 10 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QString()
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T20:41:48");

    QTest::newRow("encrypted-1602")
            << QFINDTESTDATA("data/archive-encrypted-1602.txt") << 4 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QStringLiteral("7zAES")
            << 1 << QStringLiteral("file2.txt") << false << true << (qulonglong) 14 << QStringLiteral("2016-03-02T22:37:55");

    QTest::newRow("multi-volume-1602")
            << QFINDTESTDATA("data/archive-multivol-1602.txt") << 2 << true << 5 << QStringList{QStringLiteral("LZMA2")} << QString()
            << 1 << QStringLiteral("largefile2") << false << false << (qulonglong) 2097152 << QStringLiteral("2016-07-17T11:26:19");

    QTest::newRow("zip-with-AES256-encryption")
            << QFINDTESTDATA("data/archive-zip-AES256-1602.txt") << 4 << false << 0
            << QStringList{QStringLiteral("Store"), QStringLiteral("Store"), QStringLiteral("Deflate"), QStringLiteral("Deflate")}
            << QStringLiteral("AES256")
            << 3 << QStringLiteral("testarchive/file2.txt") << false << true << (qulonglong) 33 << QStringLiteral("2016-11-06T21:17:02");

    // p7zip version 15.14 tests

    QTest::newRow("normal-file-1514")
            << QFINDTESTDATA("data/archive-with-symlink-1514.txt") << 10 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QString()
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-1514")
            << QFINDTESTDATA("data/archive-encrypted-1514.txt") << 9 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QStringLiteral("7zAES")
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    // p7zip version 15.09 tests

    QTest::newRow("normal-file-1509")
            << QFINDTESTDATA("data/archive-with-symlink-1509.txt") << 10 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QString()
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-1509")
            << QFINDTESTDATA("data/archive-encrypted-1509.txt") << 9 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QStringLiteral("7zAES")
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    // p7zip version 9.38.1 tests

    QTest::newRow("normal-file-9381")
            << QFINDTESTDATA("data/archive-with-symlink-9381.txt") << 10 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QString()
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-9381")
            << QFINDTESTDATA("data/archive-encrypted-9381.txt") << 9 << false << 0 << QStringList{QStringLiteral("LZMA2")} << QStringLiteral("7zAES")
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");
}

void Cli7zTest::testList()
{
    qRegisterMetaType<Archive::Entry*>("Archive::Entry*");
    CliPlugin *plugin = new CliPlugin(this, {QStringLiteral("dummy.7z"),
                                             QVariant::fromValue(m_plugin->metaData())});
    QSignalSpy signalSpyEntry(plugin, &CliPlugin::entry);
    QSignalSpy signalSpyCompMethod(plugin, &CliPlugin::compressionMethodFound);
    QSignalSpy signalSpyEncMethod(plugin, &CliPlugin::encryptionMethodFound);

    QFETCH(QString, outputTextFile);
    QFETCH(int, expectedEntriesCount);

    QFile outputText(outputTextFile);
    QVERIFY(outputText.open(QIODevice::ReadOnly));

    QTextStream outputStream(&outputText);
    while (!outputStream.atEnd()) {
        const QString line(outputStream.readLine());
        QVERIFY(plugin->readListLine(line));
    }

    QCOMPARE(signalSpyEntry.count(), expectedEntriesCount);

    QFETCH(bool, isMultiVolume);
    QCOMPARE(plugin->isMultiVolume(), isMultiVolume);

    QFETCH(int, numberOfVolumes);
    QCOMPARE(plugin->numberOfVolumes(), numberOfVolumes);

    QFETCH(QStringList, compressionMethods);
    QCOMPARE(signalSpyCompMethod.count(), compressionMethods.count());
    QStringList combinedCompMethods;
    for (int i = 0; i < signalSpyCompMethod.count(); i++) {
        combinedCompMethods.append(signalSpyCompMethod.at(i).at(0).toString());
    }
    QCOMPARE(combinedCompMethods, compressionMethods);

    QFETCH(QString, encryptionMethod);
    if (encryptionMethod.isEmpty()) {
        QCOMPARE(signalSpyEncMethod.count(), 0);
    } else {
        QVERIFY(signalSpyEncMethod.count() > 0);
        QCOMPARE(signalSpyEncMethod.at(0).at(0).toString(), encryptionMethod);
    }

    QFETCH(int, someEntryIndex);
    QVERIFY(someEntryIndex < signalSpyEntry.count());
    Archive::Entry *entry = signalSpyEntry.at(someEntryIndex).at(0).value<Archive::Entry*>();

    QFETCH(QString, expectedName);
    QCOMPARE(entry->fullPath(), expectedName);

    QFETCH(bool, isDirectory);
    QCOMPARE(entry->isDir(), isDirectory);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(entry->property("isPasswordProtected").toBool(), isPasswordProtected);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry->property("size").toULongLong(), expectedSize);

    QFETCH(QString, expectedTimestamp);
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    QEXPECT_FAIL("", "Something changed since Qt 5.11, needs investigation.", Continue);
#endif
    QCOMPARE(entry->property("timestamp").toString(), expectedTimestamp);

    plugin->deleteLater();
}

void Cli7zTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QString()
            << QStringList {
                   QStringLiteral("l"),
                   QStringLiteral("-slt"),
                   QStringLiteral("/tmp/foo.7z")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("l"),
                   QStringLiteral("-slt"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.7z")
               };
}

void Cli7zTest::testListArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cli7z plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName),
                                             QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QString, password);

    const auto replacedArgs = plugin->cliProperties()->listArgs(archiveName, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void Cli7zTest::testAddArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<bool>("encryptHeader");
    QTest::addColumn<int>("compressionLevel");
    QTest::addColumn<QString>("compressionMethod");
    QTest::addColumn<ulong>("volumeSize");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QString() << false << 5 << QStringLiteral("LZMA2") << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-l"),
                   QStringLiteral("-mx=5"),
                   QStringLiteral("-m0=LZMA2"),
                   QStringLiteral("/tmp/foo.7z")
               };

    QTest::newRow("encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QStringLiteral("1234") << false << 5 << QStringLiteral("LZMA2") << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-l"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-mx=5"),
                   QStringLiteral("-m0=LZMA2"),
                   QStringLiteral("/tmp/foo.7z")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QStringLiteral("1234") << true << 5 << QStringLiteral("LZMA2") << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-l"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-mhe=on"),
                   QStringLiteral("-mx=5"),
                   QStringLiteral("-m0=LZMA2"),
                   QStringLiteral("/tmp/foo.7z")
               };

    QTest::newRow("multi-volume")
            << QStringLiteral("/tmp/foo.7z")
            << QString() << false << 5 << QStringLiteral("LZMA2") << 2500UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-l"),
                   QStringLiteral("-mx=5"),
                   QStringLiteral("-m0=LZMA2"),
                   QStringLiteral("-v2500k"),
                   QStringLiteral("/tmp/foo.7z")
               };

    QTest::newRow("comp-method-bzip2")
            << QStringLiteral("/tmp/foo.7z")
            << QString() << false << 5 << QStringLiteral("BZip2") << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-l"),
                   QStringLiteral("-mx=5"),
                   QStringLiteral("-m0=BZip2"),
                   QStringLiteral("/tmp/foo.7z")
               };
}

void Cli7zTest::testAddArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cli7z plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName),
                                             QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QString, password);
    QFETCH(bool, encryptHeader);
    QFETCH(int, compressionLevel);
    QFETCH(ulong, volumeSize);
    QFETCH(QString, compressionMethod);

    const auto replacedArgs = plugin->cliProperties()->addArgs(archiveName, {}, password, encryptHeader, compressionLevel, compressionMethod, QString(), volumeSize);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void Cli7zTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QVector<Archive::Entry*>>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("x"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("preserve paths, unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QString()
            << QStringList {
                   QStringLiteral("x"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("e"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QString()
            << QStringList {
                   QStringLiteral("e"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };
}

void Cli7zTest::testExtractArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cli7z plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName),
                                             QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QVector<Archive::Entry*>, files);
    QStringList filesList;
    foreach (const Archive::Entry *e, files) {
        filesList << e->fullPath(NoTrailingSlash);
    }

    QFETCH(bool, preservePaths);
    QFETCH(QString, password);

    const auto replacedArgs = plugin->cliProperties()->extractArgs(archiveName, filesList, preservePaths, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void Cli7zTest::testRDAAttributes()
{
    if (!m_plugin->isValid()) {
        QSKIP("cli7z plugin not available. Skipping test.", SkipSingle);
    }

    auto loadJob = Archive::load(QFINDTESTDATA("data/RDA-attributes.zip"), m_plugin, this);
    QVERIFY(loadJob);

    int numberOfFolders = 0;
    connect(loadJob, &Job::newEntry, this, [&numberOfFolders](Archive::Entry *entry) {
        if (entry->isDir()) {
            numberOfFolders++;
        }
    });

    TestHelper::startAndWaitForResult(loadJob);
    QEXPECT_FAIL("", "Folders with RDA attributes are currently not parsed, see https://phabricator.kde.org/D18562", Continue);
    QCOMPARE(numberOfFolders, 2);
}

