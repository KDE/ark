/*
    SPDX-FileCopyrightText: 2026 Meven Car <meven@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "compressfileitemaction.h"

#include <KFileItem>
#include <KFileItemListProperties>

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QFile>
#include <QMenu>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QWidget>

class CompressFileItemActionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void compressToOutlivesThePlugin();

private:
    QAction *compressToAction(const QList<QAction *> &actions) const;
};

// The file manager owns this plugin and is free to delete it at any moment, including while the
// dialog that asks where to put the archive is open. What was asked for still happens and nothing
// reaches back into the deleted plugin. See bug 521633.
void CompressFileItemActionTest::compressToOutlivesThePlugin()
{
    QTemporaryDir workingDir;
    QVERIFY(workingDir.isValid());
    const QString filePath = workingDir.filePath(QStringLiteral("a file to compress.txt"));
    QFile file(filePath);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("some contents");
    file.close();

    QWidget parentWidget;
    parentWidget.show();
    QVERIFY(QTest::qWaitForWindowExposed(&parentWidget));

    auto *plugin = new CompressFileItemAction(nullptr, {});
    const KFileItemListProperties properties(KFileItemList{KFileItem(QUrl::fromLocalFile(filePath))});
    QAction *compressTo = compressToAction(plugin->actions(properties, &parentWidget));
    QVERIFY(compressTo);

    // The plugin goes away first, then the dialog is answered, so the code that runs after the
    // dialog is the code that used to reach into freed memory.
    QTimer::singleShot(100, [&plugin]() {
        delete plugin;
        plugin = nullptr;
    });
    QTimer::singleShot(600, []() {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        QVERIFY(dialog);
        dialog->accept();
    });

    compressTo->trigger();

    QVERIFY(!plugin);
    // The archive is written by a job of its own, which the plugin does not own.
    QTRY_VERIFY_WITH_TIMEOUT(!QDir(workingDir.path()).entryList({QStringLiteral("*.tar.gz")}, QDir::Files).isEmpty(), 10000);
}

QAction *CompressFileItemActionTest::compressToAction(const QList<QAction *> &actions) const
{
    if (actions.isEmpty() || !actions.constFirst()->menu()) {
        return nullptr;
    }

    // The entry that asks where to put the archive is the last one of the submenu.
    const QList<QAction *> compressActions = actions.constFirst()->menu()->actions();
    return compressActions.isEmpty() ? nullptr : compressActions.constLast();
}

QTEST_MAIN(CompressFileItemActionTest)

#include "compressfileitemactiontest.moc"
