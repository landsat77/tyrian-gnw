#!/bin/sh
# Build ty_render, the program tools/ty_pack.py runs to render the music
# (the headless harness, option -R: the game's own music player), for
# Linux, Windows and macOS, with "zig cc" as cross compiler:
#   pip install ziglang
#   tools/build_renderers.sh            -> tools/bin/ty_render-<os>-<arch>
# TARGETS="triple:name ..." selects the targets.
set -e
cd "$(dirname "$0")/.."
OUT=${OUT:-tools/bin}
mkdir -p "$OUT"
list() { sed -n "/^$1 =/,/^\$/p" src/ot/sources.mk | sed "s/^$1 = //; s/\\\\//g" | tr '\n' ' '; }
OT_FILES=$(list OT_FILES)
OT_RENDER_FILES=$(list OT_RENDER_FILES)
INC="-Isrc/port -Isrc/port/sdl -Isrc/ot"
DEFS="-DGNW -DNDEBUG"
ZIG="${ZIG:-python3 -m ziglang}"
TARGETS="${TARGETS:-x86_64-linux-musl:linux-x86_64 aarch64-linux-musl:linux-arm64 x86_64-windows-gnu:windows-x86_64.exe x86_64-macos:macos-x86_64 aarch64-macos:macos-arm64}"
for target in $TARGETS; do
    triple=${target%%:*}
    name=ty_render-${target#*:}
    tmp=$(mktemp -d)
    objs=""
    n=0
    # the game and the port code it calls: the game heap (gnw_ty_glue.h)
    for f in $OT_FILES $OT_RENDER_FILES; do
        n=$((n + 1))
        o="$tmp/ot$n.o"
        $ZIG cc -target "$triple" -O2 -std=gnu11 $DEFS -w $INC -include src/port/gnw_ty_glue.h \
            -c "src/ot/$f" -o "$o" &
        objs="$objs $o"
    done
    for f in ty_file.c ty_video.c ty_sdl.c ty_core.c ty_loudness.c ty_state.c; do
        o="$tmp/p_${f%.c}.o"
        $ZIG cc -target "$triple" -O2 -std=gnu11 $DEFS -w $INC -include src/port/gnw_ty_glue.h \
            -c "src/port/$f" -o "$o" &
        objs="$objs $o"
    done
    wait
    for f in src/port/ty_alloc.c src/port/ty_pack.c src/port/ty_music.c src/port/ty_render.c harness/ty_host.c; do
        o="$tmp/c_$(basename "$f" .c).o"
        $ZIG cc -target "$triple" -O2 -std=gnu11 $DEFS -w $INC -c "$f" -o "$o" &
        objs="$objs $o"
    done
    wait
    $ZIG cc -target "$triple" -O2 -s $objs -lm -o "$OUT/$name"
    rm -f "$OUT/${name%.exe}.pdb" "$OUT"/*.lib
    rm -rf "$tmp"
    echo "built $OUT/$name"
done
