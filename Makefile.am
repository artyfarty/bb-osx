# This is pre-C99 code: modern clang/gcc reject implicit int, implicit
# function declarations and some K&R idioms this codebase relies on.
# These flags keep the build working on Apple Silicon and current x86
# toolchains without requiring a hand-supplied CFLAGS. They are appended
# after @CFLAGS@ so they take precedence over any -std the user passes.
CFLAGS=@CFLAGS@ -std=gnu89 -Wno-implicit-function-declaration -D SOUNDDIR=\"$(pkgdatadir)\"
bin_PROGRAMS = bb
bb_SOURCES = autopilo.c backconv.c bb.c credits.c credits2.c ctrl87.c fk1.c fk2.c fk3.c fk4.c formulas.c hh1.c hh2.c hh3.c hh4.c image.c julia.c kt1.c kt2.c kt3.c kt4.c main.c messager.c minilzo.c ms1.c ms2.c ms3.c ms4.c print.c scene1.c scene2.c scene3.c scene4.c scene5.c scene7.c scene8.c scene9.c tex.c textform.c timers.c uncompfn.c zeb.c zoom.c
pkgdata_DATA = @SOUNDFILES@
EXTRA_DIST = README.LZO bb.s3m bb2.s3m bb3.s3m acconfig.h bb.lsm autopilo.h bb.h complex.h config.h ctrl87.h formulas.h image.h minilzo.h mylzo.h patnik.h tex.h timers.h torus.h ui.h zoom.h bb.1 m4/libmikmod.m4
man_MANS = bb.1
