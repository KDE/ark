/*
    ark: A program for modifying archives via a GUI.

    SPDX-FileCopyrightText: 2004-2008 Henrique Pinto <henrique.pinto@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef ARKVIEWER_H
#define ARKVIEWER_H

#include "ui_arkviewer.h"

#include <KParts/MainWindow>
#include <KParts/ReadOnlyPart>
#include <KService>

#include <QMimeType>
#include <QPointer>

#include <optional>

class ArkViewer : public KParts::MainWindow, public Ui::ArkViewer
{
    Q_OBJECT

public:
    ~ArkViewer() override;

    static void view(const QString& fileName, const QString& entryPath = QString());

private:
    explicit ArkViewer();

    static KService::Ptr getExternalViewer(const QString& mimeType);
    static std::optional<KPluginMetaData> getInternalViewer(const QString& mimeType);

    static void openExternalViewer(const KService::Ptr viewer, const QString& fileName);

    static void openInternalViewer(const KPluginMetaData& viewer, const QString& fileName, const QString& entryPath, const QMimeType& mimeType);

    static bool askViewAsPlainText(const QMimeType& mimeType);

    bool viewInInternalViewer(const KPluginMetaData& viewer, const QString& fileName, const QString& entryPath, const QMimeType& mimeType);

private Q_SLOTS:
    void aboutKPart();

private:
    QPointer<KParts::ReadOnlyPart> m_part;
    QString m_fileName;
};

#endif // ARKVIEWER_H

