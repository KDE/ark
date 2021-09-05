/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef PLUGIN_H
#define PLUGIN_H

#include "kerfuffle_export.h"

#include <QObject>

#include <KPluginMetaData>

namespace Kerfuffle
{

class KERFUFFLE_EXPORT Plugin : public QObject
{
    Q_OBJECT

    /**
     * The priority of the plugin. The higher the better.
     */
    Q_PROPERTY(int priority READ priority CONSTANT)

    /**
     * Whether the plugin has been enabled in the settings.
     */
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged MEMBER m_enabled)

    /**
     * Whether the plugin is read-write *at runtime*.
     * A plugin could be declared read-write at build-time but "downgraded" to read-only at runtime.
     */
    Q_PROPERTY(bool readWrite READ isReadWrite CONSTANT)

    /**
     * The list of executables required by the plugin to operate in read-only mode.
     */
    Q_PROPERTY(QStringList readOnlyExecutables READ readOnlyExecutables CONSTANT)

    /**
     * The list of executables required by the plugin to operate in read-write mode.
     * This is an empty list if the plugin is declared as read-only.
     */
    Q_PROPERTY(QStringList readWriteExecutables READ readWriteExecutables CONSTANT)

    /**
     * The plugin's JSON metadata. This provides easy access to the supported mimetypes list.
     */
    Q_PROPERTY(KPluginMetaData metaData READ metaData MEMBER m_metaData CONSTANT)

public:
    explicit Plugin(QObject *parent = nullptr, const KPluginMetaData& metaData = KPluginMetaData());


    int priority() const;
    bool isEnabled() const;
    void setEnabled(bool enabled);
    bool isReadWrite() const;
    QStringList readOnlyExecutables() const;
    QStringList readWriteExecutables() const;
    KPluginMetaData metaData() const;

    /**
     * @return Whether the executables required for a functional plugin are installed.
     * This is true if all the read-only executables are found in the path.
     */
    bool hasRequiredExecutables() const;

    /**
     * @return Whether the plugin is ready to be used.
     * This implies isEnabled(), while an enabled plugin may not be valid.
     * A read-write plugin downgraded to read-only is still valid.
     */
    bool isValid() const;

Q_SIGNALS:
    void enabledChanged();

private:

    /**
     * @return Whether all the given executables are found in $PATH.
     */
    static bool findExecutables(const QStringList &executables);

    bool m_enabled;
    KPluginMetaData m_metaData;
};

}

#endif
