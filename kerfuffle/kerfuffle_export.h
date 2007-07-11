/***************************************************************************
 *   Copyright (C) 2006      by Pino Toscano <toscano.pino@tiscali.it>     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef KERFUFFLE_EXPORT_H
#define KERFUFFLE_EXPORT_H

/* needed for KDE_EXPORT macros */
#include <kdemacros.h>


#if defined _WIN32 || defined _WIN64
#ifndef KERFUFFLE_EXPORT
# ifdef MAKE_KERFUFFLE_LIB
#  define KERFUFFLE_EXPORT KDE_EXPORT
# else
#  define KERFUFFLE_EXPORT KDE_IMPORT
# endif
#endif

#else /* UNIX*/


/* export statements for unix */
#define KERFUFFLE_EXPORT KDE_EXPORT
#endif

#endif
