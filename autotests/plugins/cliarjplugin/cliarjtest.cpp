/*
    SPDX-FileCopyrightText: 2022 Ilya Pominov <ipominov@astralinux.ru>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cliarjtest.h"
#include "cliplugin.h"

#include <QTest>

QTEST_GUILESS_MAIN(CliArjTest)

using namespace Kerfuffle;

void CliArjTest::initTestCase()
{
    m_plugin = new Plugin(this);
    const auto plugins = m_pluginManger.availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->metaData().pluginId() == QLatin1String("kerfuffle_cliarj")) {
            m_plugin = plugin;
            return;
        }
    }
}

void CliArjTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("fake arj") << QStringLiteral("/tmp/foo.arj") << QString() << QStringList{QStringLiteral("v"), QStringLiteral("/tmp/foo.arj")};

    QTest::newRow("fake encrypted arj") << QStringLiteral("/tmp/foo.arj") << QStringLiteral("1234")
                                        << QStringList{QStringLiteral("v"), QStringLiteral("/tmp/foo.arj")};
}

void CliArjTest::testListArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliarj plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName), QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QString, password);
    const auto replacedArgs = plugin->cliProperties()->listArgs(archiveName, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliArjTest::testAddArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<int>("compressionLevel");
    QTest::addColumn<QString>("compressionMethod");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted") << QStringLiteral("/tmp/foo.arj") << QString() << -1 << QString()
                                 << QStringList{QStringLiteral("a"), QStringLiteral("-r"), QStringLiteral("/tmp/foo.arj")};

    QTest::newRow("encrypted") << QStringLiteral("/tmp/foo.arj") << QStringLiteral("1234") << -1 << QString()
                               << QStringList{QStringLiteral("a"), QStringLiteral("-r"), QStringLiteral("-g1234"), QStringLiteral("/tmp/foo.arj")};

    QTest::newRow("comp-method-good") << QStringLiteral("/tmp/foo.arj") << QString() << -1 << QStringLiteral("Good (default)")
                                      << QStringList{QStringLiteral("a"), QStringLiteral("-r"), QStringLiteral("-m1"), QStringLiteral("/tmp/foo.arj")};
}

void CliArjTest::testAddArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliarj plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName), QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QString, password);
    QFETCH(int, compressionLevel);
    QFETCH(QString, compressionMethod);

    const auto replacedArgs = plugin->cliProperties()->addArgs(archiveName, {}, password, false, compressionLevel, compressionMethod, QString(), 0);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliArjTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QList<Archive::Entry *>>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted") << QStringLiteral("/tmp/foo.arj")
                                               << (QList<Archive::Entry *>{
                                                      new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                                                      new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
                                                  })
                                               << true << QStringLiteral("1234")
                                               << QStringList{
                                                      QStringLiteral("x"),
                                                      QStringLiteral("-p1"),
                                                      QStringLiteral("-jyc"),
                                                      QStringLiteral("-g1234"),
                                                      QStringLiteral("/tmp/foo.arj"),
                                                      QStringLiteral("aDir/textfile2.txt"),
                                                      QStringLiteral("c.txt"),
                                                  };

    QTest::newRow("preserve paths, unencrypted") << QStringLiteral("/tmp/foo.arj")
                                                 << (QList<Archive::Entry *>{
                                                        new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                                                        new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
                                                    })
                                                 << true << QString()
                                                 << QStringList{
                                                        QStringLiteral("x"),
                                                        QStringLiteral("-p1"),
                                                        QStringLiteral("-jyc"),
                                                        QStringLiteral("/tmp/foo.arj"),
                                                        QStringLiteral("aDir/textfile2.txt"),
                                                        QStringLiteral("c.txt"),
                                                    };

    QTest::newRow("without paths, encrypted") << QStringLiteral("/tmp/foo.arj")
                                              << (QList<Archive::Entry *>{
                                                     new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                                                     new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
                                                 })
                                              << false << QStringLiteral("1234")
                                              << QStringList{
                                                     QStringLiteral("e"),
                                                     QStringLiteral("-g1234"),
                                                     QStringLiteral("/tmp/foo.arj"),
                                                     QStringLiteral("aDir/textfile2.txt"),
                                                     QStringLiteral("c.txt"),
                                                 };

    QTest::newRow("without paths, unencrypted") << QStringLiteral("/tmp/foo.arj")
                                                << (QList<Archive::Entry *>{
                                                       new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                                                       new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
                                                   })
                                                << false << QString()
                                                << QStringList{
                                                       QStringLiteral("e"),
                                                       QStringLiteral("/tmp/foo.arj"),
                                                       QStringLiteral("aDir/textfile2.txt"),
                                                       QStringLiteral("c.txt"),
                                                   };
}

void CliArjTest::testExtractArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliarj plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName), QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QList<Archive::Entry *>, files);
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

#include "moc_cliarjtest.cpp"
