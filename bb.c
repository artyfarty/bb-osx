/*
 * BB: The portable demo
 *
 * (C) 1997 by AA-group (e-mail: aa@horac.ta.jcu.cz)
 *
 * 3rd August 1997
 * version: 1.2 [final3]
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public Licences as by published
 * by the Free Software Foundation; either version 2; or (at your option)
 * any later version
 *
 * This program is distributed in the hope that it will entertaining,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILTY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Publis License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <time.h> 
#include <ctype.h>
#include <aalib.h>
#include "bb.h"

/* aalib's internal per-glyph ink table (see aalib src/aaint.h); the header is
 * not shipped by the installed build, but the layout is part of the library's
 * interface (p[4] is the glyph ink, used by the filltable builder). */
struct parameters {
    unsigned int p[5];
};

static int stage = 1;
/* aalib builds its tables lazily in aa_render, so force the build here to patch
 * the filltable before the first render. */
extern unsigned short *aa_mktable(aa_context *);
/* The stock aalib filltable is a per-intensity argmin-ink-distance pick: it
 * reaches the correct *average* ink but with uneven band widths (measured on
 * the real table: '|' owns ~19 of the 256 intensities while 'U' owns just 1).
 * Sweeping a smooth gradient through it therefore changes glyph at clustered,
 * uneven intervals, which reads as banding / spurious extra characters in
 * smooth regions. This remaps the 256 intensities onto the *same* glyph set
 * but evenly, so every glyph owns 256/n intensities and the ramp is a clean
 * monotonic gradient. Glyphs are ordered by ink (not by first appearance in
 * the stock table) so the ramp is monotonic in ink by construction; for the
 * fonts/masks bb uses (normal+bold+dim, and the -eight block set) aalib's
 * stock walk is already ink-ordered, so the sort is currently a no-op but
 * keeps the invariant robust. The uneven bands are the real target: no-dither
 * (AA_NONE) has no error diffusion and renders them raw, while
 * Floyd-Steinberg and error-distribution diffuse the quantization error to
 * neighbours and hide them. Runs on whatever filltable aalib has built. */
static void bb_even_filltable(aa_context *ctx)
{
    unsigned short *ft = ctx->filltable;
    int seen[256];
    int g[256];
    unsigned int ink[256];
    int n = 0, i, j, v;
    unsigned short *new;
    if (ft == NULL)
	return;
    memset(seen, 0, sizeof(seen));
    for (v = 0; v < 256; v++) {
	unsigned short g0 = ft[v] & 0xff;
	if (!seen[g0]) {
	    seen[g0] = 1;
	    g[n] = g0;
	    ink[n] = ctx->parameters[g0].p[4];
	    n++;
	}
    }
    if (n < 2)
	return;
    for (i = 1; i < n; i++)
	for (j = i - 1; j >= 0 && ink[j] > ink[i]; j--) {
	    unsigned int ti = ink[j]; ink[j] = ink[j+1]; ink[j+1] = ti;
	    int tg = g[j]; g[j] = g[j+1]; g[j+1] = tg;
	}
    new = (unsigned short *) malloc(256 * sizeof(*new));
    if (new == NULL)
	return;
    for (v = 0; v < 256; v++)
	new[v] = g[(v * (n - 1)) / 255];
    memcpy(ft, new, 256 * sizeof(*new));
    free(new);
}

int finish_stuff, starttime, endtime;
int dual = 0;
static int quitnow = 0;
int loopmode;
aa_context *context;
aa_renderparams *params;
int TIME;
tl_timer *scenetimer;
struct font *font;

double getwidth(double size)
{
    double height = aa_imgheight(context) / size;
    double width = height * (double) aa_imgwidth(context) * 0.75 / aa_imgheight(context) * aa_mmheight(context) / aa_mmwidth(context);
    return (width);
}

void centerprint(int x, int y, double size, int color, char *text, int mode)
{
    if (!dual || !mode) {
	double height = aa_imgheight(context) / size;
	double width = height * (double) aa_imgwidth(context) * 0.75 / aa_imgheight(context) * aa_mmheight(context) / aa_mmwidth(context);
	print(x - (width * strlen(text)) / 2, y - height / 2, width, height, font, color, text);
    }
    else {
	if (mode & 1) {
	    double height = aa_imgheight(context) / size;
	    double width = height * (double) aa_imgwidth(context) * 0.75 / aa_imgheight(context) * aa_mmheight(context) / aa_mmwidth(context);
	    print(x / 2 - (width * strlen(text)) / 2, y - height / 2, width, height, font, color, text);
	}
	if (mode & 2) {
	    double height = aa_imgheight(context) / size;
	    double width = height * (double) aa_imgwidth(context) * 0.75 / aa_imgheight(context) * aa_mmheight(context) / aa_mmwidth(context);
	    print(aa_imgwidth(context) / 2 + x / 2 - (width * strlen(text)) / 2, y - height / 2, width, height, font, color, text);
	}
    }
}

void centerprinth(int x, int y, double size, int color, char *text, int mode)
{
    if (!mode || !dual) {
	double width = aa_imgwidth(context) / size;
	double height = width * (double) aa_imgheight(context) * 1.333 / aa_imgwidth(context) * aa_mmwidth(context) / aa_mmheight(context);
	print(x - (width * strlen(text)) / 2, y - height / 2, width, height, font, color, text);
    }
    else {
	if (mode & 1) {
	    double width = aa_imgwidth(context) / size / 2;
	    double height = width * (double) aa_imgheight(context) * 1.333 / aa_imgwidth(context) * aa_mmwidth(context) / aa_mmheight(context);
	    print(x / 2 - (width * strlen(text)) / 2, y - height / 2, width, height, font, color, text);
	}
	if (mode & 1) {
	    double width = aa_imgwidth(context) / size / 2;
	    double height = width * (double) aa_imgheight(context) * 1.333 / aa_imgwidth(context) * aa_mmwidth(context) / aa_mmheight(context);
	    print(aa_imgwidth(context) / 2 + x / 2 - (width * strlen(text)) / 2, y - height / 2, width, height, font, color, text);
	}
    }
}

static void (*control1) (int);
static int called = 0;

static void mycontrol(void *data, int i)
{
    called = 1;
    if (control1 != NULL)
	control1(i);
}
static void mycontrol2(void *data, int i)
{
   ((void (*) (int))data)(i);
}

int bbupdate()
{
    int ch;
    tl_update_time();
    TIME = tl_lookup_timer(scenetimer);
    tl_process_group (syncgroup, NULL);
    ch = aa_getkey(context, 0);
    switch (ch) {
    case 's':
    case 'S':
    case AA_BACKSPACE:
	finish_stuff = 1;
	break;
    case AA_ESC:
    case 'q':
	finish_stuff = 1, quitnow = 1;
    }
    return (ch);
}

void timestuff(int rate, void (*control) (int), void (*draw) (void), int maxtime)
{
    int waitmode = 0, t;
    tl_timer *timer;
    bbupdate();
    /*starttime = TIME; */
    endtime = starttime + maxtime;
    timer = tl_create_timer();
    if (control == NULL) {
	rate = -40;
    }
    if (rate < 0) {
	waitmode = 1, rate = -rate;
	control1 = control;
	tl_set_multihandler(timer, mycontrol, NULL);
    }
    else
	tl_set_multihandler(timer, mycontrol2, control);
    tl_set_interval(timer, 1000000 / rate);
    tl_add_timer(syncgroup, timer);
    tl_reset_timer(timer);
    tl_slowdown_timer (timer, starttime - TIME);
    if (control != NULL)
	control(1);
    while (!finish_stuff && TIME < endtime) {
	called = 0;
	bbupdate();
	t = tl_process_group(syncgroup, NULL);
	if (TIME > endtime)
	    break;
	if (!called && waitmode)
	    tl_sleep(t);
	else {
	    if (draw != NULL)
		draw();
	}
    }
    starttime = endtime;
    tl_free_timer(timer);
}

void
bbwait (int maxtime)
{
  int wait;
  if (finish_stuff)
    return;
  bbupdate ();
  endtime = starttime + maxtime;

  wait = endtime - TIME;
  while (wait > 0)
    {
      int t;
      bbupdate ();
      t = tl_process_group (syncgroup, NULL);
      wait = endtime - TIME;
      if (wait < t)
	t = wait;
      tl_sleep (t);
    }
  starttime = endtime;
}

void bbflushwait(int maxtime)
{
    int wait;
    if (finish_stuff)
	return;
    bbupdate();
    wait = maxtime + starttime - TIME;
    if (wait > 0) {
	aa_flush(context);
    }
    bbwait(maxtime);
}



int bbinit(int argc, char **argv)
{
    int i;
    aa_defparams.supported|= AA_NORMAL_MASK | AA_BOLD_MASK | AA_DIM_MASK;
    aa_parseoptions(NULL, NULL, &argc, argv);
    /* After aalib has consumed its own options, the remaining tokens are the
     * bb-specific ones: "-loop" (loop the demo) and/or a stage number.
     * 1-3 start an act (1 = full show; 4-8 fall through to the full show, as
     * in the original), and 6/7/9 are hidden standalone bonus scenes. They
     * may be given in any order and combination, e.g. "bb -loop 9". */
    for (i = 1; i < argc; i++) {
	char *a = argv[i];
	if (!strcmp(a, "-loop")) {
	    loopmode = 1;
	} else if (a[0] >= '1' && a[0] <= '9' && !a[1]) {
	    stage = atoi(a);
	} else {
	    printf("Usage: bb [aaoptions] [-loop] [number]\n\n");
	    printf("Options:\n"
		   "  -loop          play demo in infinite loop\n"
		   "  1, 2, 3        start at act 1, 2 or 3 (1 = full show)\n"
		   "  6              hidden bonus scene (XaoS fractal zoom, mouse-driven)\n"
		   "  7              hidden bonus scene (XaoS julia sequence)\n"
		   "  9              hidden bonus scene (XaoS julia morph)\n"
		   "AAlib options:\n%s\n", aa_help);
	    exit(1);
	}
    }
    context = aa_autoinit(&aa_defparams);
    if (!context) {
	printf("Failed to initialize aalib\n");
	exit(2);
    }
    if (!aa_autoinitkbd(context, 0)) {
	aa_close(context);
	printf("Failed to initialize keyboard\n");
	exit(3);
    }
    aa_hidecursor(context);
    return 1;
}

int bb(void)
{
    aa_gotoxy(context, 0, 0);
    introscreen();
    params = aa_getrenderparams();
    aa_render(context, params, 0, 0, 1, 1);
    font = uncompressfont( /*context->params.font */ &aa_font16);
    scenetimer = tl_create_timer();
    srand(time(NULL));
    if (stage != 1)
	finish_stuff = 1;
    do
	switch (stage) {
	case 9:
	case 7:
	case 6:
	    /* Standalone bonus scene: one of the three XaoS fractal parts of
	     * act 2, normally only a brief segment of the full show.
	     *   6 = fractal zoom (mouse interactive)
	     *   7 = julia sequence
	     *   9 = julia morph
	     * Stood alone it animates in real time, then exits (or repeats
	     * with -loop). These cases sit before "default:" and each break,
	     * so the act chain (1 -> 2 -> 3) can never fall through into them. */
	    finish_stuff = 0;	/* animate (the "stage != 1" fast-forward above set it) */
	    starttime = endtime = TIME;	/* real-time clock origin for this scene */
	    aa_resize (context);
	    if (stage == 6) {
		/* User-driven fractal explorer: turn autopilot off and use the
		 * aalib mouse driver. Button 1 (left) zooms in, button 2 (middle)
		 * zooms out; both are anchored at the cursor position. q / s / ESC to quit. */
		if (aa_autoinitmouse(context, 0)) {
		    aa_showcursor(context);
		    scene6(1);
		    aa_hidecursor(context);
		    aa_uninitmouse(context);
		} else {
		    printf("Mouse driver unavailable, falling back to autopilot.\n");
		    scene6(0);
		}
	    } else if (stage == 7)
		scene7();
	    else
		scene9();
	    aa_resize (context);
	    if (quitnow)
		goto quit;
	    break;
	default:
	case 1:
	    load_song("bb.s3m");
	    bbupdate();
	    starttime = endtime = TIME;

	    aa_resize (context);
	    scene1();
	    aa_resize (context);
	    scene3();
	    if (quitnow)
		goto quit;
	    aa_resize (context);
	    vezen(&fk1, &fk2, &fk3, &fk4);
	    messager("FILIP KUPSA known as FK, Tingle Notions, Dawn Music\n"
		"birth: June 22 1979, Tabor, Czech Republic, sex: male\n"
		     "\n"
	     "1992 - Changed his piano for 386/mp.com/pc-speaker music\n"
		     "1993 - Got his first Sound Blaster\n"
		     "1995 - Changed his SB for a new GUS technology\n"
		     "1996 - Composed his first great hits\n"
		     "1996 - FAT recomposition made by Windows 95\n"
		     "1997 - Released his musac in BB\n"
		     "\n"
		     "1998 - Got retired\n"
		     "\n"
		     "Contact address: via KT");
	    devezen2();
	    aa_resize (context);
	    scene4();
	    aa_resize (context);
	    scene2();
	    aa_resize (context);
	    if (quitnow)
		goto quit;
	    vezen(&ms1, &ms2, &ms3, &ms4);
	    messager("MOJMIR SVOBODA known as MS, TiTania, MSS, Bill\n"
		     "birth: ??, Tabor, Czech Republic, sex: ? male ?\n"
		     "\n"
		     "1993 - Installed Linux on his 386sx/25 + 40MB HDD\n"
		     "1994 - Removed Linux to make space for Doom\n"
		   "1995 - Reinstalled Linux on his 486Dx4/120 + 850MB\n"
		     "1996 - Removed Linux to make space for Windows 95\n"
		     "\n"
		     "1997 - Removed Windows 95 to make space for aalib\n"
		     "\n"
		     "Contact address: titania@mbox.vol.cz");
	    devezen3();
	    aa_resize (context);
	    scene8();
	    aa_resize (context);
	    scene6(0);
	    aa_resize (context);
	case 2:
	    if (quitnow)
		goto quit;
	    aa_resize (context);
	    vezen(&kt1, &kt2, &kt3, &kt4);
	    messager("KAMIL TOMAN known as KT, Kato, Whale, Bart\n"
		 "birth: May 19 1979, Tabor, Czech Republic, sex: male\n"
		     "\n"
		     "1993 - Became a linux extremist\n"
		     "1993 - Successful attempt to establish a secret organization\n"
		     "       Commandline Brotherhood\n"
		     "1995 - Action 'koules' - a secret project to train brotherhood\n"
		     "       members - covered under a game design\n"
		     "\n"
		 "1998 - Heading a new wave of command line revolution\n"
		     "\n"
		     "Contact address: toman@horac.ta.jcu.cz");
	    bbupdate();
	    starttime = endtime = TIME;
	    devezen1();
	    aa_resize (context);
	    if (quitnow)
		goto quit;
	    aa_resize (context);
	    scene7();
	    aa_resize (context);
	    scene9();
	    aa_resize (context);
	    if (quitnow)
		goto quit;
	    aa_resize (context);
	    scene5();
	    if (quitnow)
		goto quit;
	    aa_resize (context);
	    scene10();
	    vezen(&hh1, &hh2, &hh3, &hh4);
	    messager("JAN HUBICKA known as HH, Jahusoft, HuJaSoft, JHS, UNIX, Honza\n"
		  "birth: Apr 1 1978, Tabor, Czech Republic, sex: male\n"
		     "\n"
		     "1991 - Installed underground hackers OS Linux\n"
		     "1995 - Headed Action 'koules'\n"
		     "1996 - Famous troan XaoS to convert all windows instalations\n"
		     "       into Linux\n"
		     "\n"
		     "1998 - Secret plan to make `Text Windows` system to confuse users\n"
		 "2001 - Planning an assassination of dictator Bill G.\n"
		     "\n"
		     "Contact address: hubicka@paru.cas.cz");
	    aa_resize (context);
	    devezen4();
	    if (quitnow)
		goto quit;
	    aa_resize (context);
	    credits();
	    if (quitnow)
		goto quit;
	case 3:
	    if (loopmode)
		break;
	    aa_resize (context);
	    credits2();
	}
    while (loopmode);
  quit:;
    aa_close(context);
    return (0);
}
