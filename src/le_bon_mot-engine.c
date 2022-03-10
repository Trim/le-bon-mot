/* le_bon_mot-engine.c
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

#include "le_bon_mot-engine.h"

const guint LE_BON_MOT_ENGINE_ROWS = 6;

static GString *le_bon_mot_engine_word_init();
static GPtrArray *le_bon_mot_engine_board_init(GString *word);

const gchar LE_BON_MOT_NULL_LETTER = '_';

typedef struct {
  GString *word;
  GPtrArray *board;
  guint current_row;
  gboolean is_finished;
} LeBonMotEnginePrivate;

struct _LeBonMotEngine
{
  GObject parent_instance;
};

G_DEFINE_TYPE_WITH_PRIVATE(LeBonMotEngine, le_bon_mot_engine, G_TYPE_OBJECT);

static void
le_bon_mot_engine_dispose (GObject *gobject)
{
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private (LE_BON_MOT_ENGINE (gobject));

  // Board is initialized to cascade unref to rows and free letters
  g_ptr_array_unref(priv->board);

  G_OBJECT_CLASS (le_bon_mot_engine_parent_class)->dispose (gobject);
}

static void
le_bon_mot_engine_finalize (GObject *gobject)
{
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private (LE_BON_MOT_ENGINE (gobject));

  // Word
  g_string_free (priv->word, TRUE);

  G_OBJECT_CLASS (le_bon_mot_engine_parent_class)->finalize (gobject);
}

static void
le_bon_mot_engine_class_init(LeBonMotEngineClass *klass) {
  GObjectClass *g_object_class = G_OBJECT_CLASS(klass);

  g_object_class->finalize = le_bon_mot_engine_finalize;
  g_object_class->dispose = le_bon_mot_engine_dispose;
}

static void
le_bon_mot_engine_init(LeBonMotEngine *self) {
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  priv->word = le_bon_mot_engine_word_init();
  priv->board = le_bon_mot_engine_board_init(priv->word); 
  priv->current_row = 0;
  priv->is_finished = FALSE;
}

static GString *le_bon_mot_engine_word_init() {
  // TODO use a dictionary and some randomness
  return g_string_new("animal");
}

static void le_bon_mot_engine_board_destroy_row(gpointer data) {
  g_ptr_array_unref(data);
}

static GPtrArray *le_bon_mot_engine_board_init(GString *word) {
  GPtrArray *board = g_ptr_array_new_full(
      LE_BON_MOT_ENGINE_ROWS,
      le_bon_mot_engine_board_destroy_row
  );
  for (guint rowIndex = 0; rowIndex < LE_BON_MOT_ENGINE_ROWS; rowIndex += 1) {
    GPtrArray *row = g_ptr_array_new_full(word->len, g_free);
    for (guint columnIndex = 0; columnIndex < word->len; columnIndex += 1) {
      LeBonMotLetter *letter = g_new(LeBonMotLetter, 1);
      if (rowIndex == 0 && columnIndex == 0) {
        letter->letter = word->str[0];
        letter->state = LE_BON_MOT_LETTER_WELL_PLACED;
      } else {
        letter->letter = LE_BON_MOT_NULL_LETTER;
        letter->state = LE_BON_MOT_LETTER_UNKOWN;
      }
      g_ptr_array_add(row, letter);
    }
    g_ptr_array_add(board, row);
  }
  return board;
}

static gpointer le_bon_mot_engine_board_copy_letter(gconstpointer src, gpointer data)
{
  LeBonMotLetter *copy = g_new(LeBonMotLetter, 1);
  const LeBonMotLetter *letter = src;
  copy->letter = letter->letter;
  copy->found = letter->found;
  copy->state = letter->state;
  return copy;
}

static gpointer le_bon_mot_engine_board_copy_row (gconstpointer src, gpointer data)
{
  return g_ptr_array_copy((GPtrArray *) src, le_bon_mot_engine_board_copy_letter, NULL);
}

GPtrArray* le_bon_mot_engine_get_board_state(LeBonMotEngine* self) {
  g_return_val_if_fail(LE_BON_MOT_IS_ENGINE(self), NULL);
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  return g_ptr_array_copy(priv->board, le_bon_mot_engine_board_copy_row, NULL);
}

void le_bon_mot_engine_add_letter (LeBonMotEngine *self, const char *newLetter)
{
  g_return_if_fail(LE_BON_MOT_IS_ENGINE(self));
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  if (priv->current_row >= LE_BON_MOT_ENGINE_ROWS || priv->is_finished) {
    return;
  }
  
  GPtrArray* row = g_ptr_array_index(priv->board, priv->current_row);

  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);
    if (letter->letter == LE_BON_MOT_NULL_LETTER) {
      letter->letter = newLetter[0];
      letter->state = LE_BON_MOT_LETTER_UNKOWN;
      break;
    }
  }
}

void le_bon_mot_engine_remove_letter (LeBonMotEngine *self)
{
  g_return_if_fail(LE_BON_MOT_IS_ENGINE(self));
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  if (priv->current_row >= LE_BON_MOT_ENGINE_ROWS || priv->is_finished) {
    return;
  }
  
  GPtrArray* row = g_ptr_array_index(priv->board, priv->current_row);

  for (guint col = row->len - 1; col <= row->len; col -= 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);
    if (letter->letter != LE_BON_MOT_NULL_LETTER && col != 0) {
      letter->letter = LE_BON_MOT_NULL_LETTER;
      letter->state = LE_BON_MOT_LETTER_UNKOWN;
      break;
    }
  }
}

void le_bon_mot_engine_validate(LeBonMotEngine *self) {
  g_return_if_fail(LE_BON_MOT_IS_ENGINE(self));
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);
  
  GPtrArray* row = g_ptr_array_index(priv->board, priv->current_row);
  
  if (priv->current_row >= LE_BON_MOT_ENGINE_ROWS || priv->is_finished) {
    return;
  }

  // Ensure all letters were given
  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);

    if (letter->letter == LE_BON_MOT_NULL_LETTER) {
      return;
    }
  }

  // Validate state for each letter on current row
  guint well_placed = 0;
  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);

    GString* needle = g_string_new("");
    g_string_append_c(needle, letter->letter);

    if (priv->word->str[col] == letter->letter) {
      letter->state = LE_BON_MOT_LETTER_WELL_PLACED;
      well_placed++;
    } else if(strstr(priv->word->str, needle->str)) {
      letter->state = LE_BON_MOT_LETTER_PRESENT;
    } else {
      letter->state = LE_BON_MOT_LETTER_NOT_PRESENT;
    }

    g_string_free(needle, TRUE);
  }

  if (well_placed == row->len) {
    priv->is_finished = TRUE;
    return;
  }

  // Move to next row
  priv->current_row++;

  if (priv->current_row < LE_BON_MOT_ENGINE_ROWS) {
    GPtrArray* nextRow = g_ptr_array_index(priv->board, priv->current_row);
    LeBonMotLetter* firstLetter = g_ptr_array_index(nextRow, 0);
    firstLetter->letter = priv->word->str[0];
    firstLetter->state = LE_BON_MOT_LETTER_WELL_PLACED;
  }
}

guint le_bon_mot_engine_get_current_row (LeBonMotEngine *self) {
  g_return_val_if_fail(LE_BON_MOT_IS_ENGINE(self), -1);
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);
  return priv->current_row;
}
