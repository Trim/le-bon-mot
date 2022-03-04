/* le_bon_mot-engine.h
 *
 * Copyright 2022 Adrien Dorsaz
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

#pragma once

#include <glib-object.h>
#include "glib-2.0/glib.h"
#include "gtk/gtk.h"

G_BEGIN_DECLS

/*
 * Type declaration.
 * */
#define LE_BON_MOT_TYPE_ENGINE le_bon_mot_engine_get_type ()
G_DECLARE_FINAL_TYPE(LeBonMotEngine, le_bon_mot_engine, LE_BON_MOT, ENGINE, GObject);

/*
 * Method definitions
 * */
LeBonMotEngine *le_bon_mot_engine_new(void);
void le_bon_mot_engine_show_board (LeBonMotEngine *self,
                                   GtkGrid        *grid);

G_END_DECLS
