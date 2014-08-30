#!/bin/bash
#
# Qt 5.0.1 has a broken build system that adds
# some invalid paths into the Makefile, so every time qmake is
# run, we have to fix the makefile.
#
# https://bugreports.qt-project.org/browse/QTBUG-28336
#

QT_ROOT="/Users/$USER/Qt5.0.1/5.0.1/clang_64"

fix_transform()
{
    sed \
        -e "s|-F${QT_ROOT}/qtdeclarative/lib||" \
        -e "s|-F${QT_ROOT}/qtbase/lib||" \
        -e "s|-F${QT_ROOT}/qtwebkit/lib||" \
        -e "s|-F${QT_ROOT}/qtjsbackend/lib||" < /dev/stdin
}

fix_build_dir()
{
    MAKEFILE_PATH="$1/Makefile"
    TEMPFILE="/tmp/QMD-fix"

    cat "$MAKEFILE_PATH" | fix_transform > "$TEMPFILE" && \
        mv "$TEMPFILE" "$MAKEFILE_PATH" && \
        rm -f "$TEMPFILE"
}

MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

for BUILD_PATH in $(ls "$MYDIR" | grep ^qarkdown-build); do
    echo "Fixing: $MYDIR/$BUILD_PATH"
    fix_build_dir "$MYDIR/$BUILD_PATH"
done
