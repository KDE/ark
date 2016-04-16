/*
 * Copyright (c) 2011,2014 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2015,2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#include "clirartest.h"

#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

#include <KPluginLoader>

QTEST_GUILESS_MAIN(CliRarTest)

using namespace Kerfuffle;

void CliRarTest::initTestCase()
{
    qRegisterMetaType<ArchiveEntry>();

    const auto plugins = KPluginLoader::findPluginsById(QStringLiteral("kerfuffle"), QStringLiteral("kerfuffle_clirar"));
    if (plugins.size() == 1) {
        m_pluginMetadata = plugins.at(0);
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

    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("archive with one top-level folder")
            << archivePath
            << QFileInfo(archivePath).fileName()
            << false << true << Archive::Unencrypted
            << QStringLiteral("A");
}

void CliRarTest::testArchive()
{
    if (!m_pluginMetadata.isValid()) {
        QSKIP("Could not find the clirar plugin. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    Archive *archive = Archive::create(archivePath, m_pluginMetadata, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the clirar plugin. Skipping test.", SkipSingle);
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

void CliRarTest::testList_data()
{
    QTest::addColumn<QString>("outputTextFile");
    QTest::addColumn<int>("expectedEntriesCount");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<QString>("symlinkTarget");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<qulonglong>("expectedCompressedSize");
    QTest::addColumn<QString>("expectedTimestamp");

    // Unrar 5 tests

    QTest::newRow("normal-file-unrar5")
            << QFINDTESTDATA("data/archive-with-symlink-unrar5.txt") << 8
            << 2 << QStringLiteral("rartest/file2.txt") << false << false << QString() << (qulonglong) 14 << (qulonglong) 23 << QStringLiteral("2016-03-21T08:57:36");

    QTest::newRow("symlink-unrar5")
            << QFINDTESTDATA("data/archive-with-symlink-unrar5.txt") << 8
            << 3 << QStringLiteral("rartest/linktofile1.txt") << false << false << QStringLiteral("file1.txt") << (qulonglong) 9 << (qulonglong) 9 << QStringLiteral("2016-03-21T08:58:16");

    QTest::newRow("encrypted-unrar5")
            << QFINDTESTDATA("data/archive-encrypted-unrar5.txt") << 7
            << 2 << QStringLiteral("rartest/file2.txt") << false << true << QString() << (qulonglong) 14 << (qulonglong) 32 << QStringLiteral("2016-03-21T17:03:36");

    QTest::newRow("recovery-record-unrar5")
            << QFINDTESTDATA("data/archive-recovery-record-unrar5.txt") << 3
            << 0 << QStringLiteral("file1.txt") << false << false << QString() << (qulonglong) 32 << (qulonglong) 33 << QStringLiteral("2015-07-26T19:04:38");

    QTest::newRow("corrupt-archive-unrar5")
            << QFINDTESTDATA("data/archive-corrupt-file-header-unrar5.txt") << 8
            << 6 << QStringLiteral("dir1/") << true << false << QString() << (qulonglong) 0 << (qulonglong) 0 << QStringLiteral("2015-05-14T01:45:24");

    // Unrar 4 tests

    QTest::newRow("normal-file-unrar4")
            << QFINDTESTDATA("data/archive-with-symlink-unrar4.txt") << 8
            << 2 << QStringLiteral("rartest/file2.txt") << false << false << QString() << (qulonglong) 14 << (qulonglong) 23 << QStringLiteral("2016-03-21T08:57:00");

    QTest::newRow("symlink-unrar4")
            << QFINDTESTDATA("data/archive-with-symlink-unrar4.txt") << 8
            << 3 << QStringLiteral("rartest/linktofile1.txt") << false << false << QStringLiteral("file1.txt") << (qulonglong) 9 << (qulonglong) 9 << QStringLiteral("2016-03-21T08:58:00");

    QTest::newRow("encrypted-unrar4")
            << QFINDTESTDATA("data/archive-encrypted-unrar4.txt") << 7
            << 2 << QStringLiteral("rartest/file2.txt") << false << true << QString() << (qulonglong) 14 << (qulonglong) 32 << QStringLiteral("2016-03-21T17:03:00");

    QTest::newRow("recovery-record-unrar4")
            << QFINDTESTDATA("data/archive-recovery-record-unrar4.txt") << 3
            << 0 << QStringLiteral("file1.txt") << false << false << QString() << (qulonglong) 32 << (qulonglong) 33 << QStringLiteral("2015-07-26T19:04:00");

    QTest::newRow("corrupt-archive-unrar4")
            << QFINDTESTDATA("data/archive-corrupt-file-header-unrar4.txt") << 8
            << 6 << QStringLiteral("dir1/") << true << false << QString() << (qulonglong) 0 << (qulonglong) 0 << QStringLiteral("2015-05-14T01:45:00");

    /*
     * Check that the plugin will not crash when reading corrupted archives, which
     * have lines such as "Unexpected end of archive" or "??? - the file header is
     * corrupt" instead of a file name and the header string after it.
     *
     * See bug 262857 and commit 2042997013432cdc6974f5b26d39893a21e21011.
     */
    QTest::newRow("corrupt-archive-unrar3")
            << QFINDTESTDATA("data/archive-corrupt-file-header-unrar3.txt") << 1
            << 0 << QStringLiteral("some-file.ext") << false << false << QString() << (qulonglong) 732522496 << (qulonglong) 14851208 << QStringLiteral("2010-10-29T20:47:00");
}

void CliRarTest::testList()
{
    CliPlugin *rarPlugin = new CliPlugin(this, {QStringLiteral("dummy.rar")});
    QSignalSpy signalSpy(rarPlugin, SIGNAL(entry(ArchiveEntry)));

    QFETCH(QString, outputTextFile);
    QFETCH(int, expectedEntriesCount);

    QFile outputText(outputTextFile);
    QVERIFY(outputText.open(QIODevice::ReadOnly));

    QTextStream outputStream(&outputText);
    while (!outputStream.atEnd()) {
        const QString line(outputStream.readLine());
        QVERIFY(rarPlugin->readListLine(line));
    }

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

    QFETCH(QString, symlinkTarget);
    QCOMPARE(entry[Link].toString(), symlinkTarget);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry[Size].toULongLong(), expectedSize);

    QFETCH(qulonglong, expectedCompressedSize);
    QCOMPARE(entry[CompressedSize].toULongLong(), expectedCompressedSize);

    QFETCH(QString, expectedTimestamp);
    QCOMPARE(entry[Timestamp].toString(), expectedTimestamp);

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
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList listArgs = { QStringLiteral("vt"),
                                   QStringLiteral("-v"),
                                   QStringLiteral("$PasswordSwitch"),
                                   QStringLiteral("$Archive") };

    QFETCH(QString, password);
    const auto replacedArgs = plugin->substituteListVariables(listArgs, password);

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
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QString() << false << 3
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("-m3")
               };

    QTest::newRow("encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QStringLiteral("1234") << false << 3
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-m3")
               };

    QTest::newRow("header-encrypted")
            << QStringLiteral("/tmp/foo.rar")
            << QStringLiteral("1234") << true << 3
            << QStringList {
                   QStringLiteral("a"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("-hp1234"),
                   QStringLiteral("-m3")
               };
}

void CliRarTest::testAddArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *rarPlugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(rarPlugin);

    const QStringList addArgs = { QStringLiteral("a"),
                                  QStringLiteral("$Archive"),
                                  QStringLiteral("$PasswordSwitch"),
                                  QStringLiteral("$CompressionLevelSwitch"),
                                  QStringLiteral("$Files") };

    QFETCH(QString, password);
    QFETCH(bool, encryptHeader);
    QFETCH(int, compressionLevel);

    QStringList replacedArgs = rarPlugin->substituteAddVariables(addArgs, {}, password, encryptHeader, compressionLevel);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    rarPlugin->deleteLater();
}

void CliRarTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QVariantList>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QString>("rootNode");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted, root node")
            << QStringLiteral("/tmp/foo.rar")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << true << QStringLiteral("1234") << QStringLiteral("aDir")
            << QStringList {
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("x"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-apaDir"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("preserve paths, unencrypted, root node")
            << QStringLiteral("/tmp/foo.rar")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << true << QString() << QStringLiteral("aDir")
            << QStringList {
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("x"),
                   QStringLiteral("-apaDir"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, encrypted, root node")
            << QStringLiteral("/tmp/foo.rar")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << false << QStringLiteral("1234") << QStringLiteral("aDir")
            << QStringList {
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("e"),
                   QStringLiteral("-p1234"),
                   QStringLiteral("-apaDir"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, unencrypted, root node")
            << QStringLiteral("/tmp/foo.rar")
            << QVariantList {
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("aDir/b.txt"), QStringLiteral("aDir"))),
                   QVariant::fromValue(fileRootNodePair(QStringLiteral("c.txt"), QString()))
               }
            << false << QString() << QStringLiteral("aDir")
            << QStringList {
                   QStringLiteral("-kb"),
                   QStringLiteral("-p-"),
                   QStringLiteral("e"),
                   QStringLiteral("-apaDir"),
                   QStringLiteral("/tmp/foo.rar"),
                   QStringLiteral("aDir/b.txt"),
                   QStringLiteral("c.txt"),
               };
}

void CliRarTest::testExtractArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *rarPlugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(rarPlugin);

    const QStringList extractArgs = { QStringLiteral("-kb"),
                                      QStringLiteral("-p-"),
                                      QStringLiteral("$PreservePathSwitch"),
                                      QStringLiteral("$PasswordSwitch"),
                                      QStringLiteral("$RootNodeSwitch"),
                                      QStringLiteral("$Archive"),
                                      QStringLiteral("$Files") };

    QFETCH(QVariantList, files);
    QFETCH(bool, preservePaths);
    QFETCH(QString, password);
    QFETCH(QString, rootNode);

    QStringList replacedArgs = rarPlugin->substituteCopyVariables(extractArgs, files, preservePaths, password, rootNode);
    QVERIFY(replacedArgs.size() >= extractArgs.size());

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    rarPlugin->deleteLater();
}
