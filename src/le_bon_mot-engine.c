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

typedef enum {
  LETTER_UNKOWN,
  LETTER_NOT_PRESENT,
  LETTER_PRESENT,
  LETTER_WELL_PLACED,
} LetterState;

typedef struct {
  gchar letter;
  LetterState state;
} Letter;

static GString *le_bon_mot_engine_init_word();
static GPtrArray *le_bon_mot_engine_init_board(GString *word);

typedef struct {
  GString *word;
  GPtrArray *board;
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

  // Dispose the word
  g_clear_object (&priv->word);
  // Dispose the board
  for (guint rowIndex = 0; rowIndex < priv->board->len; rowIndex +=1 ) {
    GPtrArray *row = g_ptr_array_index(priv->board, rowIndex);
    for (guint columnIndex = 0; columnIndex < row->len; columnIndex += 1) {
      Letter *letter = g_ptr_array_index(row, columnIndex);
      g_free(letter);
    }
    g_clear_object(&row);
  }
  g_clear_object(&priv->board);

  G_OBJECT_CLASS (le_bon_mot_engine_parent_class)->dispose (gobject);
}

static void
le_bon_mot_engine_finalize (GObject *gobject)
{
  G_OBJECT_CLASS (le_bon_mot_engine_parent_class)->finalize (gobject);
}

static void
le_bon_mot_engine_class_init(LeBonMotEngineClass *klass) {
}

static void
le_bon_mot_engine_init(LeBonMotEngine *self) {
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  priv->word = le_bon_mot_engine_init_word();
  priv->board = le_bon_mot_engine_init_board(priv->word); 
}

static GString *le_bon_mot_engine_init_word() {
  // TODO use a dictionary and some randomness
  return g_string_new("animal");
}

static GPtrArray *le_bon_mot_engine_init_board(GString *word) {
  GPtrArray *board = g_ptr_array_sized_new(LE_BON_MOT_ENGINE_ROWS);
  for (guint rowIndex = 0; rowIndex < LE_BON_MOT_ENGINE_ROWS; rowIndex += 1) {
    GPtrArray *row = g_ptr_array_sized_new(word->len);
    for (guint columnIndex = 0; columnIndex < word->len; columnIndex += 1) {
      Letter *letter = g_new(Letter, 1);
      if (rowIndex == 0 && columnIndex == 0) {
        letter->letter = word->str[0];
        letter->state = LETTER_WELL_PLACED;
      } else {
        letter->letter = '_';
        letter->state = LETTER_UNKOWN;
      }
      g_ptr_array_add(row, letter);
    }
    g_ptr_array_add(board, row);
  }
  return board;
}

void le_bon_mot_engine_show_board(LeBonMotEngine* self, GtkGrid *grid) {
  g_return_if_fail(LE_BON_MOT_IS_ENGINE(self));

  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  for (guint rowIndex = 0; rowIndex < priv->board->len; rowIndex +=1 ) {
    GPtrArray *row = g_ptr_array_index(priv->board, rowIndex);
    for (guint columnIndex = 0; columnIndex < row->len; columnIndex += 1) {
      Letter *letter = g_ptr_array_index(row, columnIndex);
      GString *label = g_string_new("");
      g_string_append_c(label, letter->letter);
      gtk_grid_attach(grid, gtk_label_new(label->str), columnIndex, rowIndex, 1, 1);
    }
  }
}
