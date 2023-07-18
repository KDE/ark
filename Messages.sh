#! /bin/sh
$EXTRACTRC $(find . -name '*.rc') >> rc.cpp || exit 11
$EXTRACTRC $(find . -name '*.ui') >> rc.cpp || exit 12
$EXTRACTRC $(find . -name '*.kcfg') >> rc.cpp
$XGETTEXT $(find app kerfuffle part plugins -name '*.cpp') rc.cpp -o $podir/ark.pot
rm -f rc.cpp
