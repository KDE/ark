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

#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#include <KPluginLoader>

QTEST_GUILESS_MAIN(Cli7zTest)

using namespace Kerfuffle;

void Cli7zTest::initTestCase()
{
    qRegisterMetaType<ArchiveEntry>();

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
    Archive *archive = Archive::create(archivePath, m_plugin, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cli7z plugin. Skipping test.", SkipSingle);
    }

    QFETCH(QString, expectedFileName);
    QCOMPARE(QFileInfo(archive->fileName()).fileName(), expectedFileName);

    QFETCH(bool, isReadOnly);
    QCOMPARE(archive->isReadOnly(), isReadOnly);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(archive->isSingleFolderArchive(), isSingleFolder);

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
            << QFINDTESTDATA("data/archive-with-symlink-1602.txt") << 10 << false << 0
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T20:41:48");

    QTest::newRow("encrypted-1602")
            << QFINDTESTDATA("data/archive-encrypted-1602.txt") << 4 << false << 0
            << 1 << QStringLiteral("file2.txt") << false << true << (qulonglong) 14 << QStringLiteral("2016-03-02T22:37:55");

    QTest::newRow("multi-volume-1602")
            << QFINDTESTDATA("data/archive-multivol-1602.txt") << 2 << true << 5
            << 1 << QStringLiteral("largefile2") << false << false << (qulonglong) 2097152 << QStringLiteral("2016-07-17T11:26:19");

    // p7zip version 15.14 tests

    QTest::newRow("normal-file-1514")
            << QFINDTESTDATA("data/archive-with-symlink-1514.txt") << 10 << false << 0
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-1514")
            << QFINDTESTDATA("data/archive-encrypted-1514.txt") << 9 << false << 0
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    // p7zip version 15.09 tests

    QTest::newRow("normal-file-1509")
            << QFINDTESTDATA("data/archive-with-symlink-1509.txt") << 10 << false << 0
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-1509")
            << QFINDTESTDATA("data/archive-encrypted-1509.txt") << 9 << false << 0
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    // p7zip version 9.38.1 tests

    QTest::newRow("normal-file-9381")
            << QFINDTESTDATA("data/archive-with-symlink-9381.txt") << 10 << false << 0
            << 4 << QStringLiteral("testarchive/dir2/file2.txt") << false << false << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");

    QTest::newRow("encrypted-9381")
            << QFINDTESTDATA("data/archive-encrypted-9381.txt") << 9 << false << 0
            << 3 << QStringLiteral("testarchive/dir1/file1.txt") << false << true << (qulonglong) 32 << QStringLiteral("2015-05-17T19:41:48");
}

void Cli7zTest::testList()
{
    CliPlugin *plugin = new CliPlugin(this, {QStringLiteral("dummy.7z")});
    QSignalSpy signalSpy(plugin, SIGNAL(entry(ArchiveEntry)));

    QFETCH(QString, outputTextFile);
    QFETCH(int, expectedEntriesCount);

    QFile outputText(outputTextFile);
    QVERIFY(outputText.open(QIODevice::ReadOnly));

    QTextStream outputStream(&outputText);
    while (!outputStream.atEnd()) {
        const QString line(outputStream.readLine());
        QVERIFY(plugin->readListLine(line));
    }

    QCOMPARE(signalSpy.count(), expectedEntriesCount);

    QFETCH(bool, isMultiVolume);
    QCOMPARE(plugin->isMultiVolume(), isMultiVolume);

    QFETCH(int, numberOfVolumes);
    QCOMPARE(plugin->numberOfVolumes(), numberOfVolumes);

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

    QFETCH(QString, expectedTimestamp);
    QCOMPARE(entry[Timestamp].toString(), expectedTimestamp);

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
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList listArgs = { QStringLiteral("l"),
                                   QStringLiteral("-slt"),
                                   QStringLiteral("$PasswordSwitch"),
                                   QStringLiteral("$Archive") };

    QFETCH(QString, password);
    const auto replacedArgs = plugin->substituteListVariables(listArgs, password);

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
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QString() << false << 5
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("-mx=5")
               };

    QTest::newRow("encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QStringLiteral("1234") << false << 5
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-mx=5")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QStringLiteral("1234") << true << 5
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-mhe=on"),
                   QStringLiteral("-mx=5")
               };
}

void Cli7zTest::testAddArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList addArgs = { QStringLiteral("a"),
                                  QStringLiteral("$Archive"),
                                  QStringLiteral("$PasswordSwitch"),
                                  QStringLiteral("$CompressionLevelSwitch"),
                                  QStringLiteral("$Files") };

    QFETCH(QString, password);
    QFETCH(bool, encryptHeader);
    QFETCH(int, compressionLevel);

    QStringList replacedArgs = plugin->substituteAddVariables(addArgs, {}, password, encryptHeader, compressionLevel);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void Cli7zTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QVariantList>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << true << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("x"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("preserve paths, unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << true << QString()
            << QStringList {
                   QStringLiteral("x"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, encrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << false << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("e"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, unencrypted")
            << QStringLiteral("/tmp/foo.7z")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << false << QString()
            << QStringList {
                   QStringLiteral("e"),
                   QStringLiteral("/tmp/foo.7z"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };
}

void Cli7zTest::testExtractArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList extractArgs = { QStringLiteral("$PreservePathSwitch"),
                                      QStringLiteral("$PasswordSwitch"),
                                      QStringLiteral("$Archive"),
                                      QStringLiteral("$Files") };

    QFETCH(QVariantList, files);
    QFETCH(bool, preservePaths);
    QFETCH(QString, password);

    QStringList replacedArgs = plugin->substituteCopyVariables(extractArgs, files, preservePaths, password);
    QVERIFY(replacedArgs.size() >= extractArgs.size());

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}
