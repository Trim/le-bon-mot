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
#include <glib-2.0/glib.h>

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

typedef enum {
  LE_BON_MOT_ENGINE_STATE_CONTINUE,
  LE_BON_MOT_ENGINE_STATE_LOST,
  LE_BON_MOT_ENGINE_STATE_WON,
} LeBonMotEngineState;

typedef struct {
  gchar letter;
  LeBonMotLetterState state;
} LeBonMotLetter;

/*
 * Errors
 * */

typedef enum {
  LE_BON_MOT_ENGINE_ERROR_LINE_INCOMPLETE,
  LE_BON_MOT_ENGINE_ERROR_WORD_UNKOWN,
} LeBonMotEngineError;

#define LE_BON_MOT_ENGINE_ERROR le_bon_mot_engine_error_quark()

/*
 * Type declaration.
 * */

#define LE_BON_MOT_TYPE_ENGINE le_bon_mot_engine_get_type ()

/*
 * This work is normally done by G_DECLARE_FINAL_TYPE macro
 * We cannot use this macro directly, because we need to add class members
 * */
GType le_bon_mot_engine_get_type(void);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _LeBonMotEngine LeBonMotEngine;

typedef struct _LeBonMotEngineClass LeBonMotEngineClass;

_GLIB_DEFINE_AUTOPTR_CHAINUP (LeBonMotEngine, GObject)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (LeBonMotEngineClass, g_type_class_unref)

G_GNUC_UNUSED static inline LeBonMotEngine* LE_BON_MOT_ENGINE (gpointer ptr)
{
  return G_TYPE_CHECK_INSTANCE_CAST (ptr, le_bon_mot_engine_get_type(), LeBonMotEngine);
}

G_GNUC_UNUSED static inline LeBonMotEngineClass *LE_BON_MOT_ENGINE_CLASS (gpointer ptr)
{
  return G_TYPE_CHECK_CLASS_CAST (ptr, le_bon_mot_engine_get_type (), LeBonMotEngineClass);
}

G_GNUC_UNUSED static inline gboolean LE_BON_MOT_IS_ENGINE (gpointer ptr)
{
  return G_TYPE_CHECK_INSTANCE_TYPE (ptr, le_bon_mot_engine_get_type());
}

G_GNUC_UNUSED static inline gboolean LE_BON_MOT_IS_ENGINE_CLASS (gpointer ptr)
{
  return G_TYPE_CHECK_CLASS_TYPE (ptr, le_bon_mot_engine_get_type());
}

G_GNUC_UNUSED static inline LeBonMotEngineClass * LE_BON_MOT_ENGINE_GET_CLASS (gpointer ptr)
{
  return G_TYPE_INSTANCE_GET_CLASS (ptr, le_bon_mot_engine_get_type(), LeBonMotEngineClass);
}

G_GNUC_END_IGNORE_DEPRECATIONS

/*
 * End of G_DECLARE_FINAL_TYPE macro work
 * */

/*
 * Public method definitions.
 * */
LeBonMotEngine *le_bon_mot_engine_new(void);

// Return the current game board state inside a matrix of GPtrArray
// (rows and columns) of LeBonMotLetter
GPtrArray* le_bon_mot_engine_get_board_state (LeBonMotEngine *self);

void le_bon_mot_engine_add_letter (LeBonMotEngine *self, const char *letter);

void le_bon_mot_engine_remove_letter (LeBonMotEngine *self);

LeBonMotEngineState le_bon_mot_engine_get_game_state (LeBonMotEngine *self);
void le_bon_mot_engine_validate(LeBonMotEngine *self, GError **error);

guint le_bon_mot_engine_get_current_row (LeBonMotEngine *self);

GString *le_bon_mot_engine_get_word(LeBonMotEngine *self);

G_END_DECLS
