/*
 * Cheese - Take photos and videos with your webcam, with fun graphical effects
 * Copyright © 2013 David King <amigadave@amigadave.com>
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

int main (string[] args)
{
    Intl.bindtextdomain (Config.GETTEXT_PACKAGE, Config.PACKAGE_LOCALEDIR);
    Intl.bind_textdomain_codeset (Config.GETTEXT_PACKAGE, "UTF-8");
    Intl.textdomain (Config.GETTEXT_PACKAGE);

    if (!Cheese.gtk_init (ref args))
    {
        return Posix.EXIT_FAILURE;
    }

    Cheese.Application app;
    app = new Cheese.Application ("org.gnome.Cheese");

    app.activate.connect (app.on_app_activate);
    int status = app.run (args);

    return status;
}
