#! /bin/sh
$EXTRACTRC $(find . -name '*.rc') >> rc.cpp || exit 11
$XGETTEXT app/*.cpp kerfuffle/*.cpp part/*.cpp plugins/*/*.cpp rc.cpp -o $podir/ark.pot
rm -f rc.cpp
