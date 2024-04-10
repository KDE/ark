/*
    SPDX-FileCopyrightText: 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archive_kerfuffle.h"
#include "jobs.h"
#include "pluginmanager.h"
#include "testhelper.h"

#include <KIO/Global>

#include <QDirIterator>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTest>
#if defined(Q_OS_FREEBSD) || defined(Q_OS_LINUX)
#include <sys/stat.h>
#endif

using namespace Kerfuffle;

class ExtractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testExtraction_data();
    void testExtraction();
    void testPreservePermissions_data();
    void testPreservePermissions();

private:
    PluginManager m_pluginManager;
    QString m_expectedWorkingDir;
};

QTEST_GUILESS_MAIN(ExtractTest)

void ExtractTest::initTestCase()
{
    // #395939: after each extraction, the cwd must be the one we started from.
#if defined(Q_OS_FREEBSD) || defined(Q_OS_LINUX)
    umask(0022);
#endif
    m_expectedWorkingDir = QDir::currentPath();
}

void ExtractTest::testExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<QList<Archive::Entry *>>("entriesToExtract");
    QTest::addColumn<ExtractionOptions>("extractionOptions");
    QTest::addColumn<int>("expectedExtractedEntriesCount");

    const auto setupRow = [](const char *name,
                             const QString &archivePath,
                             QList<Archive::Entry *> entriesToExtract,
                             ExtractionOptions extractionOptions,
                             int expectedExtractedEntriesCount) {
        QTest::newRow(name) << archivePath << entriesToExtract << extractionOptions << expectedExtractedEntriesCount;
    };

    ExtractionOptions optionsPreservePaths;

    ExtractionOptions optionsNoPaths;
    optionsNoPaths.setPreservePaths(false);

    ExtractionOptions dragAndDropOptions;
    dragAndDropOptions.setDragAndDropEnabled(true);

    QString archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    setupRow("extract the whole simplearchive.tar.gz", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 4);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    setupRow("extract selected entries from a tar.gz, without paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                 new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    setupRow("extract selected entries from a tar.gz, preserve paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir")),
                 new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
             },
             optionsPreservePaths,
             3);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.gz");
    setupRow("extract selected entries from a tar.gz, drag-and-drop",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("c.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("aDir/b.txt"), QStringLiteral("aDir/")),
             },
             dragAndDropOptions,
             2);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    setupRow("extract the whole one_toplevel_folder.zip", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 9);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    setupRow("extract selected entries from a zip, without paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                 new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    setupRow("extract selected entries from a zip, preserve paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                 new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
             },
             optionsPreservePaths,
             4);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.zip");
    setupRow("extract selected entries from a zip, drag-and-drop",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A/")),
                 new Archive::Entry(this, QStringLiteral("A/B/C/"), QStringLiteral("A/B/")),
                 new Archive::Entry(this, QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/")),
                 new Archive::Entry(this, QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/")),
             },
             dragAndDropOptions,
             4);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    setupRow("extract the whole one_toplevel_folder .7z", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 9);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    setupRow("extract selected entries from a 7z, without paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                 new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    setupRow("extract selected entries from a 7z, preserve paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                 new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
             },
             optionsPreservePaths,
             4);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.7z");
    setupRow("extract selected entries from a 7z, drag-and-drop",
             archivePath,
             QList<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("A/B/test2.txt"), QStringLiteral("A/B/"))},
             dragAndDropOptions,
             1);

    archivePath = QFINDTESTDATA("data/empty_folders.zip");
    setupRow("zip with empty folders", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 5);

    archivePath = QFINDTESTDATA("data/empty_folders.tar.gz");
    setupRow("tar with empty folders", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 5);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    setupRow("extract selected entries from a bzip2-compressed tarball without path",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.bz2");
    setupRow("extract all entries from a bzip2 - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    setupRow("extract selected entries from a xz-compressed tarball without path",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.xz");
    setupRow("extract all entries from a xz - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    setupRow("extract selected entries from a lzma-compressed tarball without path",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lzma");
    setupRow("extract all entries from a lzma - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    setupRow("extract selected entries from a compress (.Z)-compressed tarball without path",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.Z");
    setupRow("extract all entries from a compress (.Z)-compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    setupRow("extract selected entries from a lzipped tarball without path",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.tar.lz");
    setupRow("extract all entries from a lzipped tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

    // Only run tests for lzop compressed files if tar.lzo format is available
    if (PluginManager().supportedMimeTypes().contains(QLatin1String("application/x-tzo"))) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
        setupRow("extract selected entries from a lzop-compressed tarball without path",
                 archivePath,
                 QList<Archive::Entry *>{
                     new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                     new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
                 },
                 optionsNoPaths,
                 2);

        archivePath = QFINDTESTDATA("data/simplearchive.tar.lzo");
        setupRow("extract all entries from a lzop - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

        archivePath = QFINDTESTDATA("data/test.png.lzo");
        setupRow("extract the single - file test.png.lzo", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);
    }

    // Only run tests for lrzipped files if lrzip executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lrzip")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        setupRow("extract selected entries from a lrzip-compressed tarball without path",
                 archivePath,
                 QList<Archive::Entry *>{
                     new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                     new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
                 },
                 optionsNoPaths,
                 2);

        archivePath = QFINDTESTDATA("data/simplearchive.tar.lrz");
        setupRow("extract all entries from a lrzip - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

        archivePath = QFINDTESTDATA("data/test.txt.lrz");
        setupRow("extract the single - file test.txt.lrz", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);
    } else {
        qDebug() << "lrzip executable not found in path. Skipping lrzip test.";
    }

    // Only run tests for zstd-compressed files if zstd executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("zstd")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.zst");
        setupRow("extract selected entries from a zstd-compressed tarball without path",
                 archivePath,
                 QList<Archive::Entry *>{
                     new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                     new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
                 },
                 optionsNoPaths,
                 2);

        archivePath = QFINDTESTDATA("data/simplearchive.tar.zst");
        setupRow("extract all entries from a zst - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

        archivePath = QFINDTESTDATA("data/test.txt.zst");
        setupRow("extract the single - file test.txt.zst", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);
    } else {
        qDebug() << "zstd executable not found in path. Skipping zstd test.";
    }

    // Only run tests for lz4-compressed files if lz4 executable is found in path.
    if (!QStandardPaths::findExecutable(QStringLiteral("lz4")).isEmpty()) {
        archivePath = QFINDTESTDATA("data/simplearchive.tar.lz4");
        setupRow("extract selected entries from a lz4-compressed tarball without path",
                 archivePath,
                 QList<Archive::Entry *>{
                     new Archive::Entry(this, QStringLiteral("file3.txt"), QString()),
                     new Archive::Entry(this, QStringLiteral("dir2/file22.txt"), QString()),
                 },
                 optionsNoPaths,
                 2);

        archivePath = QFINDTESTDATA("data/simplearchive.tar.lz4");
        setupRow("extract all entries from a lz4 - compressed tarball with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

        archivePath = QFINDTESTDATA("data/test.txt.lz4");
        setupRow("extract the single - file test.txt.lz4", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);
    } else {
        qDebug() << "lz4 executable not found in path. Skipping lz4 test.";
    }

    archivePath = QFINDTESTDATA("data/simplearchive.xar");
    setupRow("extract selected entries from a xar archive without path",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("dir1/file11.txt"), QString()),
                 new Archive::Entry(this, QStringLiteral("file4.txt"), QString()),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/simplearchive.xar");
    setupRow("extract all entries from a xar archive with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 6);

    archivePath = QFINDTESTDATA("data/hello-1.0-x86_64.AppImage");
    setupRow("extract all entries from an AppImage with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 7);

    archivePath = QFINDTESTDATA("data/hello-1.0-x86_64.AppImage");
    setupRow("extract selected entries from an AppImage with path",
             archivePath,
             QList<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("usr/bin/hello"), QString())},
             optionsPreservePaths,
             3);

    archivePath = QFINDTESTDATA("data/archive-multivolume.7z.001");
    setupRow("extract all entries from a multivolume 7z archive with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 3);

    archivePath = QFINDTESTDATA("data/archive-multivolume.part1.rar");
    setupRow("extract all entries from a multivolume rar archive with path", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 3);

    archivePath = QFINDTESTDATA("data/firmware-pine64-20160329-6.1.aarch64.rpm");
    setupRow("extract selected entries from rpm with path",
             archivePath,
             QList<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("boot/sunxi-spl.bin"), QString())},
             optionsPreservePaths,
             2);

    archivePath = QFINDTESTDATA("data/firmware-pine64-20160329-6.1.aarch64.rpm");
    setupRow("#369535: broken drag-and-drop from rpm",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("boot/sunxi-spl.bin"), QStringLiteral("boot/")),
                 new Archive::Entry(this, QStringLiteral("boot/u-boot.img"), QStringLiteral("boot/")),
             },
             dragAndDropOptions,
             2);

    archivePath = QFINDTESTDATA("data/bug_#394542.zip");
    setupRow("#394542: libzip doesn't extract selected folder",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("2017 - 05/")),
                 new Archive::Entry(this, QStringLiteral("2017 - 05/uffdå")),
             },
             optionsPreservePaths,
             2);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.arj");
    setupRow("extract the whole one_toplevel_folder.arj", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 9);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.arj");
    setupRow("extract selected entries from a arj, without paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                 new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
             },
             optionsNoPaths,
             2);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.arj");
    setupRow("extract selected entries from a arj, preserve paths",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A")),
                 new Archive::Entry(this, QStringLiteral("A/B/test1.txt"), QStringLiteral("A/B")),
             },
             optionsPreservePaths,
             4);

    archivePath = QFINDTESTDATA("data/one_toplevel_folder.arj");
    setupRow("extract selected entries from a arj, drag-and-drop",
             archivePath,
             QList<Archive::Entry *>{
                 new Archive::Entry(this, QStringLiteral("A/test2.txt"), QStringLiteral("A/")),
                 new Archive::Entry(this, QStringLiteral("A/B/C/"), QStringLiteral("A/B/")),
                 new Archive::Entry(this, QStringLiteral("A/B/C/test1.txt"), QStringLiteral("A/B/")),
                 new Archive::Entry(this, QStringLiteral("A/B/C/test2.txt"), QStringLiteral("A/B/")),
             },
             dragAndDropOptions,
             4);

    archivePath = QFINDTESTDATA("data/test.z");
    setupRow("extract the single - file test.z", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    archivePath = QFINDTESTDATA("data/test.zz");
    setupRow("extract the single - file test.zz", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    archivePath = QFINDTESTDATA("data/test.txt.gz");
    setupRow("extract the single - file test.txt.gz", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    archivePath = QFINDTESTDATA("data/test.txt.bz2");
    setupRow("extract the single - file test.txt.bz2", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    archivePath = QFINDTESTDATA("data/test.png.lzma");
    setupRow("extract the single - file test.png.lzma", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    archivePath = QFINDTESTDATA("data/test.svgz");
    setupRow("extract the single - file test.svgz", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    archivePath = QFINDTESTDATA("data/test.sit");
    setupRow("extract the whole test.sit", archivePath, QList<Archive::Entry *>(), optionsPreservePaths, 1);

    m_expectedWorkingDir = QDir::currentPath();
}

void ExtractTest::testExtraction()
{
    QFETCH(QString, archivePath);
    auto loadJob = Archive::load(archivePath, this);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    QFETCH(QList<Archive::Entry *>, entriesToExtract);
    QFETCH(ExtractionOptions, extractionOptions);
    auto extractionJob = archive->extractFiles(entriesToExtract, destDir.path(), extractionOptions);
    QVERIFY(extractionJob);
    extractionJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(extractionJob);

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    QDirIterator dirIt(destDir.path(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        extractedEntriesCount++;
        dirIt.next();
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);
    QCOMPARE(QDir::currentPath(), m_expectedWorkingDir);

    loadJob->deleteLater();
    extractionJob->deleteLater();
    archive->deleteLater();
}

void ExtractTest::testPreservePermissions_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<Plugin *>("plugin");
    QTest::addColumn<QString>("testFile");
    QTest::addColumn<int>("expectedPermissions");

    // Repeat the same test case for each format and for each plugin supporting the format.
    const QStringList formats = TestHelper::testFormats();
    for (const QString &format : formats) {
        const QString filename = QFINDTESTDATA(QStringLiteral("data/test_permissions.%1").arg(format));
        const auto mime = QMimeDatabase().mimeTypeForFile(filename, QMimeDatabase::MatchExtension);
        const auto plugins = m_pluginManager.preferredWritePluginsFor(mime);
        for (const auto plugin : plugins) {
            QTest::newRow(QStringLiteral("test preserve 0755 permissions (%1, %2)").arg(format, plugin->metaData().pluginId()).toUtf8().constData())
                << filename << plugin << QStringLiteral("0755.sh") << 0755;
        }
    }
}

void ExtractTest::testPreservePermissions()
{
    QFETCH(QString, archiveName);
    QFETCH(Plugin *, plugin);
    QVERIFY(plugin);
    auto loadJob = Archive::load(archiveName, plugin);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    auto extractionJob = archive->extractFiles({}, destDir.path());
    QVERIFY(extractionJob);
    extractionJob->setAutoDelete(false);
    TestHelper::startAndWaitForResult(extractionJob);

    // Check whether extraction preserved the original permissions.
    QFETCH(QString, testFile);
    QFile file(QStringLiteral("%1/%2").arg(destDir.path(), testFile));
    QVERIFY(file.exists());
    QFETCH(int, expectedPermissions);
    const auto expectedQtPermissions = KIO::convertPermissions(expectedPermissions);
    // On Linux we get also the XXXUser flags which are ignored by KIO::convertPermissions(),
    // so we need to remove them before the comparison with the expected permissions.
    QCOMPARE(file.permissions() & expectedQtPermissions, expectedQtPermissions);

    loadJob->deleteLater();
    extractionJob->deleteLater();
    archive->deleteLater();
}

#include "extracttest.moc"
