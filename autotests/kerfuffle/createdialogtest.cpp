/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "createdialog.h"
#include "pluginmanager.h"

#include <KCollapsibleGroupBox>

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QTest>

using namespace Kerfuffle;

class CreateDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testBasicWidgets_data();
    void testBasicWidgets();
    void testEncryption_data();
    void testEncryption();
    void testHeaderEncryptionTooltip();

private:
    PluginManager m_pluginManager;
};

void CreateDialogTest::testBasicWidgets_data()
{
    QTest::addColumn<QString>("mimeType");

    QTest::newRow("tar") << QStringLiteral("application/x-tar");
    QTest::newRow("targzip") << QStringLiteral("application/x-compressed-tar");
    QTest::newRow("tarbzip") << QStringLiteral("application/x-bzip-compressed-tar");
    QTest::newRow("tarZ") << QStringLiteral("application/x-tarz");
    QTest::newRow("tarxz") << QStringLiteral("application/x-xz-compressed-tar");
    QTest::newRow("tarlzma") << QStringLiteral("application/x-lzma-compressed-tar");
    QTest::newRow("tarlzip") << QStringLiteral("application/x-lzip-compressed-tar");

    const auto writeMimeTypes = m_pluginManager.supportedWriteMimeTypes();

    if (writeMimeTypes.contains(QLatin1String("application/zip"))) {
        QTest::newRow("zip") << QStringLiteral("application/zip");
    } else {
        qDebug() << "zip format not available in CreateDialog, skipping test.";
    }

    if (writeMimeTypes.contains(QLatin1String("application/x-7z-compressed"))) {
        QTest::newRow("7z") << QStringLiteral("application/x-7z-compressed");
    } else {
        qDebug() << "7z format not available in CreateDialog, skipping test.";
    }

    if (writeMimeTypes.contains(QLatin1String("application/vnd.rar"))) {
        QTest::newRow("rar") << QStringLiteral("application/vnd.rar");
    } else {
        qDebug() << "rar format not available in CreateDialog, skipping test.";
    }

    if (writeMimeTypes.contains(QLatin1String("application/x-lrzip-compressed-tar"))) {
        QTest::newRow("tarlrzip") << QStringLiteral("application/x-lrzip-compressed-tar");
    } else {
        qDebug() << "tar.lrzip format not available in CreateDialog, skipping test.";
    }

    if (writeMimeTypes.contains(QLatin1String("application/x-tzo"))) {
        QTest::newRow("tarlzop") << QStringLiteral("application/x-tzo");
    } else {
        qDebug() << "tar.lzo format not available in CreateDialog, skipping test.";
    }
}

void CreateDialogTest::testBasicWidgets()
{
    CreateDialog *dialog = new CreateDialog(nullptr, QString(), QUrl());

    auto fileNameLineEdit = dialog->findChild<QLineEdit*>(QStringLiteral("filenameLineEdit"));
    auto archiveTypeComboBox = dialog->findChild<QComboBox*>(QStringLiteral("mimeComboBox"));
    QVERIFY(fileNameLineEdit);
    QVERIFY(archiveTypeComboBox);

    QFETCH(QString, mimeType);
    const QMimeType mime = QMimeDatabase().mimeTypeForName(mimeType);

    // Test if combobox is updated when user enters a filename with suffix.
    fileNameLineEdit->setText(QStringLiteral("basename.%1").arg(mime.preferredSuffix()));
    QCOMPARE(archiveTypeComboBox->currentText(), mime.comment());

    // Test if suffix is added correctly when the user selects an archive type in combobox.
    fileNameLineEdit->setText(QStringLiteral("basename"));
    archiveTypeComboBox->setCurrentText(mime.comment());
    QCOMPARE(QFileInfo(dialog->selectedUrl().toLocalFile()).fileName(), QStringLiteral("basename.%1").arg(mime.preferredSuffix()));
}

void CreateDialogTest::testEncryption_data()
{
    QTest::addColumn<QString>("filter");
    QTest::addColumn<bool>("isEncryptionAvailable");
    QTest::addColumn<bool>("isHeaderEncryptionAvailable");

    QTest::newRow("tar") << QStringLiteral("application/x-compressed-tar") << false << false;

    if (m_pluginManager.supportedWriteMimeTypes().contains(QLatin1String("application/zip"))) {
        QTest::newRow("zip") << QStringLiteral("application/zip") << true << false;
    } else {
        qDebug() << "zip format not available in CreateDialog, skipping test.";
    }

    if (m_pluginManager.supportedWriteMimeTypes().contains(QLatin1String("application/x-7z-compressed"))) {
        QTest::newRow("7z") << QStringLiteral("application/x-7z-compressed") << true << true;
    } else {
        qDebug() << "7z format not available in CreateDialog, skipping test.";
    }

    if (m_pluginManager.supportedWriteMimeTypes().contains(QLatin1String("application/vnd.rar"))) {
        QTest::newRow("rar") << QStringLiteral("application/vnd.rar") << true << true;
    } else {
        qDebug() << "rar format not available in CreateDialog, skipping test.";
    }
}

void CreateDialogTest::testEncryption()
{
    CreateDialog *dialog = new CreateDialog(nullptr, QString(), QUrl());

    QFETCH(QString, filter);
    QFETCH(bool, isEncryptionAvailable);
    QFETCH(bool, isHeaderEncryptionAvailable);

    auto collapsibleEncryption = dialog->findChild<KCollapsibleGroupBox*>(QStringLiteral("collapsibleEncryption"));
    auto encryptHeaderCheckBox = dialog->findChild<QCheckBox*>(QStringLiteral("encryptHeaderCheckBox"));
    QVERIFY(collapsibleEncryption);
    QVERIFY(encryptHeaderCheckBox);

    QVERIFY(dialog->setMimeType(filter));

    // Encryption is initially not enabled.
    QVERIFY(!dialog->isEncryptionEnabled());
    QVERIFY(!dialog->isHeaderEncryptionEnabled());

    if (isEncryptionAvailable) {
        QVERIFY(dialog->isEncryptionAvailable());

        collapsibleEncryption->setExpanded(true);
        QVERIFY(dialog->isEncryptionEnabled());

        if (isHeaderEncryptionAvailable) {
            QVERIFY(dialog->isHeaderEncryptionAvailable());
            // Header encryption is enabled by default, if available.
            QVERIFY(dialog->isHeaderEncryptionEnabled());

            encryptHeaderCheckBox->setChecked(false);
            QVERIFY(!dialog->isHeaderEncryptionEnabled());
        } else {
            QVERIFY(!dialog->isHeaderEncryptionAvailable());
        }

        collapsibleEncryption->setExpanded(false);
        QVERIFY(!dialog->isEncryptionEnabled());
        QVERIFY(!dialog->isHeaderEncryptionEnabled());
    } else {
        QVERIFY(!dialog->isEncryptionAvailable());
        QVERIFY(!dialog->isEncryptionEnabled());
        QVERIFY(!dialog->isHeaderEncryptionAvailable());
        QVERIFY(!dialog->isHeaderEncryptionEnabled());
    }
}

void CreateDialogTest::testHeaderEncryptionTooltip()
{
    if (!m_pluginManager.supportedWriteMimeTypes().contains(QLatin1String("application/zip"))) {
        QSKIP("zip format not available in CreateDialog, skipping test.", SkipSingle);
    }

    CreateDialog *dialog = new CreateDialog(nullptr, QString(), QUrl());

    auto collapsibleEncryption = dialog->findChild<KCollapsibleGroupBox*>(QStringLiteral("collapsibleEncryption"));
    auto encryptHeaderCheckBox = dialog->findChild<QCheckBox*>(QStringLiteral("encryptHeaderCheckBox"));
    QVERIFY(collapsibleEncryption);
    QVERIFY(encryptHeaderCheckBox);

    collapsibleEncryption->setExpanded(true);

    QVERIFY(dialog->setMimeType(QStringLiteral("application/zip")));
    QVERIFY(!encryptHeaderCheckBox->toolTip().isEmpty());

    // If we set a tar filter after the zip one, ensure that the old zip's tooltip is not shown anymore.
    QVERIFY(dialog->setMimeType(QStringLiteral("application/x-compressed-tar")));
    QVERIFY(encryptHeaderCheckBox->toolTip().isEmpty());
}

QTEST_MAIN(CreateDialogTest)

#include "createdialogtest.moc"
