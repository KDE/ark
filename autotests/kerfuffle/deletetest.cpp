/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archiveentry.h"
#include "jobs.h"
#include "pluginmanager.h"
#include "testhelper.h"

#include <QMimeDatabase>
#include <QTest>

using namespace Kerfuffle;

class DeleteTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDelete_data();
    void testDelete();

private:
    PluginManager m_pluginManager;
};

QTEST_GUILESS_MAIN(DeleteTest)

void DeleteTest::testDelete_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<Plugin*>("plugin");
    QTest::addColumn<QVector<Archive::Entry*>>("targetEntries");
    QTest::addColumn<uint>("expectedEntriesCount");
    QTest::addColumn<uint>("expectedRemainingEntriesCount");

    // Repeat the same test case for each format and for each plugin supporting the format.
    const QStringList formats = TestHelper::testFormats();
    for (const QString &format : formats) {
        const QString filename = QStringLiteral("test.%1").arg(format);
        const auto mime = QMimeDatabase().mimeTypeForFile(filename, QMimeDatabase::MatchExtension);

        const auto plugins = m_pluginManager.preferredWritePluginsFor(mime);
        for (const auto plugin : plugins) {
            QTest::newRow(qPrintable(QStringLiteral("delete a single file (%1, %2)").arg(format, plugin->metaData().pluginId())))
                << filename
                << plugin
                << QVector<Archive::Entry*> { new Archive::Entry(this, QStringLiteral("dir1/a.txt")) }
                << 13u
                << 12u;

            QTest::newRow(qPrintable(QStringLiteral("delete multiple files (%1, %2)").arg(format, plugin->metaData().pluginId())))
                << filename
                << plugin
                << QVector<Archive::Entry*> {
                       new Archive::Entry(this, QStringLiteral("a.txt")),
                       new Archive::Entry(this, QStringLiteral("dir1/b.txt"))
                   }
                << 13u
                << 11u;
        }
    }
}

void DeleteTest::testDelete()
{
    QTemporaryDir temporaryDir;

    QFETCH(QString, archiveName);
    const QString archivePath = QStringLiteral("%1/%2").arg(temporaryDir.path(), archiveName);
    QVERIFY(QFile::copy(QFINDTESTDATA(QStringLiteral("data/%1").arg(archiveName)), archivePath));

    QFETCH(Plugin*, plugin);
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

    QFETCH(uint, expectedEntriesCount);
    QCOMPARE(archive->numberOfEntries(), expectedEntriesCount);

    QFETCH(QVector<Archive::Entry*>, targetEntries);
    auto deleteJob = archive->deleteFiles(targetEntries);
    QVERIFY(deleteJob);
    TestHelper::startAndWaitForResult(deleteJob);

    QFETCH(uint, expectedRemainingEntriesCount);
    QCOMPARE(archive->numberOfEntries(), expectedRemainingEntriesCount);

    loadJob->deleteLater();
    archive->deleteLater();
}

#include "deletetest.moc"
