#!/bin/sh
# SPDX-FileCopyrightText: None
# SPDX-License-Identifier: CC0-1.0

glaxnimate_subdirs="data src"

mkdir -p "$podir"
$EXTRACTRC `find $glaxnimate_subdirs -name '*.rc' -o -name '*.ui' -o -name '*.kcfg'` >> rc.cpp
# find src/core/model -name '*.hpp' | xargs sed -nr 's/^\s*GLAXNIMATE_[^(]+\([^,]+, ([a-z0-9_]+).*/i18n("\1");/p' >> rc.cpp
find src/core/model -name '*.hpp' -exec sed -nr '{s~^\s*GLAXNIMATE_[^(]+\([^,]+, ([a-z0-9_]+).*~// i18n: file: {}\n// i18n: Property name\ni18n("\1");~p;N}' {} \;  >> rc.cpp
$XGETTEXT `find $glaxnimate_subdirs -name '*.cpp' -o -name '*.hpp'` rc.cpp -o "$podir/glaxnimate.pot"
rm -f rc.cpp
