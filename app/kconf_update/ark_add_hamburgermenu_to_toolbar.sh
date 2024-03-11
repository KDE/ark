#!/bin/sh

if [ -z "$XDG_DATA_HOME" ]; then
    XDG_DATA_HOME="$HOME/.local/share"
fi

sed --regexp-extended --null-data --in-place '
s/(<ToolBar.*)(<\/ToolBar>)/\1<Spacer\/>\n<Action name="hamburger_menu"\/>\n\2/
' \
${XDG_DATA_HOME}/kxmlgui5/ark/ark_part.rc
