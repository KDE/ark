/*
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef QUERIES_H
#define QUERIES_H

#include "kerfuffle_export.h"

#include <QCheckBox>
#include <QHash>
#include <QMutex>
#include <QString>
#include <QVariant>
#include <QWaitCondition>

namespace Kerfuffle
{
typedef QHash<QString, QVariant> QueryData;

class KERFUFFLE_EXPORT Query
{
public:
    /**
     * Execute the response. It needs to be called from the GUI thread.
     */
    virtual void execute() = 0;

    /**
     * Will block until the response have been set.
     * Useful for worker threads that need to show a dialog.
     */
    void waitForResponse();

    QVariant response() const;

protected:
    /**
     * Protected constructor
     */
    Query();
    virtual ~Query()
    {
    }

    void setResponse(const QVariant &response);

    QueryData m_data;

private:
    QWaitCondition m_responseCondition;
    QMutex m_responseMutex;
};

/* *****************************************************************
 * Used to query the user if an existing file should be overwritten.
 * *****************************************************************
 */
class KERFUFFLE_EXPORT OverwriteQuery : public Query
{
public:
    explicit OverwriteQuery(const QString &filename);
    void execute() override;
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

/* **************************************
 * Used to query the user for a password.
 * **************************************
 */
class KERFUFFLE_EXPORT PasswordNeededQuery : public Query
{
public:
    explicit PasswordNeededQuery(const QString &archiveFilename, bool incorrectTryAgain = false);
    void execute() override;

    bool responseCancelled();
    QString password();
};

/* *************************************************************
 * Used to query the user if a corrupt archive should be loaded.
 * *************************************************************
 */
class KERFUFFLE_EXPORT LoadCorruptQuery : public Query
{
public:
    explicit LoadCorruptQuery(const QString &archiveFilename);
    void execute() override;

    bool responseYes();
};

class KERFUFFLE_EXPORT ContinueExtractionQuery : public Query
{
public:
    explicit ContinueExtractionQuery(const QString &error, const QString &archiveEntry);
    void execute() override;

    bool responseCancelled();
    bool dontAskAgain();

private:
    QCheckBox m_chkDontAskAgain;
};

}

Q_DECLARE_METATYPE(Kerfuffle::Query *)

#endif /* ifndef QUERIES_H */
