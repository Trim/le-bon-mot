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

G_BEGIN_DECLS

/*
 * Public structure
 * */

typedef enum {
  LE_BON_MOT_LETTER_UNKOWN,
  LE_BON_MOT_LETTER_NOT_PRESENT,
  LE_BON_MOT_LETTER_PRESENT,
  LE_BON_MOT_LETTER_WELL_PLACED,
} LeBonMotLetterState;

typedef struct {
  gchar letter;
  guint found;
  LeBonMotLetterState state;
} LeBonMotLetter;

/*
 * Type declaration.
 * */
#define LE_BON_MOT_TYPE_ENGINE le_bon_mot_engine_get_type ()
G_DECLARE_FINAL_TYPE(LeBonMotEngine, le_bon_mot_engine, LE_BON_MOT, ENGINE, GObject);

/*
 * Public method definitions.
 * */
LeBonMotEngine *le_bon_mot_engine_new(void);

// Return the current game board state inside a matrix of GPtrArray
// (rows and columns) of LeBonMotLetter
GPtrArray* le_bon_mot_engine_get_board_state (LeBonMotEngine *self);

void le_bon_mot_engine_add_letter (LeBonMotEngine *self, const char *letter);

void le_bon_mot_engine_remove_letter (LeBonMotEngine *self);

void le_bon_mot_engine_validate(LeBonMotEngine *self);

G_END_DECLS
