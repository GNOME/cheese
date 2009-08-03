/*
 * Copyright © 2008 James Liggett <jrliggett@cox.net>
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

#ifndef _CHEESE_PREFS_DIALOG_H_
#define _CHEESE_PREFS_DIALOG_H_

#ifdef HAVE_CONFIG_H
  #include "cheese-config.h"
#endif

#include "cheese-webcam.h"
#include "cheese-prefs-dialog-widgets.h"
#include "cheese-prefs-resolution-combo.h"
#include "cheese-prefs-webcam-combo.h"
#include "cheese-prefs-balance-scale.h"
#include "cheese-prefs-burst-spinbox.h"

void cheese_prefs_dialog_run (GtkWidget *parent, CheeseGConf *gconf, CheeseWebcam *webcam);

#endif /* _CHEESE_PREFS_DIALOG_H_ */
