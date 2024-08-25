/*
    SPDX-FileCopyrightText: 2023 Kristen McWilliam <kmcwilliampublic@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

/**
    This test verifies that user metadata set on an archive through Dolphin -
    such as tags, comments, and ratings - are preserved when the archive is
    modified.
 */

#include "abstractaddtest.h"
#include "jobs.h"
#include "testhelper.h"

#include <KIO/CopyJob>

#include <QTest>

using namespace Kerfuffle;

class PreserveMetadataTest : public AbstractAddTest
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void cleanup();

    void addFiles();
    void copyFiles();
    void deleteFiles();
    void encrypt();
    void extractFiles();
    void moveFiles();

private:
    QString archivePath;
    Archive *archive;
    QTemporaryDir temporaryDir;

    const QString TEST_DATA_DIR = QStringLiteral("data");
    const QString ARCHIVE_NAME = QStringLiteral("archive-with-metadata.zip");
    const QStringList EXPECTED_TAGS = QStringList() << QStringLiteral("testTag");
    const int EXPECTED_RATING = 10;
    const QString EXPECTED_COMMENT = QStringLiteral("This is a comment");

    void setMetadata();
    void verifyMetadata();
};

QTEST_GUILESS_MAIN(PreserveMetadataTest)

void PreserveMetadataTest::init()
{
    QVERIFY(temporaryDir.isValid());

    const QString sourceArchive = QFINDTESTDATA(TEST_DATA_DIR + QLatin1Char('/') + ARCHIVE_NAME);
    archivePath = temporaryDir.path() + QLatin1Char('/') + ARCHIVE_NAME;

    // Copy the archive to a temporary directory
    QFile::copy(sourceArchive, archivePath);

    // Set the metadata on the archive
    setMetadata();

    // Verify the archive has the expected metadata before Ark modifies it
    verifyMetadata();

    // Load the archive
    auto loadJob = Archive::load(archivePath);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);
    TestHelper::startAndWaitForResult(loadJob);
    archive = loadJob->archive();
    QVERIFY(archive);
    const uint archiveEntries = archive->numberOfEntries();
    QCOMPARE(archiveEntries, 2);
    loadJob->deleteLater();
}

void PreserveMetadataTest::cleanup()
{
    // Clear the archive from the temporary directory
    QFile::remove(archivePath);
    archive->deleteLater();
}

void PreserveMetadataTest::addFiles()
{
    // Add a file to the archive
    const QString newFileName = QStringLiteral("testfile.txt");
    const QString newFilePath = QFINDTESTDATA(TEST_DATA_DIR + QLatin1Char('/') + newFileName);

    QList<Archive::Entry *> entries;
    entries.append(new Archive::Entry(this, newFilePath));

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA(TEST_DATA_DIR));
    Archive::Entry *destination = new Archive::Entry(this);
    auto addJob = archive->addFiles(entries, destination, options);

    TestHelper::startAndWaitForResult(addJob);

    // Verify the archive now has the expected number of entries
    QCOMPARE(archive->numberOfEntries(), 3);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();
}

void PreserveMetadataTest::copyFiles()
{
    // The name of a file that already exists in the archive
    const QString existingFileName = QStringLiteral("test.txt");

    QList<Archive::Entry *> targetEntries;
    targetEntries.append(new Archive::Entry(this, existingFileName));
    Archive::Entry destination = Archive::Entry(this, QStringLiteral("testCopy"));

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA(TEST_DATA_DIR));
    CopyJob *copyJob = archive->copyFiles(targetEntries, &destination, options);
    TestHelper::startAndWaitForResult(copyJob);

    // Verify the archive now has the expected number of entries
    QCOMPARE(archive->numberOfEntries(), 3);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();
}

void PreserveMetadataTest::deleteFiles()
{
    // The name of the file that will be deleted from the archive
    const QString fileForDeletionName = QStringLiteral("a.txt");

    QList<Archive::Entry *> targetEntries;
    targetEntries.append(new Archive::Entry(this, fileForDeletionName));

    DeleteJob *deleteJob = archive->deleteFiles(targetEntries);
    TestHelper::startAndWaitForResult(deleteJob);

    // Verify the archive now has the expected number of entries
    QCOMPARE(archive->numberOfEntries(), 1);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();
}

void PreserveMetadataTest::encrypt()
{
    QCOMPARE(archive->encryptionType(), Archive::EncryptionType::Unencrypted);

    // The password to use for encryption
    const QString password = QStringLiteral("password");

    // Encrypt the archive
    bool encryptHeader = false;
    archive->encrypt(password, encryptHeader);
    QCOMPARE(archive->encryptionType(), Archive::EncryptionType::Encrypted);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();

    // Extract the archive, including the list of files
    encryptHeader = true;
    archive->encrypt(password, encryptHeader);
    QCOMPARE(archive->encryptionType(), Archive::EncryptionType::HeaderEncrypted);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();
}

void PreserveMetadataTest::extractFiles()
{
    // The name of the file that will be extracted from the archive
    const QString fileForExtractionName = QStringLiteral("test.txt");

    QList<Archive::Entry *> targetEntries;
    targetEntries.append(new Archive::Entry(this, fileForExtractionName));

    // Extract the file from the archive
    const QString destinationDir = temporaryDir.path();
    ExtractJob *extractJob = archive->extractFiles(targetEntries, destinationDir);
    TestHelper::startAndWaitForResult(extractJob);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();
}

void PreserveMetadataTest::moveFiles()
{
    // The name of the file that will be moved within the archive
    const QString fileForMoveName = QStringLiteral("test.txt");

    QList<Archive::Entry *> targetEntries;
    targetEntries.append(new Archive::Entry(this, fileForMoveName));

    Archive::Entry destination = Archive::Entry(this, QStringLiteral("test-moved.txt"));

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA(TEST_DATA_DIR));
    MoveJob *moveJob = archive->moveFiles(targetEntries, &destination, options);
    TestHelper::startAndWaitForResult(moveJob);

    // Verify the archive still has the expected number of entries
    QCOMPARE(archive->numberOfEntries(), 2);

    // Verify the archive still has the expected metadata after Ark modified it
    verifyMetadata();
}

/**
 * Set the metadata on the archive to the expected values.
 */
void PreserveMetadataTest::setMetadata()
{
    KFileMetaData::UserMetaData metaData(archivePath);
    metaData.setTags(EXPECTED_TAGS);
    metaData.setRating(EXPECTED_RATING);
    metaData.setUserComment(EXPECTED_COMMENT);
}

/**
 * Verify the metadata on the archive matches the expected values.
 */
void PreserveMetadataTest::verifyMetadata()
{
    KFileMetaData::UserMetaData metaData(archivePath);
    QCOMPARE(metaData.tags(), EXPECTED_TAGS);
    QCOMPARE(metaData.rating(), EXPECTED_RATING);
    QCOMPARE(metaData.userComment(), EXPECTED_COMMENT);
}

#include "preservemetadatatest.moc"
