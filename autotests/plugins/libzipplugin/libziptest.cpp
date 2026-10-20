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
#include <QFileInfo>
#include <QLocale>
#include <QScopeGuard>
#include <QStringDecoder>
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

void LibzipTest::testLegacyCodePageNames_data()
{
    QTest::addColumn<QLocale>("locale");
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QByteArray>("codePage");
    QTest::addColumn<QStringList>("expectedPaths");

    const QString cp866Archive = QFINDTESTDATA("data/cp866-names.zip");
    const QStringList cyrillicPaths{QStringLiteral("Задания/"), QStringLiteral("Задания/Вариант 1.txt"), QStringLiteral("Ответы.txt")};
    QTest::newRow("Russian") << QLocale(QLocale::Russian, QLocale::Russia) << cp866Archive << QByteArray("IBM866") << cyrillicPaths;
    QTest::newRow("Azerbaijani Cyrillic") << QLocale(QLocale::Azerbaijani, QLocale::CyrillicScript, QLocale::Azerbaijan) << cp866Archive << QByteArray("IBM866")
                                          << cyrillicPaths;
    QTest::newRow("Uzbek Cyrillic") << QLocale(QLocale::Uzbek, QLocale::CyrillicScript, QLocale::Uzbekistan) << cp866Archive << QByteArray("IBM866")
                                    << cyrillicPaths;
    QTest::newRow("Sakha") << QLocale(QLocale::Sakha, QLocale::Russia) << cp866Archive << QByteArray("IBM866") << cyrillicPaths;
    QTest::newRow("Bosnian Cyrillic") << QLocale(QLocale::Bosnian, QLocale::CyrillicScript, QLocale::BosniaAndHerzegovina)
                                      << QFINDTESTDATA("data/cp855-names.zip") << QByteArray("IBM855") << cyrillicPaths;
    QTest::newRow("Turkmen") << QLocale(QLocale::Turkmen, QLocale::Turkmenistan) << QFINDTESTDATA("data/cp852-names.zip") << QByteArray("IBM852")
                             << QStringList{QStringLiteral("Úlohy/"), QStringLiteral("Úlohy/Varianta 1.txt"), QStringLiteral("Odpovědi.txt")};
    QTest::newRow("Filipino") << QLocale(QLocale::Filipino, QLocale::Philippines) << QFINDTESTDATA("data/cp437-names.zip") << QByteArray("IBM437")
                              << QStringList{QStringLiteral("¢.txt")};
    QTest::newRow("English South Africa") << QLocale(QLocale::English, QLocale::SouthAfrica) << QFINDTESTDATA("data/cp437-names.zip") << QByteArray("IBM437")
                                          << QStringList{QStringLiteral("¢.txt")};
    QTest::newRow("Urdu") << QLocale(QLocale::Urdu, QLocale::Pakistan) << QFINDTESTDATA("data/cp720-names.zip") << QByteArray("IBM720")
                          << QStringList{QStringLiteral("إجابات.txt")};

    const QString cp720Archive = QFINDTESTDATA("data/cp720-names.zip");
    const QStringList arabicPaths{QStringLiteral("إجابات.txt")};
    QTest::newRow("Punjabi Arabic") << QLocale(QLocale::Punjabi, QLocale::ArabicScript, QLocale::Pakistan) << cp720Archive << QByteArray("IBM720")
                                    << arabicPaths;
    QTest::newRow("Kurdish Arabic Iraq") << QLocale(QLocale::Kurdish, QLocale::ArabicScript, QLocale::Iraq) << cp720Archive << QByteArray("IBM720")
                                         << arabicPaths;
    QTest::newRow("Sindhi Arabic") << QLocale(QLocale::Sindhi, QLocale::ArabicScript, QLocale::Pakistan) << cp720Archive << QByteArray("IBM720") << arabicPaths;
    QTest::newRow("Inuktitut Latin") << QLocale(QLocale::Inuktitut, QLocale::LatinScript, QLocale::Canada) << QFINDTESTDATA("data/cp437-names.zip")
                                     << QByteArray("IBM437") << QStringList{QStringLiteral("¢.txt")};
    QTest::newRow("Korean South Korea") << QLocale(QLocale::Korean, QLocale::SouthKorea) << QFINDTESTDATA("data/cp949-names.zip") << QByteArray("windows-949")
                                        << QStringList{QStringLiteral("가.txt")};

    const QString cp437Archive = QFINDTESTDATA("data/cp437-names.zip");
    const QStringList fallbackPaths{QStringLiteral("¢.txt")};
    QTest::newRow("Punjabi Gurmukhi fallback") << QLocale(QLocale::Punjabi, QLocale::GurmukhiScript, QLocale::India) << cp437Archive << QByteArray()
                                               << fallbackPaths;
    QTest::newRow("Tamazight Latin Morocco fallback") << QLocale(QLocale::CentralAtlasTamazight, QLocale::LatinScript, QLocale::Morocco) << cp437Archive
                                                      << QByteArray() << fallbackPaths;
    QTest::newRow("Kurdish Latin fallback") << QLocale(QLocale::Kurdish, QLocale::LatinScript, QLocale::Turkey) << cp437Archive << QByteArray()
                                            << fallbackPaths;
    QTest::newRow("Kurdish Arabic Iran fallback") << QLocale(QLocale::Kurdish, QLocale::ArabicScript, QLocale::Iran) << cp437Archive << QByteArray()
                                                  << fallbackPaths;
    QTest::newRow("Sindhi Devanagari fallback") << QLocale(QLocale::Sindhi, QLocale::DevanagariScript, QLocale::India) << cp437Archive << QByteArray()
                                                << fallbackPaths;
    QTest::newRow("Inuktitut Canadian Aboriginal fallback")
        << QLocale(QLocale::Inuktitut, QLocale::CanadianAboriginalScript, QLocale::Canada) << cp437Archive << QByteArray() << fallbackPaths;
    QTest::newRow("Korean North Korea fallback") << QLocale(QLocale::Korean, QLocale::NorthKorea) << QFINDTESTDATA("data/cp949-names.zip") << QByteArray()
                                                 << QStringList{QStringLiteral("░í.txt")};
}

void LibzipTest::testLegacyCodePageNames()
{
    if (!m_plugin || !m_plugin->isValid()) {
        QSKIP("libzip plugin not available. Skipping test.", SkipSingle);
    }
    QFETCH(QByteArray, codePage);
    if (!codePage.isEmpty() && !QStringDecoder(codePage.constData()).isValid()) {
        QSKIP("No decoder for the OEM code page. Skipping test.", SkipSingle);
    }

    const QLocale defaultLocale;
    QFETCH(QLocale, locale);
    QLocale::setDefault(locale);
    const auto restoreLocale = qScopeGuard([&defaultLocale] {
        QLocale::setDefault(defaultLocale);
    });

    QFETCH(QString, archivePath);
    const auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);

    QStringList paths;
    connect(loadJob, &Job::newEntry, this, [&paths](Archive::Entry *entry) {
        paths << entry->fullPath();
    });
    const auto archive = loadJob->archive();
    TestHelper::startAndWaitForResult(loadJob);

    QFETCH(QStringList, expectedPaths);
    QCOMPARE(paths, expectedPaths);

    QVERIFY(archive);
    QVERIFY(archive->isValid());

    const QTemporaryDir destDir;
    QVERIFY(destDir.isValid());

    const QString entryPath = expectedPaths.at(expectedPaths.size() > 1 ? 1 : 0);
    Archive::Entry entry(this, entryPath, QFileInfo(entryPath).path());
    const auto extractionJob = archive->extractFiles({&entry}, destDir.path(), ExtractionOptions());
    QVERIFY(extractionJob);
    TestHelper::startAndWaitForResult(extractionJob);

    QVERIFY(QDir(destDir.path()).exists(entryPath));
}

void LibzipTest::testReplaceLegacyCodePageEntry()
{
    if (!m_plugin || !m_plugin->isValid()) {
        QSKIP("libzip plugin not available. Skipping test.", SkipSingle);
    }
    if (!QStringDecoder("IBM866").isValid()) {
        QSKIP("No decoder for IBM866. Skipping test.", SkipSingle);
    }

    const QLocale defaultLocale;
    QLocale::setDefault(QLocale(QLocale::Russian, QLocale::Russia));
    const auto restoreLocale = qScopeGuard([&defaultLocale] {
        QLocale::setDefault(defaultLocale);
    });

    QTemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    const QString archivePath = temporaryDir.filePath(QStringLiteral("cp866-names.zip"));
    QVERIFY(QFile::copy(QFINDTESTDATA("data/cp866-names.zip"), archivePath));

    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);
    auto archive = loadJob->archive();
    TestHelper::startAndWaitForResult(loadJob);
    QVERIFY(archive);
    QVERIFY(archive->isValid());

    const QString fileName = QStringLiteral("Ответы.txt");
    const QByteArray replacement("replacement contents\n");
    QFile source(temporaryDir.filePath(fileName));
    QVERIFY(source.open(QIODevice::WriteOnly));
    QCOMPARE(source.write(replacement), replacement.size());
    source.close();

    Archive::Entry entry(this, source.fileName(), QString());
    CompressionOptions options;
    options.setGlobalWorkDir(temporaryDir.path());
    auto addJob = archive->addFiles({&entry}, nullptr, options);
    QVERIFY(addJob);
    int addError = -1;
    connect(addJob, &KJob::result, this, [&addError](KJob *job) {
        addError = job->error();
    });
    TestHelper::startAndWaitForResult(addJob);
    QCOMPARE(addError, 0);

    QStringList paths;
    auto reloadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(reloadJob);
    connect(reloadJob, &Job::newEntry, this, [&paths](Archive::Entry *entry) {
        paths << entry->fullPath();
    });
    const auto reloadedArchive = reloadJob->archive();
    TestHelper::startAndWaitForResult(reloadJob);
    QCOMPARE(paths, QStringList({QStringLiteral("Задания/"), QStringLiteral("Задания/Вариант 1.txt"), fileName}));

    QTemporaryDir destination;
    QVERIFY(destination.isValid());
    Archive::Entry archivedEntry(this, fileName, QString());
    auto extractionJob = reloadedArchive->extractFiles({&archivedEntry}, destination.path(), ExtractionOptions());
    QVERIFY(extractionJob);
    int extractionError = -1;
    connect(extractionJob, &KJob::result, this, [&extractionError](KJob *job) {
        extractionError = job->error();
    });
    TestHelper::startAndWaitForResult(extractionJob);
    QCOMPARE(extractionError, 0);
    QFile extracted(destination.filePath(fileName));
    QVERIFY(extracted.open(QIODevice::ReadOnly));
    QCOMPARE(extracted.readAll(), replacement);
}

#include "moc_libziptest.cpp"
