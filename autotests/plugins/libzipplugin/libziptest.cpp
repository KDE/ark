/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "libziptest.h"
#include "archive_kerfuffle.h"
#include "jobs.h"
#include "testhelper.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

QTEST_GUILESS_MAIN(LibzipTest)

using namespace Kerfuffle;

void LibzipTest::initTestCase()
{
    m_plugin = m_pluginManager.pluginById(QLatin1String("kerfuffle_libzip"));
}

void LibzipTest::testPathSeparator_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QStringList>("expectedPaths");

    QTest::newRow("windows separators") << QFINDTESTDATA("data/windows-separators.zip")
                                        << QStringList{QStringLiteral("addon/addon.lua"),
                                                       QStringLiteral("addon/lang/en.lua"),
                                                       QStringLiteral("addon/lang/fr.lua"),
                                                       QStringLiteral("readme.txt")};

    QTest::newRow("backslash in name") << QFINDTESTDATA("data/backslash-in-name.zip")
                                       << QStringList{QStringLiteral("back\\slash.txt"), QStringLiteral("plain.txt")};
}

void LibzipTest::testPathSeparator()
{
    if (!m_plugin || !m_plugin->isValid()) {
        QSKIP("libzip plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);

    QStringList paths;
    connect(loadJob, &Job::newEntry, this, [&paths](Archive::Entry *entry) {
        paths << entry->fullPath();
    });

    TestHelper::startAndWaitForResult(loadJob);

    QFETCH(QStringList, expectedPaths);
    QCOMPARE(paths, expectedPaths);
}

void LibzipTest::testExtractionWithWindowsSeparators()
{
    if (!m_plugin || !m_plugin->isValid()) {
        QSKIP("libzip plugin not available. Skipping test.", SkipSingle);
    }

    auto loadJob = Archive::load(QFINDTESTDATA("data/windows-separators.zip"), m_plugin, this);
    QVERIFY(loadJob);
    TestHelper::startAndWaitForResult(loadJob);

    auto archive = loadJob->archive();
    QVERIFY(archive);
    QVERIFY(archive->isValid());

    QTemporaryDir destDir;
    QVERIFY(destDir.isValid());

    auto extractionJob = archive->extractFiles({}, destDir.path(), ExtractionOptions());
    QVERIFY(extractionJob);
    TestHelper::startAndWaitForResult(extractionJob);

    const QDir dir(destDir.path());
    QVERIFY(dir.exists(QStringLiteral("addon/lang/en.lua")));
    QVERIFY(dir.exists(QStringLiteral("addon/lang/fr.lua")));
    QVERIFY(dir.exists(QStringLiteral("addon/addon.lua")));
    QVERIFY(dir.exists(QStringLiteral("readme.txt")));

    QTemporaryDir singleEntryDir;
    QVERIFY(singleEntryDir.isValid());

    Archive::Entry entry(this, QStringLiteral("addon/lang/en.lua"), QStringLiteral("addon/lang"));
    auto singleEntryJob = archive->extractFiles({&entry}, singleEntryDir.path(), ExtractionOptions());
    QVERIFY(singleEntryJob);
    TestHelper::startAndWaitForResult(singleEntryJob);

    QVERIFY(QDir(singleEntryDir.path()).exists(QStringLiteral("addon/lang/en.lua")));
}

void LibzipTest::testMoveWithWindowsSeparators()
{
    if (!m_plugin || !m_plugin->isValid()) {
        QSKIP("libzip plugin not available. Skipping test.", SkipSingle);
    }

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString archivePath = temporaryDir.filePath(QStringLiteral("windows-separators.zip"));
    QVERIFY(QFile::copy(QFINDTESTDATA("data/windows-separators.zip"), archivePath));

    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);
    TestHelper::startAndWaitForResult(loadJob);

    auto archive = loadJob->archive();
    QVERIFY(archive);
    QVERIFY(archive->isValid());

    Archive::Entry entry(this, QStringLiteral("readme.txt"), QString());
    Archive::Entry destination(this, QStringLiteral("addon/readme.txt"), QString());
    auto moveJob = archive->moveFiles({&entry}, &destination, CompressionOptions());
    QVERIFY(moveJob);
    TestHelper::startAndWaitForResult(moveJob);

    QStringList paths;
    auto reloadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(reloadJob);
    connect(reloadJob, &Job::newEntry, this, [&paths](Archive::Entry *entry) {
        paths << entry->fullPath();
    });
    TestHelper::startAndWaitForResult(reloadJob);

    paths.sort();
    QCOMPARE(
        paths,
        QStringList(
            {QStringLiteral("addon/addon.lua"), QStringLiteral("addon/lang/en.lua"), QStringLiteral("addon/lang/fr.lua"), QStringLiteral("addon/readme.txt")}));
}

#include "moc_libziptest.cpp"
