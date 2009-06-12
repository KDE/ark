/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include "archiveinterface.h"
#include "observer.h"
#include <kdebug.h>
#include <kfileitem.h>

#include <QFileInfo>
#include <QDir>

namespace Kerfuffle
{
ReadOnlyArchiveInterface::ReadOnlyArchiveInterface(const QString & filename, QObject *parent)
        : QObject(parent), m_filename(filename), m_waitForFinishedSignal(false)
{
}

ReadOnlyArchiveInterface::~ReadOnlyArchiveInterface()
{
}

const QString& ReadOnlyArchiveInterface::filename() const
{
    return m_filename;
}

bool ReadOnlyArchiveInterface::isReadOnly() const
{
    return true;
}

bool ReadOnlyArchiveInterface::open()
{
    return true;
}

void ReadOnlyArchiveInterface::setPassword(QString password)
{
    m_password = password;
}

const QString& ReadOnlyArchiveInterface::password() const
{
    return m_password;
}

void ReadOnlyArchiveInterface::error(const QString & message, const QString & details)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onError(message, details);
    }
}

void ReadOnlyArchiveInterface::entry(const ArchiveEntry & archiveEntry)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onEntry(archiveEntry);
    }
}

void ReadOnlyArchiveInterface::entryRemoved(const QString & path)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onEntryRemoved(path);
    }
}

void ReadOnlyArchiveInterface::progress(double p)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onProgress(p);
    }
}

void ReadOnlyArchiveInterface::info(const QString& info)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onInfo(info);
    }
}

void ReadOnlyArchiveInterface::finished(bool result)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onFinished(result);
    }
}

bool ReadOnlyArchiveInterface::doKill()
{
    //default implementation
    return false;
}

bool ReadOnlyArchiveInterface::doSuspend()
{
    //default implementation
    return false;
}

bool ReadOnlyArchiveInterface::doResume()
{
    //default implementation
    return false;
}

void ReadOnlyArchiveInterface::userQuery(Query *query)
{
    foreach(ArchiveObserver *observer, m_observers) {
        observer->onUserQuery(query);
    }
}

void ReadOnlyArchiveInterface::registerObserver(ArchiveObserver *observer)
{
    m_observers.append(observer);
}

void ReadOnlyArchiveInterface::removeObserver(ArchiveObserver *observer)
{
    m_observers.removeAll(observer);
}

ReadWriteArchiveInterface::ReadWriteArchiveInterface(const QString & filename, QObject *parent)
        : ReadOnlyArchiveInterface(filename, parent)
{
}

ReadWriteArchiveInterface::~ReadWriteArchiveInterface()
{
}

void ReadOnlyArchiveInterface::setWaitForFinishedSignal(bool value)
{
    m_waitForFinishedSignal = value;
}

bool ReadWriteArchiveInterface::isReadOnly() const
{
    QFileInfo fileInfo(filename());
    if (fileInfo.exists()) {
        return ! fileInfo.isWritable();
    } else {
        return !fileInfo.dir().exists(); // TODO: Should also check if we can create a file in that directory
    }
}

} // namespace Kerfuffle

#include "archiveinterface.moc"
