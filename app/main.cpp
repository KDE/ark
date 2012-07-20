/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#include "mainwindow.h"
#include "batchextract.h"
#include "kerfuffle/addtoarchive.h"

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include <KLocale>

#include <QByteArray>
#include <QFileInfo>

using Kerfuffle::AddToArchive;

int main(int argc, char **argv)
{
    KAboutData aboutData("ark", 0, ki18n("Ark"),
                         "2.19", ki18n("KDE Archiving tool"),
                         KAboutData::License_GPL,
                         ki18n("(c) 1997-2011, The Various Ark Developers"),
                         KLocalizedString(),
                         "http://utils.kde.org/projects/ark"
                        );

    aboutData.addAuthor(ki18n("Raphael Kubo da Costa"),
                        ki18n("Maintainer"),
                        "rakuco@FreeBSD.org");
    aboutData.addAuthor(ki18n("Harald Hvaal"),
                        ki18n("Former Maintainer"),
                        "haraldhv@stud.ntnu.no");
    aboutData.addAuthor(ki18n("Henrique Pinto"),
                        ki18n("Former Maintainer"),
                        "henrique.pinto@kdemail.net");
    aboutData.addAuthor(ki18n("Helio Chissini de Castro"),
                        ki18n("Former maintainer"),
                        "helio@kde.org");
    aboutData.addAuthor(ki18n("Georg Robbers"),
                        KLocalizedString(),
                        "Georg.Robbers@urz.uni-hd.de");
    aboutData.addAuthor(ki18n("Roberto Selbach Teixeira"),
                        KLocalizedString(),
                        "maragato@kde.org");
    aboutData.addAuthor(ki18n("Francois-Xavier Duranceau"),
                        KLocalizedString(),
                        "duranceau@kde.org");
    aboutData.addAuthor(ki18n("Emily Ezust (Corel Corporation)"),
                        KLocalizedString(),
                        "emilye@corel.com");
    aboutData.addAuthor(ki18n("Michael Jarrett (Corel Corporation)"),
                        KLocalizedString(),
                        "michaelj@corel.com");
    aboutData.addAuthor(ki18n("Robert Palmbos"),
                        KLocalizedString(),
                        "palm9744@kettering.edu");

    aboutData.addCredit(ki18n("Bryce Corkins"),
                        ki18n("Icons"),
                        "dbryce@attglobal.net");
    aboutData.addCredit(ki18n("Liam Smit"),
                        ki18n("Ideas, help with the icons"),
                        "smitty@absamail.co.za");
    aboutData.addCredit(ki18n("Andrew Smith"),
                        ki18n("bkisofs code"),
                        QByteArray(),
                        "http://littlesvr.ca/misc/contactandrew.php");
    aboutData.setProgramIconName(QLatin1String("ark"));

    KCmdLineArgs::init(argc, argv, &aboutData);
    KCmdLineOptions option;
    option.add("+[url]", ki18n("URL of an archive to be opened"));
    option.add("d").add("dialog", ki18n("Show a dialog for specifying the options for the operation (extract/add)"));
    option.add("o").add("destination <directory>", ki18n("Destination folder to extract to. Defaults to current path if not specified."));
    option.add(":", ki18n("Options for adding files"));
    option.add("c").add("add", ki18n("Query the user for an archive filename and add specified files to it. Quit when finished."));
    option.add("t").add("add-to <filename>", ki18n("Add the specified files to 'filename'. Create archive if it does not exist. Quit when finished."));
    option.add("p").add("changetofirstpath", ki18n("Change the current dir to the first entry and add all other entries relative to this one."));
    option.add("f").add("autofilename <suffix>", ki18n("Automatically choose a filename, with the selected suffix (for example rar, tar.gz, zip or any other supported types)"));
    option.add(":", ki18n("Options for batch extraction:"));
    option.add("b").add("batch", ki18n("Use the batch interface instead of the usual dialog. This option is implied if more than one url is specified."));
    option.add("e").add("autodestination", ki18n("The destination argument will be set to the path of the first file supplied."));
    option.add("a").add("autosubfolder", ki18n("Archive contents will be read, and if detected to not be a single folder archive, a subfolder with the name of the archive will be created."));
    KCmdLineArgs::addCmdLineOptions(option);
    KCmdLineArgs::addTempFileOption();

    KApplication application;
    application.setQuitOnLastWindowClosed(false);

    //session restoring
    if (application.isSessionRestored()) {
        MainWindow* window = NULL;

        if (KMainWindow::canBeRestored(1)) {
            window = new MainWindow;
            window->restore(1);
            if (!window->loadPart()) {
                delete window;
                window = NULL;
            }
        }

        if (window == NULL) {
            return -1;
        }
    } else { //new ark window (no restored session)
        // open any given URLs
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

        if (args->isSet("add") || args->isSet("add-to")) {
            AddToArchive *addToArchiveJob = new AddToArchive;
            application.connect(addToArchiveJob, SIGNAL(result(KJob*)), SLOT(quit()), Qt::QueuedConnection);

            if (args->isSet("changetofirstpath")) {
                addToArchiveJob->setChangeToFirstPath(true);
            }

            if (args->isSet("add-to")) {
                addToArchiveJob->setFilename(args->getOption("add-to"));
            }

            if (args->isSet("autofilename")) {
                addToArchiveJob->setAutoFilenameSuffix(args->getOption("autofilename"));
            }

            for (int i = 0; i < args->count(); ++i) {
                //TODO: use the returned value here?
                addToArchiveJob->addInput(args->url(i));
            }

            if (args->isSet("dialog")) {
                if (!addToArchiveJob->showAddDialog()) {
                    return 0;
                }
            }

            addToArchiveJob->start();
        } else if (args->isSet("batch")) {
            BatchExtract *batchJob = new BatchExtract;
            application.connect(batchJob, SIGNAL(result(KJob*)), SLOT(quit()), Qt::QueuedConnection);

            for (int i = 0; i < args->count(); ++i) {
                batchJob->addInput(args->url(i));
            }

            if (args->isSet("autosubfolder")) {
                kDebug() << "Setting autosubfolder";
                batchJob->setAutoSubfolder(true);
            }

            if (args->isSet("autodestination")) {
                QString autopath = QFileInfo(args->url(0).path()).path();
                kDebug() << "By autodestination, setting path to " << autopath;
                batchJob->setDestinationFolder(autopath);
            }

            if (args->isSet("destination")) {
                kDebug() << "Setting destination to " << args->getOption("destination");
                batchJob->setDestinationFolder(args->getOption("destination"));
            }

            if (args->isSet("dialog")) {
                if (!batchJob->showExtractDialog()) {
                    return 0;
                }
            }

            batchJob->start();
        } else {
            MainWindow *window = new MainWindow;
            if (!window->loadPart()) { // if loading the part fails
                return -1;
            }

            if (args->count()) {
                kDebug() << "trying to open" << args->url(0);

                if (args->isSet("dialog")) {
                    window->setShowExtractDialog(true);
                }
                window->openUrl(args->url(0));
            }
            window->show();
        }
    }

    kDebug() << "Entering application loop";
    return application.exec();
}
