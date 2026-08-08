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
#include <memory>

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

    /**
     * Answers the question with a no and lets whoever waits for it carry on.
     *
     * Whatever the dialog answers afterwards is dropped, so a question can be given up on
     * while it is still on the screen. Can be called from any thread.
     */
    void abort();

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

    /**
     * The answer that stands for a no for this question, given when it is given up on.
     */
    virtual QVariant cancelledResponse() const = 0;

    QVariant data(const QString &key) const;
    void setData(const QString &key, const QVariant &value);

private:
    QueryData m_data;
    bool m_aborted = false;
    QWaitCondition m_responseCondition;
    mutable QMutex m_responseMutex;
};

/* *****************************************************************
 * Used to query the user if an existing file should be overwritten.
 * *****************************************************************
 */
class KERFUFFLE_EXPORT OverwriteQuery : public Query
{
public:
    QVariant cancelledResponse() const override;
    explicit OverwriteQuery(const QString &filename);
    void execute() override;
    bool responseCancelled();
    bool responseOverwriteAll();
    bool responseOverwrite();
    bool responseRename();
    bool responseSkip();
    bool responseAutoSkip();
    QString newFilename();

    void setArchiveFileName(const QString &fileName);
    void setArchiveMimeType(const QString &mimeType);
    void setDestination(const QString &destination);

    void setNoRenameMode(bool enableNoRenameMode);
    bool noRenameMode();
    void setMultiMode(bool enableMultiMode);
    bool multiMode();

private:
    QString m_archiveFileName;
    QString m_archiveMimeType;
    QString m_destination;
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
    QVariant cancelledResponse() const override;
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
    QVariant cancelledResponse() const override;
    explicit LoadCorruptQuery(const QString &archiveFilename);
    void execute() override;

    bool responseYes();
};

class KERFUFFLE_EXPORT ContinueExtractionQuery : public Query
{
public:
    QVariant cancelledResponse() const override;
    explicit ContinueExtractionQuery(const QString &error, const QString &archiveEntry);
    void execute() override;

    bool responseCancelled();
    bool dontAskAgain();

private:
    QCheckBox m_chkDontAskAgain;
};

}

Q_DECLARE_METATYPE(std::shared_ptr<Kerfuffle::Query>)

#endif /* ifndef QUERIES_H */
