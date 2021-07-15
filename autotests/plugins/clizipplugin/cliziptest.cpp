/*
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

#include "cliziptest.h"
#include "cliplugin.h"

#include <QTest>

QTEST_GUILESS_MAIN(CliZipTest)

using namespace Kerfuffle;

void CliZipTest::initTestCase()
{
    m_plugin = new Plugin(this);
    const auto plugins = m_pluginManger.availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->metaData().pluginId() == QLatin1String("kerfuffle_clizip")) {
            m_plugin = plugin;
            return;
        }
    }
}

void CliZipTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("fake zip")
            << QStringLiteral("/tmp/foo.zip")
            << QString()
            << QStringList {
                   QStringLiteral("-l"),
                   QStringLiteral("-T"),
                   QStringLiteral("-z"),
                   QStringLiteral("/tmp/foo.zip")
               };

    QTest::newRow("fake encrypted zip")
            << QStringLiteral("/tmp/foo.zip")
            << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-l"),
                   QStringLiteral("-T"),
                   QStringLiteral("-z"),
                   QStringLiteral("/tmp/foo.zip")
               };
}

void CliZipTest::testListArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("clizip plugin not available. Skipping test.", SkipSingle);
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

void CliZipTest::testAddArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<int>("compressionLevel");
    QTest::addColumn<QString>("compressionMethod");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QString() << 3 << QStringLiteral("Deflate")
            << QStringList {
                   QStringLiteral("-r"),
                   QStringLiteral("-3"),
                   QStringLiteral("-Zdeflate"),
                   QStringLiteral("/tmp/foo.zip")
               };

    QTest::newRow("encrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QStringLiteral("1234") << 3 << QString()
            << QStringList {
                   QStringLiteral("-r"),
                   QStringLiteral("-P1234"),
                   QStringLiteral("-3"),
                   QStringLiteral("/tmp/foo.zip")
               };

    QTest::newRow("comp-method-bzip2")
            << QStringLiteral("/tmp/foo.zip")
            << QString() << 3 << QStringLiteral("BZip2")
            << QStringList {
                   QStringLiteral("-r"),
                   QStringLiteral("-3"),
                   QStringLiteral("-Zbzip2"),
                   QStringLiteral("/tmp/foo.zip")
               };
}

void CliZipTest::testAddArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("clizip plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName),
                                             QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QString, password);
    QFETCH(int, compressionLevel);
    QFETCH(QString, compressionMethod);

    const auto replacedArgs = plugin->cliProperties()->addArgs(archiveName, {}, password, false, compressionLevel, compressionMethod, QString(), 0);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliZipTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QVector<Archive::Entry*>>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-P1234"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("preserve paths, unencrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QString()
            << QStringList {
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, encrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-j"),
                   QStringLiteral("-P1234"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, unencrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QVector<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QString()
            << QStringList {
                   QStringLiteral("-j"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };
}

void CliZipTest::testExtractArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("clizip plugin not available. Skipping test.", SkipSingle);
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
