#!/bin/sh
# Build bb against a patched, curses+mouse aalib — in one command.
#
# The stock Homebrew aalib is built without the curses mouse driver and its
# curses driver breaks on modern macOS terminals (private _maxx/_maxy
# access, hard-coded 80 columns, no frame sync). This script:
#   1. patches a pristine aalib-1.4.0 tree (Homebrew modernization patch +
#      this repo's aalib-curses-fix.patch: getmaxyx, termattrs, DEC 2026
#      synchronized output)
#   2. configures it with the ncurses curses + keyboard + mouse driver
#   3. turns the tree into a usable aalib "prefix" (include/ + lib/ symlinks
#      + aalib-config)
#   4. configures + builds bb against THAT aalib (via AALIB_CONFIG), so the
#      resulting ./bb has the working curses driver and the mouse driver.
#
# Usage: ./build-aalib.sh [srcdir]
#   srcdir = extracted aalib-1.4.0 tree (default: ./aalib-1.4.0 in the repo
#   root). bb's sources are assumed to be in the repo root (parent of this
#   script's directory).
set -e
SCRIPTDIR=$(cd "$(dirname "$0")" && pwd)
ROOT=$(cd "$SCRIPTDIR/.." && pwd)
AA="${1:-$ROOT/aalib-1.4.0}"
NC=$(brew --prefix ncurses)

[ -d "$AA" ] || { echo "error: aalib source tree not found: $AA" >&2
                 echo "extract aalib-1.4rc5.tar.gz (-> aalib-1.4.0/) first" >&2
                 exit 1; }
AA=$(cd "$AA" && pwd)

# 1. upstream/Homebrew modernization patch (opaque ncurses, etc.)
#    https://raw.githubusercontent.com/Homebrew/formula-patches/4cd6785/aalib/1.4rc5.patch
if [ ! -f "$AA/.hb-patched" ]; then
    curl -sL -o /tmp/aalib-hb.patch \
      https://raw.githubusercontent.com/Homebrew/formula-patches/4cd6785/aalib/1.4rc5.patch
    (cd "$AA" && patch -p1 --batch < /tmp/aalib-hb.patch && touch .hb-patched)
fi

# 2. this repo's fix: getmaxyx size detection + termattrs + 2026 frame sync
#    (absolute path: $SCRIPTDIR still resolves after the cd)
if [ ! -f "$AA/.fix-patched" ]; then
    (cd "$AA" && patch -p1 --batch < "$SCRIPTDIR/aalib-curses-fix.patch" && touch .fix-patched)
fi

# 3. configure with the ncurses curses driver (no X); the --prefix makes
#    aalib-config report this tree as the aalib prefix (used in step 5)
(cd "$AA" && CFLAGS="-std=gnu89 -O2" ./configure \
    --prefix="$AA" --with-ncurses="$NC" --with-curses-driver=yes --without-x)

# 4. build
(cd "$AA" && make)

# 5. make the tree a usable aalib "prefix": aalib-config (generated with
#    prefix=$AA) plus the include/ and lib/ paths it reports
mkdir -p "$AA/include"
[ -e "$AA/include/aalib.h" ] || ln -s ../src/aalib.h "$AA/include/aalib.h"
[ -e "$AA/lib" ] || ln -s src/.libs "$AA/lib"

# 6. build bb against the patched aalib (AALIB_CONFIG is honored by
#    bb's configure; -std=gnu89 for the pre-C99 sources)
[ -f "$ROOT/Makefile.am" ] || { echo "error: bb sources not found in $ROOT" >&2
                                exit 1; }
(cd "$ROOT" && rm -f config.cache && \
    AALIB_CONFIG="$AA/aalib-config" \
    CFLAGS="-std=gnu89 -Wno-implicit-function-declaration" \
    ./configure --with-libmikmod-prefix="$(brew --prefix libmikmod)")
# old automake does not re-link bb when the external libaa.a changes, so a
# previously built bb (linked against the stock aalib) would survive
# unchanged; force the relink
(cd "$ROOT" && make clean >/dev/null 2>&1; make)

# hard check: the bb binary itself must contain the curses driver and the
# mouse driver (catches a stale link against the stock aalib, which would
# otherwise silently fall back to the stdio driver)
if nm "$ROOT/bb" | grep -q _curses_d && nm "$ROOT/bb" | grep -q _mouse_curses_d; then
    ok="OK"
else
    echo "error: bb was not linked against the patched aalib (no curses/mouse" >&2
    echo "       driver in the binary) — rebuild failed to pick up $AA" >&2
    exit 1
fi
echo "done."
echo "  patched aalib: $AA (curses getmaxyx/termattrs/2026 + mouse driver)"
echo "  bb binary:     $ROOT/bb (curses + mouse driver: $ok)"
