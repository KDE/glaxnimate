#!/bin/sh
# SPDX-FileCopyrightText: None
# SPDX-License-Identifier: CC0-1.0

glaxnimate_subdirs="data src"

mkdir -p "$podir"
$EXTRACTRC `find $glaxnimate_subdirs -name '*.rc' -o -name '*.ui' -o -name '*.kcfg'` >> rc.cpp
$XGETTEXT `find $glaxnimate_subdirs -name '*.cpp' -o -name '*.hpp'` rc.cpp -o "$podir/glaxnimate.pot"
rm -f rc.cpp
