/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "abstractaddtest.h"
#include "archiveentry.h"
#include "jobs.h"
#include "testhelper.h"

#include <QTest>

using namespace Kerfuffle;

class CopyTest : public AbstractAddTest
{
    Q_OBJECT

public:
    CopyTest()
        : AbstractAddTest()
    {
    }

private Q_SLOTS:
    void testCopying_data();
    void testCopying();
};

QTEST_GUILESS_MAIN(CopyTest)

void CopyTest::testCopying_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<Plugin *>("plugin");
    QTest::addColumn<QList<Archive::Entry *>>("targetEntries");
    QTest::addColumn<Archive::Entry *>("destination");
    QTest::addColumn<QStringList>("expectedNewPaths");
    QTest::addColumn<uint>("numberOfEntries");

    setupRows(QStringLiteral("copy a single file"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("a.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{QStringLiteral("empty_dir/a.txt")},
              14);

    setupRows(QStringLiteral("copy several files"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")), new Archive::Entry(this, QStringLiteral("b.txt"))},
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{
                  QStringLiteral("empty_dir/a.txt"),
                  QStringLiteral("empty_dir/b.txt"),
              },
              15);

    setupRows(QStringLiteral("copy a root directory"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{
                  QStringLiteral("empty_dir/dir1/"),
                  QStringLiteral("empty_dir/dir1/dir/"),
                  QStringLiteral("empty_dir/dir1/dir/a.txt"),
                  QStringLiteral("empty_dir/dir1/dir/b.txt"),
                  QStringLiteral("empty_dir/dir1/a.txt"),
                  QStringLiteral("empty_dir/dir1/b.txt"),
              },
              19);

    setupRows(QStringLiteral("copy a root directory 2"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir2/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{
                  QStringLiteral("empty_dir/dir2/"),
                  QStringLiteral("empty_dir/dir2/dir/"),
                  QStringLiteral("empty_dir/dir2/dir/a.txt"),
                  QStringLiteral("empty_dir/dir2/dir/b.txt"),
              },
              17);

    setupRows(QStringLiteral("copy a root directory 3"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir2/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("dir1/")),
              QStringList{
                  QStringLiteral("dir1/dir2/"),
                  QStringLiteral("dir1/dir2/dir/"),
                  QStringLiteral("dir1/dir2/dir/a.txt"),
                  QStringLiteral("dir1/dir2/dir/b.txt"),
              },
              17);

    setupRows(QStringLiteral("copy a directory"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{QStringLiteral("empty_dir/dir/"), QStringLiteral("empty_dir/dir/a.txt"), QStringLiteral("empty_dir/dir/b.txt")},
              16);

    setupRows(QStringLiteral("copy several directories"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/b.txt")),
                  new Archive::Entry(this, QStringLiteral("dir2/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{
                  QStringLiteral("empty_dir/dir1/"),
                  QStringLiteral("empty_dir/dir1/dir/"),
                  QStringLiteral("empty_dir/dir1/dir/a.txt"),
                  QStringLiteral("empty_dir/dir1/dir/b.txt"),
                  QStringLiteral("empty_dir/dir1/a.txt"),
                  QStringLiteral("empty_dir/dir1/b.txt"),
                  QStringLiteral("empty_dir/dir2/"),
                  QStringLiteral("empty_dir/dir2/dir/"),
                  QStringLiteral("empty_dir/dir2/dir/a.txt"),
                  QStringLiteral("empty_dir/dir2/dir/b.txt"),
              },
              23);

    setupRows(QStringLiteral("copy several entries"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{
                  QStringLiteral("empty_dir/dir/"),
                  QStringLiteral("empty_dir/dir/a.txt"),
                  QStringLiteral("empty_dir/dir/b.txt"),
                  QStringLiteral("empty_dir/a.txt"),
                  QStringLiteral("empty_dir/b.txt"),
              },
              18);

    setupRows(QStringLiteral("copy a directory inside itself"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("dir1/")),
              QStringList{
                  QStringLiteral("dir1/dir1/"),
                  QStringLiteral("dir1/dir1/dir/"),
                  QStringLiteral("dir1/dir1/dir/a.txt"),
                  QStringLiteral("dir1/dir1/dir/b.txt"),
                  QStringLiteral("dir1/dir1/a.txt"),
                  QStringLiteral("dir1/dir1/b.txt"),
              },
              19);

    setupRows(QStringLiteral("copy a directory to the root"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
              },
              new Archive::Entry(this, QString()),
              QStringList{
                  QStringLiteral("dir/"),
                  QStringLiteral("dir/a.txt"),
                  QStringLiteral("dir/b.txt"),
              },
              16);
}

void CopyTest::testCopying()
{
    QTemporaryDir temporaryDir;

    QFETCH(QString, archiveName);
    const QString archivePath = temporaryDir.path() + QLatin1Char('/') + archiveName;
    QVERIFY(QFile::copy(QFINDTESTDATA(QStringLiteral("data/") + archiveName), archivePath));

    QFETCH(Plugin *, plugin);
    QVERIFY(plugin);

    auto loadJob = Archive::load(archivePath, plugin);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);
    // This job needs to be delete before the CopyJob starts.
    delete loadJob;

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QFETCH(QList<Archive::Entry *>, targetEntries);
    QFETCH(Archive::Entry *, destination);
    QFETCH(QStringList, expectedNewPaths);

    // Retrieve current paths in the archive.
    QStringList oldPaths = getEntryPaths(archive);

    // Check that the entries to be copied are in the archive.
    for (const auto entry : std::as_const(targetEntries)) {
        QVERIFY(oldPaths.contains(entry->fullPath()));
    }

    // Check that the expected paths (after the CopyJob) are not in the archive.
    for (const auto &expectedPath : std::as_const(expectedNewPaths)) {
        QVERIFY(!oldPaths.contains(expectedPath));
    }

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA("data"));
    CopyJob *copyJob = archive->copyFiles(targetEntries, destination, options);
    TestHelper::startAndWaitForResult(copyJob);

    // Retrieve the resulting paths.
    QStringList newPaths = getEntryPaths(archive);

    // Check that the expected paths are now in the archive.
    for (const auto &path : std::as_const(expectedNewPaths)) {
        QVERIFY(newPaths.contains(path));
    }

    // Check also that the target paths are still in the archive.
    for (const auto entry : std::as_const(targetEntries)) {
        QVERIFY(newPaths.contains(entry->fullPath()));
    }

    QFETCH(uint, numberOfEntries);
    QCOMPARE(archive->numberOfEntries(), numberOfEntries);

    archive->deleteLater();
}

#include "copytest.moc"
