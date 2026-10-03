What is this fork
=================
This is AA-Project's classic Demo "BB" with some fixes applied to allow it to build on modern Macs and run at absurdly high resolutions (2880x1800 text mode!).

How to build and run:

First clone/downlaod this repository
```
cd bb-osx
brew install libmikmod aalib
./configure; make; make install
bb
```

Works on both Intel and Apple Silicon (ARM64) Macs. The build requires
autoconf/automake/libtool-era tools only if regenerating; a checked-in
`configure` script is provided, so no autotools are needed to build.

Notes:
- `./configure` usually finds `libmikmod` automatically. If it does not,
  pass the path manually: `--with-libmikmod-prefix=$(brew --prefix libmikmod)`.
- The codebase is pre-C99; the build adds `-std=gnu89
  -Wno-implicit-function-declaration` automatically so no manual `CFLAGS`
  are required.

Smooth rendering (curses driver + frame sync)
=============================================

The stock aalib ships a curses driver that breaks on modern macOS
terminals: it read the private `stdscr->_maxx` / `_maxy` fields (hard-coded
an 80-column screen), dropped bold/dim/reverse, and emitted no frame
synchronization — so the demo tore and flickered.

This repo ships a fixed aalib under `aalib-patch/`:

- `aalib-curses-fix.patch` — `getmaxyx()` for size detection, `termattrs()`
  for bold/dim/reverse, and a per-frame **DEC private mode 2026
  "synchronized output"** wrapper (`\e[?2026h` … `\e[?2026l`) so each frame
  is presented atomically at the retrace instead of tearing. Terminals that
  don't implement 2026 ignore the escape, so it's safe everywhere.
- `build-aalib.sh` — applies the patch (plus the Homebrew aalib
  modernization patch) to a fresh `aalib-1.4.0` source tree, configures with
  the ncurses driver, and builds `libaa.a`.

Build the fixed aalib and link bb against it. The upstream tarball is
`aalib-1.4rc5.tar.gz` (it extracts to a directory named `aalib-1.4.0`); get
it from https://sourceforge.net/projects/aa-project/ (aa-lib 1.4rc5):
```
tar xzf aalib-1.4rc5.tar.gz        # -> aalib-1.4.0/
./aalib-patch/build-aalib.sh aalib-1.4.0
# link bb with:
#   -L aalib-1.4.0/src/.libs -laa -L $(brew --prefix ncurses)/lib -lncurses
```
Verified: 30 test frames produced exactly 30 balanced 2026 begin/end pairs
(one per frame), and the demo renders full-width with bold/dim/reverse and no
tearing on a color terminal.

**Required for interactive `bb 6`.** The stock Homebrew aalib is built
without the curses mouse driver, so `aa_autoinitmouse` finds no driver, the
terminal mouse mask is never armed, and `bb 6` has no cursor to drive — it
prints "Mouse driver unavailable" and falls back to the autopilot zoom.
`build-aalib.sh` configures aalib with `--with-curses-driver=yes`, which
enables `CURSES_MOUSEDRIVER` and the `curses` mouse driver; link `bb` against
that `libaa.a` to get cursor-driven zoom (button 1 in / button 2 out,
anchored at the cursor). Verified: the custom aalib reports `MOUSE_DRIVER_OK
curses` and arms the ncurses mouse mask; the stock lib reports
`NO_MOUSE_DRIVER`.

Standalone bonus scenes:
========================

The three XaoS fractal parts of act 2 can be run on their own (each
animates in real time, then exits; add `-loop` to repeat):

```
bb 6    # fractal zoom (mouse-driven; needs the fixed aalib, see above)
bb 7    # julia sequence
bb 9    # julia morph
```

What does this software do then ?
=================================

	BB is an high quality audio-visual demonstration for your text
terminal. It is portable demo, so you can run it on plenty of operating 
systems and DOS. 

Supports: dos (VGA + MDA), stdio, curses, slang, X11, gpm, linux-console
Sound:  OSS, Sun audio, SGI, DEC Aplha, AIX, HP-UX, DOS (GUS CLASSIC, GUS
        CLASSIC, GUS MAX, GUS ACE, GUS PNP, SB, ESS, PC-buzzer 

How this all started
====================

   Once upon a time we've (my friend Kamil and I) bought two old
Herculeses as secondary monitors. We didn't know for that time that our
Diamond Stealths 64 cards would become obsolete soon. The next day we
downloaded the logo of Linux Texas Users Group - nice silly penguin
looking like a cowboy! It was so exciting logo ... we decided that we
couldn't live without it and we wanted to see it at boot time as a logo
on our secondary monitors. There was a small problem - Hercules doesn't
support color graphics. So we decided to convert the penguin image to
ascii art using netpbm tools.

   The output was very ugly because the converting algorithm was
absolutly stupid.  During the night I designed a new convertor that
used a font bitmap to creat an aproximation table. The output wasn't
very good since the algorithm wasn't tuned so well. Many months this
small piece of code was waiting on my disc for the day "D". Meanwhile I
started a new project XaoS (a fractal zoomer) with my friend Thomas.
And then I got an idea: Ascii Art Mandelbrots!  I was really impressed
by the result! XaoS was faster, portable and looking much better than
ever before. I found a new way to go ...


AA-Project
==========

                               dT8  8Tb
                              dT 8  8 Tb
                             dT  8  8  Tb
                          <PROJECT><PROJECT>
                           dT    8  8    Tb
                          dT     8  8     Tb
   
Three goals of AA-Project:

  1. Port all important software (like Doom, Second Reality, X windows
     etc..) on AA-lib.

  2. Port AA-lib on all available platforms (mainly ZX-Spectrum and
     Sharp).

  3. Force IBM to start manufacturing MDA cards again.

   AA-project was started by Jan Hubicka. In that times just a few
people knew about it. Then a new demo named BB has been relased to show
the power of AA-lib technology. Now the project is freely available and
anyone can help.

Where to find BB
================

   All programs covered under AA-project can be obtained at
`http://aa-project.sourceforge.net'.

Support us:
===========

   We decided to distribute all of our aa-related programs freely (see
COPYING for details). If you want to help us more (so we could develop 
more such software) send a small amount (or big one ;-) to AA/BB team.
In that case, write us for further instructions...

kupsaf@feld.cvut.cz (Filip Kupsa)
hubicka@freesoft.cz (Jan Hubicka)
titania@horac.ta.jcu.cz (Mojmir Svoboda)
toman@atrax.karlin.mff.cuni.cz (Kamil Toman)
