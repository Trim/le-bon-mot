/* muttum-engine.h
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

/**
 * MuttumLetterState:
 *
 * State of a letter inside the engine
 */
typedef enum {
  MUTTUM_LETTER_UNKOWN,
  MUTTUM_LETTER_NOT_PRESENT,
  MUTTUM_LETTER_PRESENT,
  MUTTUM_LETTER_WELL_PLACED,
} MuttumLetterState;

/**
 * MuttumEngineState:
 *
 * State of the game linked to this engine
 */
typedef enum {
  MUTTUM_ENGINE_STATE_CONTINUE,
  MUTTUM_ENGINE_STATE_LOST,
  MUTTUM_ENGINE_STATE_WON,
} MuttumEngineState;

/**
 * MuttumLetter:
 *
 * Structure containing a letter symbol and its state
 */
typedef struct {
  gchar letter;
  MuttumLetterState state;
} MuttumLetter;

/**
 * MuttumEngineError:
 *
 * These errors occur when user try to validate a line.
 */
typedef enum {
  MUTTUM_ENGINE_ERROR_LINE_INCOMPLETE,
  MUTTUM_ENGINE_ERROR_WORD_UNKOWN,
} MuttumEngineError;

#define MUTTUM_ENGINE_ERROR muttum_engine_error_quark()

/*
 * Type declaration.
 * */

#define MUTTUM_TYPE_ENGINE muttum_engine_get_type ()

/*
 * This work is normally done by G_DECLARE_FINAL_TYPE macro
 * We cannot use this macro directly, because we need to add class members
 * */
GType muttum_engine_get_type();

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct _MuttumEngine MuttumEngine;

typedef struct _MuttumEngineClass MuttumEngineClass;

_GLIB_DEFINE_AUTOPTR_CHAINUP (MuttumEngine, GObject)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (MuttumEngineClass, g_type_class_unref)

G_GNUC_UNUSED static inline MuttumEngine* MUTTUM_ENGINE (gpointer ptr)
{
  return G_TYPE_CHECK_INSTANCE_CAST (ptr, muttum_engine_get_type(), MuttumEngine);
}

G_GNUC_UNUSED static inline MuttumEngineClass *MUTTUM_ENGINE_CLASS (gpointer ptr)
{
  return G_TYPE_CHECK_CLASS_CAST (ptr, muttum_engine_get_type (), MuttumEngineClass);
}

G_GNUC_UNUSED static inline gboolean MUTTUM_IS_ENGINE (gpointer ptr)
{
  return G_TYPE_CHECK_INSTANCE_TYPE (ptr, muttum_engine_get_type());
}

G_GNUC_UNUSED static inline gboolean MUTTUM_IS_ENGINE_CLASS (gpointer ptr)
{
  return G_TYPE_CHECK_CLASS_TYPE (ptr, muttum_engine_get_type());
}

G_GNUC_UNUSED static inline MuttumEngineClass * MUTTUM_ENGINE_GET_CLASS (gpointer ptr)
{
  return G_TYPE_INSTANCE_GET_CLASS (ptr, muttum_engine_get_type(), MuttumEngineClass);
}

G_GNUC_END_IGNORE_DEPRECATIONS

/*
 * End of G_DECLARE_FINAL_TYPE macro work
 * */

/*
 * Public method definitions.
 * */

GPtrArray* muttum_engine_get_board_state (MuttumEngine *self);

GPtrArray* muttum_engine_get_alphabet_state (MuttumEngine *self);

void muttum_engine_add_letter (MuttumEngine *self, const char letter);

void muttum_engine_remove_letter (MuttumEngine *self);

MuttumEngineState muttum_engine_get_game_state (MuttumEngine *self);
void muttum_engine_validate(MuttumEngine *self, GError **error);

guint muttum_engine_get_current_row (MuttumEngine *self);

GString *muttum_engine_get_word(MuttumEngine *self);

G_END_DECLS
