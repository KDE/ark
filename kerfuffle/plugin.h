/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
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
