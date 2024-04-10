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

class MoveTest : public AbstractAddTest
{
    Q_OBJECT

public:
    MoveTest()
        : AbstractAddTest()
    {
    }

private Q_SLOTS:
    void testMoving_data();
    void testMoving();
};

QTEST_GUILESS_MAIN(MoveTest)

void MoveTest::testMoving_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<Plugin *>("plugin");
    QTest::addColumn<QList<Archive::Entry *>>("targetEntries");
    QTest::addColumn<Archive::Entry *>("destination");
    QTest::addColumn<QStringList>("expectedNewPaths");
    QTest::addColumn<uint>("numberOfEntries");

    setupRows(QStringLiteral("replace a single file"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("a.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/a.txt")),
              QStringList{QStringLiteral("empty_dir/a.txt")},
              13);

    setupRows(QStringLiteral("replace several files"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("a.txt")),
                  new Archive::Entry(this, QStringLiteral("b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{QStringLiteral("empty_dir/a.txt"), QStringLiteral("empty_dir/b.txt")},
              13);

    setupRows(QStringLiteral("replace a root directory"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList{QStringLiteral("empty_dir/a.txt"),
                          QStringLiteral("empty_dir/b.txt"),
                          QStringLiteral("empty_dir/dir/"),
                          QStringLiteral("empty_dir/dir/a.txt"),
                          QStringLiteral("empty_dir/dir/b.txt")},
              13);

    setupRows(QStringLiteral("replace a root directory 2"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir2/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir2/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/dir/")),
              QStringList{QStringLiteral("empty_dir/dir/"), QStringLiteral("empty_dir/dir/a.txt"), QStringLiteral("empty_dir/dir/b.txt")},
              13);

    setupRows(QStringLiteral("replace a directory"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/dir/")),
              QStringList{QStringLiteral("empty_dir/dir/"), QStringLiteral("empty_dir/dir/a.txt"), QStringLiteral("empty_dir/dir/b.txt")},
              13);

    setupRows(QStringLiteral("replace several directories"),
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
                  QStringLiteral("empty_dir/dir1/a.txt"),
                  QStringLiteral("empty_dir/dir1/b.txt"),
                  QStringLiteral("empty_dir/dir1/dir/"),
                  QStringLiteral("empty_dir/dir1/dir/a.txt"),
                  QStringLiteral("empty_dir/dir1/dir/b.txt"),
                  QStringLiteral("empty_dir/dir2/"),
                  QStringLiteral("empty_dir/dir2/dir/"),
                  QStringLiteral("empty_dir/dir2/dir/a.txt"),
                  QStringLiteral("empty_dir/dir2/dir/b.txt"),
              },
              13);

    setupRows(QStringLiteral("replace several entries"),
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
              13);

    setupRows(QStringLiteral("move a directory to the root"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{
                  new Archive::Entry(this, QStringLiteral("dir1/dir/")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/a.txt")),
                  new Archive::Entry(this, QStringLiteral("dir1/dir/b.txt")),
              },
              new Archive::Entry(this, QStringLiteral("dir/")),
              QStringList{QStringLiteral("dir/"), QStringLiteral("dir/a.txt"), QStringLiteral("dir/b.txt")},
              13);

    setupRows(QStringLiteral("rename a file in the root (bug #368530)"),
              QStringLiteral("test"),
              QList<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt"))},
              new Archive::Entry(this, QStringLiteral("new-name.txt")),
              QStringList{QStringLiteral("new-name.txt")},
              13);
}

void MoveTest::testMoving()
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

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QFETCH(QList<Archive::Entry *>, targetEntries);
    QFETCH(Archive::Entry *, destination);
    QFETCH(QStringList, expectedNewPaths);

    // Retrieve current paths in the archive.
    QStringList oldPaths = getEntryPaths(archive);

    // Check that the entries to be moved are in the archive.
    for (const auto entry : std::as_const(targetEntries)) {
        QVERIFY(oldPaths.contains(entry->fullPath()));
    }

    // Check that the expected paths (after the MoveJob) are not in the archive.
    for (const auto &expectedPath : std::as_const(expectedNewPaths)) {
        QVERIFY(!oldPaths.contains(expectedPath));
    }

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA("data"));
    MoveJob *moveJob = archive->moveFiles(targetEntries, destination, options);
    TestHelper::startAndWaitForResult(moveJob);

    // Retrieve the resulting paths.
    QStringList newPaths = getEntryPaths(archive);

    // Check that the expected paths are now in the archive.
    for (const auto &path : std::as_const(expectedNewPaths)) {
        QVERIFY(newPaths.contains(path));
    }

    // Check that the target paths are no longer in the archive.
    for (const auto entry : std::as_const(targetEntries)) {
        QVERIFY(!newPaths.contains(entry->fullPath()));
    }

    QFETCH(uint, numberOfEntries);
    QCOMPARE(archive->numberOfEntries(), numberOfEntries);

    loadJob->deleteLater();
    archive->deleteLater();
}

#include "movetest.moc"
