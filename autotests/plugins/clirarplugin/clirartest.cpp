/*
    SPDX-FileCopyrightText: 2011, 2014 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2015, 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "clirartest.h"
#include "cliplugin.h"
#include "archive_kerfuffle.h"
#include "jobs.h"
#include "testhelper.h"

#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QTextStream>

QTEST_GUILESS_MAIN(CliRarTest)

void initLocale()
{
    qputenv("LC_ALL", "en_US.utf-8");
}

Q_CONSTRUCTOR_FUNCTION(initLocale)

using namespace Kerfuffle;

void CliRarTest::initTestCase()
{
    m_plugin = new Plugin(this);
    const auto plugins = m_pluginManger.availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->metaData().pluginId() == QLatin1String("kerfuffle_clirar")) {
            m_plugin = plugin;
            return;
        }
    }
}

void CliRarTest::testArchive_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");

    const bool hasRar = !QStandardPaths::findExecutable(QStringLiteral("rar")).isEmpty();

    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("archive with one top-level folder")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << !hasRar << true << Archive::Unencrypted
            << QStringLiteral("A");

    archivePath = QFINDTESTDATA("data/locked_archive.rar");
    QTest::newRow("locked archive")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << true << false << Archive::Unencrypted
            << QStringLiteral("locked_archive");
}

void CliRarTest::testArchive()
{
    if (!m_plugin->isValid()) {
        QSKIP("clirar plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the clirar plugin. Skipping test.", SkipSingle);
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

void CliRarTest::testList_data()
{
    QTest::addColumn<QString>("outputTextFile");
    QTest::addColumn<QString>("errorMessage");
    QTest::addColumn<int>("expectedEntriesCount");
    QTest::addColumn<bool>("isMultiVolume");
    // Is zero for non-multi-volume archives:
    QTest::addColumn<int>("numberOfVolumes");
    QTest::addColumn<QStringList>("compressionMethods");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<QString>("symlinkTarget");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<qulonglong>("expectedCompressedSize");
    QTest::addColumn<QDateTime>("expectedTimestamp");

    // Unrar 5 tests

    QTest::newRow("normal-file-unrar5")
            << QFINDTESTDATA("data/archive-with-symlink-unrar5.txt") << QString() << 8 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 2 << QStringLiteral("rartest/file2.txt") << false << false << QString() << (qulonglong) 14 << (qulonglong) 23 << QDateTime::fromString(QStringLiteral("2016-03-21T08:57:36"),Qt::ISODateWithMs);

    QTest::newRow("symlink-unrar5")
            << QFINDTESTDATA("data/archive-with-symlink-unrar5.txt") << QString() << 8 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 3 << QStringLiteral("rartest/linktofile1.txt") << false << false << QStringLiteral("file1.txt") << (qulonglong) 9 << (qulonglong) 9 << QDateTime::fromString(QStringLiteral("2016-03-21T08:58:16"),Qt::ISODateWithMs);

    QTest::newRow("encrypted-unrar5")
            << QFINDTESTDATA("data/archive-encrypted-unrar5.txt") << QString() << 7 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 2 << QStringLiteral("rartest/file2.txt") << false << true << QString() << (qulonglong) 14 << (qulonglong) 32 << QDateTime::fromString(QStringLiteral("2016-03-21T17:03:36"),Qt::ISODateWithMs);

    QTest::newRow("recovery-record-unrar5")
            << QFINDTESTDATA("data/archive-recovery-record-unrar5.txt") << QString() << 3 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 0 << QStringLiteral("file1.txt") << false << false << QString() << (qulonglong) 32 << (qulonglong) 33 << QDateTime::fromString(QStringLiteral("2015-07-26T19:04:38"),Qt::ISODateWithMs);

    QTest::newRow("corrupt-archive-unrar5")
            << QFINDTESTDATA("data/archive-corrupt-file-header-unrar5.txt") << QString() << 8 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 6 << QStringLiteral("dir1/") << true << false << QString() << (qulonglong) 0 << (qulonglong) 0 << QDateTime::fromString(QStringLiteral("2015-05-14T01:45:24"),Qt::ISODateWithMs);

    //Note: The number of entries will be the total number of all entries in all volumes, i.e. if a file spans 3 volumes it will count as 3 entries.
    QTest::newRow("multivolume-archive-unrar5")
            << QFINDTESTDATA("data/archive-multivol-unrar5.txt") << QString() << 6 << true << 5 << QStringList{QStringLiteral("RAR4")}
            << 5 << QStringLiteral("largefile2") << false << false << QString() << (qulonglong) 2097152 << (qulonglong) 11231 << QDateTime::fromString(QStringLiteral("2016-07-17T11:26:19"),Qt::ISODateWithMs);

    QTest::newRow("RAR5-open-with-unrar5")
            << QFINDTESTDATA("data/archive-RARv5-unrar5.txt") << QString() << 9 << false << 0 << QStringList{QStringLiteral("RAR5")}
            << 4 << QStringLiteral("testarchive/dir1/file1.txt") << false << false << QString() << (qulonglong) 32 << (qulonglong) 32 << QDateTime::fromString(QStringLiteral("2015-05-17T20:41:48"),Qt::ISODateWithMs);

    // Unrar 4 tests

    QTest::newRow("normal-file-unrar4")
            << QFINDTESTDATA("data/archive-with-symlink-unrar4.txt") << QString() << 8 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 2 << QStringLiteral("rartest/file2.txt") << false << false << QString() << (qulonglong) 14 << (qulonglong) 23 << QDateTime::fromString(QStringLiteral("2016-03-21T08:57:00"),Qt::ISODateWithMs);

    QTest::newRow("symlink-unrar4")
            << QFINDTESTDATA("data/archive-with-symlink-unrar4.txt") << QString() << 8 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 3 << QStringLiteral("rartest/linktofile1.txt") << false << false << QStringLiteral("file1.txt") << (qulonglong) 9 << (qulonglong) 9 << QDateTime::fromString(QStringLiteral("2016-03-21T08:58:00"),Qt::ISODateWithMs);

    QTest::newRow("encrypted-unrar4")
            << QFINDTESTDATA("data/archive-encrypted-unrar4.txt") << QString() << 7 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 2 << QStringLiteral("rartest/file2.txt") << false << true << QString() << (qulonglong) 14 << (qulonglong) 32 << QDateTime::fromString(QStringLiteral("2016-03-21T17:03:00"),Qt::ISODateWithMs);

    QTest::newRow("recovery-record-unrar4")
            << QFINDTESTDATA("data/archive-recovery-record-unrar4.txt") << QString() << 3 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 0 << QStringLiteral("file1.txt") << false << false << QString() << (qulonglong) 32 << (qulonglong) 33 << QDateTime::fromString(QStringLiteral("2015-07-26T19:04:00"),Qt::ISODateWithMs);

    QTest::newRow("corrupt-archive-unrar4")
            << QFINDTESTDATA("data/archive-corrupt-file-header-unrar4.txt") << QString() << 8 << false << 0 << QStringList{QStringLiteral("RAR4")}
            << 6 << QStringLiteral("dir1/") << true << false << QString() << (qulonglong) 0 << (qulonglong) 0 << QDateTime::fromString(QStringLiteral("2015-05-14T01:45:00"),Qt::ISODateWithMs);

    QTest::newRow("RAR5-open-with-unrar4")
            << QFINDTESTDATA("data/archive-RARv5-unrar4.txt")
            << QStringLiteral("Your unrar executable is version 4.20, which is too old to handle this archive. Please update to a more recent version.")
            << 0 << false << 0 << QStringList() << 0 << QString() << true << false << QString() << (qulonglong) 0 << (qulonglong) 0 << QDateTime::currentDateTime();

    //Note: The number of entries will be the total number of all entries in all volumes, i.e. if a file spans 3 volumes it will count as 3 entries.
    QTest::newRow("multivolume-archive-unrar4")
            << QFINDTESTDATA("data/archive-multivol-unrar4.txt") << QString() << 6 << true << 5 << QStringList{QStringLiteral("RAR4")}
            << 5 << QStringLiteral("largefile2") << false << false << QString() << (qulonglong) 2097152 << (qulonglong) 11231 << QDateTime::fromString(QStringLiteral("2016-07-17T11:26:00"),Qt::ISODateWithMs);

    // Unrar 3 tests

    QTest::newRow("RAR5-open-with-unrar3")
            << QFINDTESTDATA("data/archive-RARv5-unrar3.txt")
            << QStringLiteral("Unrar reported a non-RAR archive. The installed unrar version (3.71) is old. Try updating your unrar.")
            << 0 << false << 0 << QStringList() << 0 << QString() << true << false << QString() << (qulonglong) 0 << (qulonglong) 0 << QDateTime::currentDateTime();

    /*
     * Check that the plugin will not crash when reading corrupted archives, which
     * have lines such as "Unexpected end of archive" or "??? - the file header is
     * corrupt" instead of a file name and the header string after it.
     *
     * See bug 262857 and commit 2042997013432cdc6974f5b26d39893a21e21011.
     */
    QTest::newRow("corrupt-archive-unrar3")
            << QFINDTESTDATA("data/archive-corrupt-file-header-unrar3.txt") << QString() << 1 << true << 1 << QStringList{QStringLiteral("RAR4")}
            << 0 << QStringLiteral("some-file.ext") << false << false << QString() << (qulonglong) 732522496 << (qulonglong) 14851208 << QDateTime::fromString(QStringLiteral("2010-10-29T20:47:00"),Qt::ISODateWithMs);
}

void CliRarTest::testList()
{
    qRegisterMetaType<Archive::Entry*>("Archive::Entry*");
    CliPlugin *rarPlugin = new CliPlugin(this, {QStringLiteral("dummy.rar"),
                                                QVariant::fromValue(m_plugin->metaData())});
    QSignalSpy signalSpyEntry(rarPlugin, &CliPlugin::entry);
    QSignalSpy signalSpyCompMethod(rarPlugin, &CliPlugin::compressionMethodFound);
    QSignalSpy signalSpyError(rarPlugin, &CliPlugin::error);

    QFETCH(QString, outputTextFile);
    QFETCH(int, expectedEntriesCount);

    QFile outputText(outputTextFile);
    QVERIFY(outputText.open(QIODevice::ReadOnly));

    QTextStream outputStream(&outputText);
    while (!outputStream.atEnd()) {
        const QString line(outputStream.readLine());
        if (!rarPlugin->readListLine(line)) {
            break;
        }
    }

    QFETCH(QString, errorMessage);
    if (!errorMessage.isEmpty()) {
        QCOMPARE(signalSpyError.count(), 1);
        QCOMPARE(signalSpyError.at(0).at(0).toString(), errorMessage);
        return;
    }

    QCOMPARE(signalSpyEntry.count(), expectedEntriesCount);

    QFETCH(bool, isMultiVolume);
    QCOMPARE(rarPlugin->isMultiVolume(), isMultiVolume);

    QFETCH(int, numberOfVolumes);
    QCOMPARE(rarPlugin->numberOfVolumes(), numberOfVolumes);

    QVERIFY(signalSpyCompMethod.count() > 0);
    QFETCH(QStringList, compressionMethods);
    if (!compressionMethods.isEmpty()) {
        QCOMPARE(signalSpyCompMethod.at(0).at(0).toStringList(), compressionMethods);
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

    QFETCH(QString, symlinkTarget);
    QCOMPARE(entry->property("link").toString(), symlinkTarget);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry->property("size").toULongLong(), expectedSize);

    QFETCH(qulonglong, expectedCompressedSize);
    QCOMPARE(entry->property("compressedSize").toULongLong(), expectedCompressedSize);

    QFETCH(QDateTime, expectedTimestamp);
    QCOMPARE(entry->property("timestamp").toDateTime(), expectedTimestamp);

    rarPlugin->deleteLater();
}

void CliRarTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QString()
            << QStringList {
                   QStringLiteral("vt"),
                   QStringLiteral("-v"),
                   QStringLiteral("/tmp/foo.rar")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("vt"),
                   QStringLiteral("-v"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.rar")
               };
}

void CliRarTest::testListArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("clirar plugin not available. Skipping test.", SkipSingle);
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

void CliRarTest::testAddArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<bool>("encryptHeader");
    QTest::addColumn<int>("compressionLevel");
    QTest::addColumn<QString>("compressionMethod");
    QTest::addColumn<ulong>("volumeSize");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QString() << false << 3 << QStringLiteral("RAR4") << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-m3"),
                   QStringLiteral("-ma4"),
                   QStringLiteral("/tmp/foo.rar")
               };

    QTest::newRow("encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QStringLiteral("1234") << false << 3 << QString() << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-m3"),
                   QStringLiteral("/tmp/foo.rar")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QStringLiteral("1234") << true << 3 << QString() << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-hp1234"),
                   QStringLiteral("-m3"),
                   QStringLiteral("/tmp/foo.rar")
               };

    QTest::newRow("multi-volume")
            << QStringLiteral("/tmp/foo.rar")
            << QString() << false << 3 << QString() << 2500UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-m3"),
                   QStringLiteral("-v2500k"),
                   QStringLiteral("/tmp/foo.rar")
               };
    QTest::newRow("comp-method-RAR5")
            << QStringLiteral("/tmp/foo.rar")
            << QString() << false << 3 << QStringLiteral("RAR5") << 0UL
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("-m3"),
                   QStringLiteral("-ma5"),
                   QStringLiteral("/tmp/foo.rar")
               };
}

void CliRarTest::testAddArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("clirar plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName),
                                             QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QString, password);
    QFETCH(bool, encryptHeader);
    QFETCH(int, compressionLevel);
    QFETCH(QString, compressionMethod);
    QFETCH(ulong, volumeSize);

    const auto replacedArgs = plugin->cliProperties()->addArgs(archiveName, {}, password, encryptHeader, compressionLevel, compressionMethod, QString(), volumeSize);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliRarTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QVector<Archive::Entry*>>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("x"),
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("preserve paths, unencrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QString()
            << QStringList {
                   QStringLiteral("x"),
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("e"),
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, unencrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QString()
            << QStringList {
                   QStringLiteral("e"),
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };
}

void CliRarTest::testExtractArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("clirar plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName),
                                             QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QVector<Archive::Entry*>, files);
    QStringList filesList;
    for (const Archive::Entry *e : std::as_const(files)) {
        filesList << e->fullPath(NoTrailingSlash);
    }

    QFETCH(bool, preservePaths);
    QFETCH(QString, password);

    const auto replacedArgs = plugin->cliProperties()->extractArgs(archiveName, filesList, preservePaths, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

#include "moc_clirartest.cpp"
