/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "cliunarchivertest.h"
#include "jobs.h"
#include "testhelper.h"

#include <QDirIterator>
#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

QTEST_GUILESS_MAIN(CliUnarchiverTest)

using namespace Kerfuffle;

void CliUnarchiverTest::initTestCase()
{
    m_plugin = new Plugin(this);
    const auto plugins = m_pluginManger.availablePlugins();
    for (Plugin *plugin : plugins) {
        if (plugin->metaData().pluginId() == QLatin1String("kerfuffle_cliunarchiver")) {
            m_plugin = plugin;
            return;
        }
    }
}

void CliUnarchiverTest::testArchive_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QString>("expectedFileName");
    QTest::addColumn<bool>("isReadOnly");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QString>("expectedSubfolderName");

    QString archivePath = QFINDTESTDATA("data/one_toplevel_folder.rar");
    QTest::newRow("RAR archive with one top-level folder")
        << archivePath << QFileInfo(archivePath).fileName() << true << true << Archive::Unencrypted << QStringLiteral("A");

    archivePath = QFINDTESTDATA("data/multiple_toplevel_entries.rar");
    QTest::newRow("RAR archive with multiple top-level entries")
        << archivePath << QFileInfo(archivePath).fileName() << true << false << Archive::Unencrypted << QStringLiteral("multiple_toplevel_entries");

    archivePath = QFINDTESTDATA("data/encrypted_entries.rar");
    QTest::newRow("RAR archive with encrypted entries")
        << archivePath << QFileInfo(archivePath).fileName() << true << true << Archive::Encrypted << QStringLiteral("A");

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.lha");
    QTest::newRow("LHA archive with one top-level folder")
        << archivePath << QFileInfo(archivePath).fileName() << true << true << Archive::Unencrypted << QStringLiteral("A");

    archivePath = QFINDTESTDATA("data/multiple_toplevel_entries.lha");
    QTest::newRow("LHA archive with multiple top-level entries")
        << archivePath << QFileInfo(archivePath).fileName() << true << false << Archive::Unencrypted << QStringLiteral("multiple_toplevel_entries");

    archivePath = QFINDTESTDATA("data/test.sit");
    QTest::newRow("stuffit archive unencrypted") << archivePath << QFileInfo(archivePath).fileName() << true << false << Archive::Unencrypted
                                                 << QStringLiteral("test");
}

void CliUnarchiverTest::testArchive()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliunarchiver plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cliunarchiver plugin. Skipping test.", SkipSingle);
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

    QTest::newRow("archive with one top-level folder") << QFINDTESTDATA("data/one_toplevel_folder.json") << 9 << 6 << QStringLiteral("A/B/C/") << true << false
                                                       << (qulonglong)0 << (qulonglong)0 << QStringLiteral("2015-12-21T16:57:20+01:00");
    QTest::newRow("archive with multiple top-level entries")
        << QFINDTESTDATA("data/multiple_toplevel_entries.json") << 12 << 4 << QStringLiteral("data/A/B/test1.txt") << false << false << (qulonglong)7
        << (qulonglong)7 << QStringLiteral("2015-12-21T16:56:28+01:00");
    QTest::newRow("archive with encrypted entries") << QFINDTESTDATA("data/encrypted_entries.json") << 9 << 5 << QStringLiteral("A/test1.txt") << false << true
                                                    << (qulonglong)7 << (qulonglong)32 << QStringLiteral("2015-12-21T16:56:28+01:00");
    QTest::newRow("huge archive") << QFINDTESTDATA("data/huge_archive.json") << 250 << 8 << QStringLiteral("PsycOPacK/Base Dictionnaries/att800") << false
                                  << false << (qulonglong)593687 << (qulonglong)225219 << QStringLiteral("2011-08-14T03:10:10+02:00");
}

void CliUnarchiverTest::testList()
{
    qRegisterMetaType<Archive::Entry *>("Archive::Entry*");
    CliPlugin *plugin = new CliPlugin(this, {QStringLiteral("dummy.rar"), QVariant::fromValue(m_plugin->metaData())});
    QSignalSpy signalSpy(plugin, &CliPlugin::entry);

    QFETCH(QString, jsonFilePath);
    QFETCH(int, expectedEntriesCount);

    QFile jsonFile(jsonFilePath);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));

    QTextStream stream(&jsonFile);
    plugin->setJsonOutput(stream.readAll());

    QCOMPARE(signalSpy.count(), expectedEntriesCount);

    QFETCH(int, someEntryIndex);
    QVERIFY(someEntryIndex < signalSpy.count());
    Archive::Entry *entry = signalSpy.at(someEntryIndex).at(0).value<Archive::Entry *>();

    QFETCH(QString, expectedName);
    QCOMPARE(entry->fullPath(), expectedName);

    QFETCH(bool, isDirectory);
    QCOMPARE(entry->isDir(), isDirectory);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(entry->property("isPasswordProtected").toBool(), isPasswordProtected);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry->property("size").toULongLong(), expectedSize);

    QFETCH(qulonglong, expectedCompressedSize);
    QCOMPARE(entry->property("compressedSize").toULongLong(), expectedCompressedSize);

    QFETCH(QString, expectedTimestamp);
    QEXPECT_FAIL("", "Something changed since Qt 5.11, needs investigation.", Continue);
    QCOMPARE(entry->property("timestamp").toString(), expectedTimestamp);

    plugin->deleteLater();
}

void CliUnarchiverTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted") << QStringLiteral("/tmp/foo.rar") << QString() << QStringList{QStringLiteral("-json"), QStringLiteral("/tmp/foo.rar")};

    QTest::newRow("header-encrypted") << QStringLiteral("/tmp/foo.rar") << QStringLiteral("1234")
                                      << QStringList{QStringLiteral("-json"),
                                                     QStringLiteral("-password"),
                                                     QStringLiteral("1234"),
                                                     QStringLiteral("/tmp/foo.rar")};
}

void CliUnarchiverTest::testListArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliunarchiver plugin not available. Skipping test.", SkipSingle);
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

void CliUnarchiverTest::testExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QVector<Archive::Entry *>>("entriesToExtract");
    QTest::addColumn<ExtractionOptions>("extractionOptions");
    QTest::addColumn<int>("expectedExtractedEntriesCount");

    ExtractionOptions defaultOptions;
    defaultOptions.setAlwaysUseTempDir(true);

    ExtractionOptions optionsNoPaths = defaultOptions;
    optionsNoPaths.setPreservePaths(false);

    ExtractionOptions dragAndDropOptions = defaultOptions;
    dragAndDropOptions.setDragAndDropEnabled(true);

    QTest::newRow("extract the whole multiple_toplevel_entries.rar")
        << QFINDTESTDATA("data/multiple_toplevel_entries.rar") << QVector<Archive::Entry *>() << defaultOptions << 12;

    QTest::newRow("extract selected entries from a rar, without paths")
        << QFINDTESTDATA("data/one_toplevel_folder.rar")
        << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                                     new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B"))}
        << optionsNoPaths << 2;

    QTest::newRow("extract selected entries from a rar, preserve paths")
        << QFINDTESTDATA("data/one_toplevel_folder.rar")
        << (QVector<Archive::Entry *>{
               new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
               new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
           })
        << defaultOptions << 4;

    QTest::newRow("extract selected entries from a rar, drag-and-drop")
        << QFINDTESTDATA("data/one_toplevel_folder.rar")
        << (QVector<Archive::Entry *>{
               new Archive::Entry(this, QStringLiteral("A/B/C/"), QStringLiteral("A/B/")),
               new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A/")),
               new Archive::Entry(this, QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/")),
               new Archive::Entry(this, QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/")),
           })
        << dragAndDropOptions << 4;

    QTest::newRow("rar with empty folders") << QFINDTESTDATA("data/empty_folders.rar") << QVector<Archive::Entry *>() << defaultOptions << 5;

    QTest::newRow("rar with hidden folder and files") << QFINDTESTDATA("data/hidden_files.rar") << QVector<Archive::Entry *>() << defaultOptions << 4;
}

// TODO: we can remove this test (which is duplicated from kerfuffle/archivetest)
// if we ever ends up using a temp dir for any cliplugin, instead of only for cliunarchiver.
void CliUnarchiverTest::testExtraction()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliunarchiver plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, m_plugin, this);
    QVERIFY(loadJob);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not load the cliunarchiver plugin. Skipping test.", SkipSingle);
    }

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    QFETCH(QVector<Archive::Entry *>, entriesToExtract);
    QFETCH(ExtractionOptions, extractionOptions);
    auto extractionJob = archive->extractFiles(entriesToExtract, destDir.path(), extractionOptions);

    QEventLoop eventLoop(this);
    connect(extractionJob, &KJob::result, &eventLoop, &QEventLoop::quit);
    extractionJob->start();
    eventLoop.exec(); // krazy:exclude=crashy

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    QDirIterator dirIt(destDir.path(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        extractedEntriesCount++;
        dirIt.next();
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);

    archive->deleteLater();
}

void CliUnarchiverTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QVector<Archive::Entry *>>("files");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("encrypted, multiple files") << QStringLiteral("/tmp/foo.rar")
                                               << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                                                                            new Archive::Entry(this, QStringLiteral("c.txt"), QString())}
                                               << QStringLiteral("1234")
                                               << QStringList{
                                                      QStringLiteral("-D"),
                                                      QStringLiteral("-password"),
                                                      QStringLiteral("1234"),
                                                      QStringLiteral("/tmp/foo.rar"),
                                                      QStringLiteral("aDir/b.txt"),
                                                      QStringLiteral("c.txt"),
                                                  };

    QTest::newRow("unencrypted, multiple files") << QStringLiteral("/tmp/foo.rar")
                                                 << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                                                                              new Archive::Entry(this, QStringLiteral("c.txt"), QString())}
                                                 << QString()
                                                 << QStringList{
                                                        QStringLiteral("-D"),
                                                        QStringLiteral("/tmp/foo.rar"),
                                                        QStringLiteral("aDir/b.txt"),
                                                        QStringLiteral("c.txt"),
                                                    };
}

void CliUnarchiverTest::testExtractArgs()
{
    if (!m_plugin->isValid()) {
        QSKIP("cliunarchiver plugin not available. Skipping test.", SkipSingle);
    }

    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName), QVariant::fromValue(m_plugin->metaData())});
    QVERIFY(plugin);

    QFETCH(QVector<Archive::Entry *>, files);
    QStringList filesList;
    for (const Archive::Entry *e : std::as_const(files)) {
        filesList << e->fullPath(NoTrailingSlash);
    }

    QFETCH(QString, password);

    const auto replacedArgs = plugin->cliProperties()->extractArgs(archiveName, filesList, false, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

#include "moc_cliunarchivertest.cpp"
