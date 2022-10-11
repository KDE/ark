#!/bin/sh

sed --regexp-extended --null-data --in-place '
s/(<ToolBar.*)(<\/ToolBar>)/\1<Spacer\/>\n<Action name="hamburger_menu"\/>\n\2/
' \
`qtpaths --locate-file GenericDataLocation kxmlgui5/ark/ark_part.rc`
