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

#include <unistd.h>
#include "timers.h"
#include "bb.h"
#ifdef HAVE_LIBMIKMOD
#include <mikmod.h>
MODULE *module;
int bbsound;
void stop();
tl_timer *update_timer;
static int freqs[14] = {
  5512, 6615, 8000, 9600, 11025, 16000, 18900,
  22050, 27428, 32000, 33075, 37800, 44100, 48000
};

static void
update_sound (void *data)
{
  if (Player_Active ())
    MikMod_Update ();
  else if (module)
    Player_SetPosition (0);
}
#endif

int
load_song (char *name)
{
#ifdef HAVE_LIBMIKMOD
  finish_stuff = 0;
  if (bbsound)
    {
      char *name2;
      name2 = malloc (strlen (name) + strlen(SOUNDDIR)+2);
      sprintf(name2, "%s/%s",SOUNDDIR, name);
      stop ();
      if (module != NULL)
	Player_Free (module);
      module = Player_Load (name2, 64, 0);
      if (!module)
        module = Player_Load (name, 64, 0);
      free (name2);
      if (module == NULL)
	{
	  aa_printf (context, 0, 0, AA_SPECIAL,
		     "Failed to load module:%s",
		     MikMod_strerror (MikMod_errno));
	  aa_flush (context);
	  sleep (1);
	}
    }
  return 0;
#endif
}

void
play ()
{
#ifdef HAVE_LIBMIKMOD
  if (module != NULL)
    {
      Player_Start (module);
      update_sound (NULL);
      if (!update_timer)
	{
	  update_timer = tl_create_timer ();
	  tl_set_handler (update_timer, update_sound, NULL);
	  tl_set_interval (update_timer, 10000);
	  tl_add_timer (syncgroup, update_timer);
	  tl_reset_timer (update_timer);
	}
    }
#endif
}

void
stop ()
{
#ifdef HAVE_LIBMIKMOD
  if (bbsound)
    {
      if (module)
        Player_Free (module);
      module = NULL;
      if (update_timer)
	{
	  tl_free_timer (update_timer);
	  update_timer = NULL;
	}
    }
#endif
}
#ifdef HAVE_LIBMIKMOD
struct table
{
  int v;
  char *c;
};
static struct table stable[] = { {DMODE_16BITS, "16 bit output"},
{DMODE_STEREO, "Stereo output"},
{DMODE_SOFT_MUSIC, "Process music via software mixer"},
{DMODE_HQMIXER, "Use high-quality (slower) software mixer"},
{DMODE_SURROUND, "Surround sound"},
{DMODE_INTERP, "Interpolation"},
{DMODE_REVERSE, "Reverse Stereo"},
{0, NULL}
};

int cont;
int srate;
int
ptable ()
{
  int i;
  for (i = 0; stable[i].c; i++)
    aa_printf (context, 0, i, AA_SPECIAL, "%i:%s - %-40s", i,
	       md_mode & stable[i].v ? "Yes" : "No ", stable[i].c);
  aa_printf (context, 0, i, AA_SPECIAL,
	     "%i:Sample rate: %i                            ", i, md_mixfreq);
  aa_printf (context, 0, i + 1, AA_SPECIAL,
	     "%i:Continue                                      ", i + 1);
  srate = i;
  cont = i + 1;
}
#endif

int
main (int argc, char *argv[])
{
  int p = 0;
  int retval;

  bbinit (argc, argv);
#ifdef HAVE_LIBMIKMOD
  aa_puts (context, 0, p++, AA_SPECIAL, "Music?[Y/n]");
  aa_flush (context);
  if (tolower (aa_getkey (context, 1)) != 'n')
    {
      MikMod_RegisterAllDrivers ();
      MikMod_RegisterLoader (&load_s3m);
      /*md_mode |= DMODE_SOFT_MUSIC; */
      while (1)
	{
	  int k;
          aa_resize(context);
	  ptable ();
	  aa_flush (context);
	  k = aa_getkey (context, 1);
	  if (k >= '0' && k <= '0' + cont)
	    {
	      k -= '0';
	      if (k == cont)
		break;
	      if (k == srate)
		{
		  int i;
		  for (i = 0; i < 14; i++)
		    if (freqs[i] == md_mixfreq)
		      break;
		  md_mixfreq = freqs[(i + 1) % 14];
		}
	      if (k < srate)
		md_mode ^= stable[k].v;
	    }
	}
      bbsound = 1;
      if (MikMod_Init (""))
	{
	  aa_printf (context, 0, p++, AA_SPECIAL,
		     "Sound initialization failed:%s",
		     MikMod_strerror (MikMod_errno));
	  aa_flush (context);
	  bbsound = 0;
	  sleep (1);
	}
    }
#endif
  aa_resize(context);
  retval = bb ();
#ifdef HAVE_LIBMIKMOD
  if (bbsound)
    {
      MikMod_Exit ();
    }
#endif
  return retval;
}
