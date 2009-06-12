/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef _QUERIES_H_
#define _QUERIES_H_

#include "kerfuffle_export.h"

#include <QString>
#include <QHash>
#include <QWaitCondition>
#include <QMutex>
#include <QVariant>

namespace Kerfuffle
{

typedef QHash<QString, QVariant> QueryData;

class KERFUFFLE_EXPORT Query
{

public:
    /**
     * Execute the response. Will happen in the GUI thread, so it's
     * safe to use widgets/gui elements here. Must call setResponse
     * when done.
     */
    virtual void execute() = 0;

    /**
     * Will block until the response have been set
     */
    void waitForResponse();

    QVariant response();

protected:
    /**
     * Protected constructor
     */
    Query();
    virtual ~Query() {}

    void setResponse(QVariant response);

    QueryData m_data;

private:
    QWaitCondition m_responseCondition;
    QMutex m_responseMutex;


};

class KERFUFFLE_EXPORT OverwriteQuery : public Query
{
public:
    explicit OverwriteQuery(QString filename);
    void execute();
    bool responseCancelled();
    bool responseOverwriteAll();
    bool responseOverwrite();
    bool responseRename();
    bool responseSkip();
    bool responseAutoSkip();
    QString newFilename();

    void setNoRenameMode(bool enableNoRenameMode);
    bool noRenameMode();
    void setMultiMode(bool enableMultiMode);
    bool multiMode();
private:
    bool m_noRenameMode;
    bool m_multiMode;
};

class KERFUFFLE_EXPORT PasswordNeededQuery : public Query
{
public:
    explicit PasswordNeededQuery(QString archiveFilename, bool incorrectTryAgain = false);
    void execute();

    bool responseCancelled();
    QString password();
};

}

#endif /* ifndef _QUERIES_H_ */
