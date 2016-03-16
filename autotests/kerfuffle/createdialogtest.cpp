/*
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "createdialog.h"
#include "mimetypes.h"

#include <QCheckBox>
#include <QMimeDatabase>
#include <QTest>

using namespace Kerfuffle;

class CreateDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testEncryption_data();
    void testEncryption();
    void testHeaderEncryptionTooltip();
};

void CreateDialogTest::testEncryption_data()
{
    QTest::addColumn<QString>("filter");
    QTest::addColumn<bool>("isEncryptionAvailable");
    QTest::addColumn<bool>("isHeaderEncryptionAvailable");

    QTest::newRow("tar") << QStringLiteral("application/x-compressed-tar") << false << false;

    if (Kerfuffle::supportedWriteMimeTypes().contains(QStringLiteral("application/zip"))) {
        QTest::newRow("zip") << QStringLiteral("application/zip") << true << false;
    } else {
        qDebug() << "zip format not available in CreateDialog, skipping test.";
    }

    if (Kerfuffle::supportedWriteMimeTypes().contains(QStringLiteral("application/x-7z-compressed"))) {
        QTest::newRow("7z") << QStringLiteral("application/x-7z-compressed") << true << true;
    } else {
        qDebug() << "7z format not available in CreateDialog, skipping test.";
    }

    if (Kerfuffle::supportedWriteMimeTypes().contains(QStringLiteral("application/x-rar"))) {
        QTest::newRow("rar") << QStringLiteral("application/x-rar") << true << true;
    } else {
        qDebug() << "rar format not available in CreateDialog, skipping test.";
    }
}

void CreateDialogTest::testEncryption()
{
    CreateDialog *dialog = new CreateDialog(Q_NULLPTR, QString(), QUrl());

    QFETCH(QString, filter);
    QFETCH(bool, isEncryptionAvailable);
    QFETCH(bool, isHeaderEncryptionAvailable);

    auto encryptCheckBox = dialog->findChild<QCheckBox*>(QStringLiteral("encryptCheckBox"));
    auto encryptHeaderCheckBox = dialog->findChild<QCheckBox*>(QStringLiteral("encryptHeaderCheckBox"));
    QVERIFY(encryptCheckBox);
    QVERIFY(encryptHeaderCheckBox);

    dialog->setCurrentFilterFromMimeType(filter);

    // Encryption is initially not enabled.
    QVERIFY(!dialog->isEncryptionEnabled());
    QVERIFY(!dialog->isHeaderEncryptionEnabled());

    if (isEncryptionAvailable) {
        QVERIFY(dialog->isEncryptionAvailable());

        encryptCheckBox->setChecked(true);
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

        encryptCheckBox->setChecked(false);
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
    if (!Kerfuffle::supportedWriteMimeTypes().contains(QStringLiteral("application/zip"))) {
        QSKIP("zip format not available in CreateDialog, skipping test.", SkipSingle);
    }

    CreateDialog *dialog = new CreateDialog(Q_NULLPTR, QString(), QUrl());

    auto encryptCheckBox = dialog->findChild<QCheckBox*>(QStringLiteral("encryptCheckBox"));
    auto encryptHeaderCheckBox = dialog->findChild<QCheckBox*>(QStringLiteral("encryptHeaderCheckBox"));
    QVERIFY(encryptCheckBox);
    QVERIFY(encryptHeaderCheckBox);

    encryptCheckBox->setChecked(true);

    dialog->setCurrentFilterFromMimeType(QStringLiteral("application/zip"));
    QVERIFY(!encryptHeaderCheckBox->toolTip().isEmpty());

    // If we set a tar filter after the zip one, ensure that the old zip's tooltip is not shown anymore.
    dialog->setCurrentFilterFromMimeType(QStringLiteral("application/x-compressed-tar"));
    QVERIFY(encryptHeaderCheckBox->toolTip().isEmpty());
}

QTEST_MAIN(CreateDialogTest)

#include "createdialogtest.moc"
