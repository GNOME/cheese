/*
 * Copyright © 2010 Filippo Argiolas <filippo.argiolas@gmail.com>
 * Copyright © 2007,2008 daniel g. siegel <dgsiegel@gnome.org>
 * Copyright © 2007,2008 Jaap Haitsma <jaap@haitsma.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cheese-commands.h"

void
cheese_cmd_help_contents (GtkAction *action, CheeseWindow *cheese_window)
{
  GError    *error = NULL;
  GdkScreen *screen;

  screen = gtk_widget_get_screen (GTK_WIDGET (cheese_window));
  gtk_show_uri (screen, "ghelp:cheese", gtk_get_current_event_time (), &error);

  if (error != NULL)
  {
    GtkWidget *d;
    d = gtk_message_dialog_new (GTK_WINDOW (cheese_window),
                                GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                _("Unable to open help file for Cheese"));
    gtk_dialog_run (GTK_DIALOG (d));
    gtk_widget_destroy (d);
    g_error_free (error);
  }
}

void
cheese_cmd_about (GtkAction *action, CheeseWindow *cheese_window)
{
  static const char *authors[] = {
    "daniel g. siegel <dgsiegel@gnome.org>",
    "Jaap A. Haitsma <jaap@haitsma.org>",
    "Filippo Argiolas <fargiolas@gnome.org>",
    "",
    "Aidan Delaney <a.j.delaney@brighton.ac.uk>",
    "Alex \"weej\" Jones <alex@weej.com>",
    "Andrea Cimitan <andrea.cimitan@gmail.com>",
    "Baptiste Mille-Mathias <bmm80@free.fr>",
    "Cosimo Cecchi <anarki@lilik.it>",
    "Diego Escalante Urrelo <dieguito@gmail.com>",
    "Felix Kaser <f.kaser@gmx.net>",
    "Gintautas Miliauskas <gintas@akl.lt>",
    "Hans de Goede <jwrdegoede@fedoraproject.org>",
    "James Liggett <jrliggett@cox.net>",
    "Luca Ferretti <elle.uca@libero.it>",
    "Mirco \"MacSlow\" Müller <macslow@bangang.de>",
    "Patryk Zawadzki <patrys@pld-linux.org>",
    "Ryan Zeigler <zeiglerr@gmail.com>",
    "Sebastian Keller <sebastian-keller@gmx.de>",
    "Steve Magoun <steve.magoun@canonical.com>",
    "Thomas Perl <thp@thpinfo.com>",
    "Tim Philipp Müller <tim@centricular.net>",
    "Todd Eisenberger <teisenberger@gmail.com>",
    "Tommi Vainikainen <thv@iki.fi>",
    NULL
  };

  static const char *artists[] = {
    "Andreas Nilsson <andreas@andreasn.se>",
    "Josef Vybíral <josef.vybiral@gmail.com>",
    "Kalle Persson <kalle@kallepersson.se>",
    "Lapo Calamandrei <calamandrei@gmail.com>",
    "Or Dvory <gnudles@nana.co.il>",
    "Ulisse Perusin <ulisail@yahoo.it>",
    NULL
  };

  static const char *documenters[] = {
    "Joshua Henderson <joshhendo@gmail.com>",
    "Jaap A. Haitsma <jaap@haitsma.org>",
    "Felix Kaser <f.kaser@gmx.net>",
    NULL
  };

  const char *translators;

  translators = _("translator-credits");

  const char *license[] = {
    N_("This program is free software; you can redistribute it and/or modify "
       "it under the terms of the GNU General Public License as published by "
       "the Free Software Foundation; either version 2 of the License, or "
       "(at your option) any later version.\n"),
    N_("This program is distributed in the hope that it will be useful, "
       "but WITHOUT ANY WARRANTY; without even the implied warranty of "
       "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the "
       "GNU General Public License for more details.\n"),
    N_("You should have received a copy of the GNU General Public License "
       "along with this program. If not, see <http://www.gnu.org/licenses/>.")
  };

  char *license_trans;

  license_trans = g_strconcat (_(license[0]), "\n", _(license[1]), "\n", _(license[2]), "\n", NULL);

  gtk_show_about_dialog (GTK_WINDOW (cheese_window),
                         "version", VERSION,
                         "copyright", "Copyright \xc2\xa9 2007 - 2009\n daniel g. siegel <dgsiegel@gnome.org>",
                         "comments", _("Take photos and videos with your webcam, with fun graphical effects"),
                         "authors", authors,
                         "translator-credits", translators,
                         "artists", artists,
                         "documenters", documenters,
                         "website", "http://projects.gnome.org/cheese",
                         "website-label", _("Cheese Website"),
                         "logo-icon-name", "cheese",
                         "wrap-license", TRUE,
                         "license", license_trans,
                         NULL);

  g_free (license_trans);
}
