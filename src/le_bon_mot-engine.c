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
#include <gio/gio.h>

const guint LE_BON_MOT_ENGINE_ROWS = 6;

static GString *le_bon_mot_engine_word_init();
static GPtrArray *le_bon_mot_engine_alphabet_init(GString *word);
static GPtrArray *le_bon_mot_engine_board_init(GString *word);

const gchar LE_BON_MOT_NULL_LETTER = '.';

typedef struct {
  gchar letter;
  LeBonMotLetterState state;
  guint found;
  GPtrArray *position;
} LeBonMotLetterPrivate;

typedef struct {
  GString *word;
  GPtrArray *alphabet;
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

  g_ptr_array_unref(priv->alphabet);

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
  priv->alphabet = le_bon_mot_engine_alphabet_init(priv->word);
  priv->board = le_bon_mot_engine_board_init(priv->word); 
  priv->current_row = 0;
  priv->is_finished = FALSE;
}

static GString *le_bon_mot_engine_word_init() {
  // TODO use a dictionary and some randomness
  GString *word_found = g_string_new(NULL);
  GFile *dictionary = g_file_new_for_uri("file:///usr/share/dict/french");
  GError *error = NULL;

  // Compile regexp to lookup word
  GRegex *word_lookup = g_regex_new("^\\w{5,8}$", G_REGEX_MULTILINE, 0, &error);
  if (!word_lookup) {
    g_print("Error while compiling regexp: code: %d, message: %s", error->code, error->message);
    return g_string_new(g_utf8_normalize("erreur", -1, G_NORMALIZE_ALL));
  }

  // Get file info to know where to skip
  GFileInfo* dictionary_info = g_file_query_info(dictionary, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NONE, NULL, &error);
  if (!dictionary_info) {
    g_print("Error while reading dictionary file info: code: %d, message: %s", error->code, error->message);
    return g_string_new(g_utf8_normalize("erreur", -1, G_NORMALIZE_ALL));
  }
  goffset offset = g_file_info_get_size(dictionary_info);
  // Open file for read
  GFileInputStream *dictionary_stream = g_file_read(dictionary, NULL, &error);
  if (!dictionary_stream) {
    g_print("Error occured while opening dictionary: code: %d, message: %s", error->code, error->message);
    return g_string_new(g_utf8_normalize("erreur", -1, G_NORMALIZE_ALL));
  }

  gssize read;
  gchar buffer[100];
  GMatchInfo *match;

  gssize skip = g_random_int_range(0, offset - 100);
  read = g_input_stream_skip(G_INPUT_STREAM(dictionary_stream), skip, NULL, &error);
  if (read < 0) {
    g_print("Error occured while reading (skip) dictionary: code: %d, message: %s", error->code, error->message);
    return g_string_new(g_utf8_normalize("erreur", -1, G_NORMALIZE_ALL));
  }

  while(TRUE) {
    read = g_input_stream_read(G_INPUT_STREAM(dictionary_stream), buffer, G_N_ELEMENTS(buffer) - 1, NULL, &error);
    if (read > 0) {
      buffer[read] = '\0';
      if (g_regex_match_all(word_lookup, buffer, G_REGEX_MATCH_NOTEMPTY, &match)) {
        g_string_append(word_found, g_match_info_fetch(match, 0)); 
        g_print("Found: %s\n", word_found->str);
        break;
      }
    } else if (read < 0) {
      g_print("Error occured while reading dictionary: code: %d, message: %s", error->code, error->message);
      return g_string_new(g_utf8_normalize("erreur", -1, G_NORMALIZE_ALL));
    } else {
      break;
    }
  }

  g_input_stream_close(G_INPUT_STREAM(dictionary_stream), NULL, NULL);
  g_match_info_unref(match);
  return g_string_new(g_utf8_normalize(word_found->str, word_found->allocated_len, G_NORMALIZE_ALL));
}

static void le_bon_mot_engine_letter_private_free(gpointer data) {
  LeBonMotLetterPrivate* letter = data;
  g_ptr_array_unref(letter->position);
  g_free(letter);
}

static GPtrArray *le_bon_mot_engine_alphabet_init(GString *word)
{
  GPtrArray *alphabet = g_ptr_array_new_full(
      26,
      le_bon_mot_engine_letter_private_free
  );

  for (gchar i = 'a'; i <= 'z'; i += 1)
  {
    LeBonMotLetterPrivate *letter = g_new(LeBonMotLetterPrivate, 1);
    letter->letter = i;
    letter->position = g_ptr_array_new_with_free_func(g_free);
    letter->found = 0;
    letter->state = LE_BON_MOT_LETTER_UNKOWN;
    for (guint j = 0; j < word->len; j += 1)
    {
      if (i == word->str[j])
      {
        guint *position = g_new(guint, 1);
        *position = j;
        g_ptr_array_add(letter->position, position);
      }
    }
    g_ptr_array_add(alphabet, letter);
  }

  return alphabet;
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
      } else {
        letter->letter = LE_BON_MOT_NULL_LETTER;
      }
      letter->state = LE_BON_MOT_LETTER_UNKOWN;
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

static gboolean
le_bon_mot_engine_compare_letter_private_with_letter (
    gconstpointer letter_private,
    gconstpointer letter
) {
  const LeBonMotLetterPrivate *first = letter_private;
  const LeBonMotLetter *second = letter;
  return first->letter == second->letter;
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

  // Reset alphabet found
  for (guint i = 0; i < priv->alphabet->len; i += 1) {
    LeBonMotLetterPrivate *alphabet = g_ptr_array_index(priv->alphabet, i);
    alphabet->found = 0;
  }

  // Validate state for each letter on current row
  
  // First pass find all well placed letters
  guint well_placed = 0;
  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);

    // Alphabet is used to store found letter count
    guint* alphabet_index = g_new(guint, 1);
    LeBonMotLetterPrivate *alphabet = NULL;
    if (g_ptr_array_find_with_equal_func(priv->alphabet, letter,
          le_bon_mot_engine_compare_letter_private_with_letter,
          alphabet_index)) {
      alphabet = g_ptr_array_index(priv->alphabet, *alphabet_index);
    }
    g_return_if_fail(alphabet);

    if (priv->word->str[col] == letter->letter) {
      letter->state = LE_BON_MOT_LETTER_WELL_PLACED;
      alphabet->found++;
      well_placed++;
    }
  }

  if (well_placed == row->len) {
    priv->is_finished = TRUE;
    return;
  }

  // Second pass find all letters present but not well placed
  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);

    // Alphabet is used to store found letter count
    guint* alphabet_index = g_new(guint, 1);
    LeBonMotLetterPrivate *alphabet = NULL;
    if (g_ptr_array_find_with_equal_func(priv->alphabet, letter,
          le_bon_mot_engine_compare_letter_private_with_letter,
          alphabet_index)) {
      alphabet = g_ptr_array_index(priv->alphabet, *alphabet_index);
    }
    g_return_if_fail(alphabet);

    if (priv->word->str[col] != letter->letter) {
      if (alphabet->found < alphabet->position->len) {
        letter->state = LE_BON_MOT_LETTER_PRESENT;
        alphabet->found++;
      } else {
        letter->state = LE_BON_MOT_LETTER_NOT_PRESENT;
      }
    }
  }

  // Move to next row
  priv->current_row++;

  if (priv->current_row < LE_BON_MOT_ENGINE_ROWS) {
    GPtrArray* nextRow = g_ptr_array_index(priv->board, priv->current_row);
    LeBonMotLetter* firstLetter = g_ptr_array_index(nextRow, 0);
    firstLetter->letter = priv->word->str[0];
  }
}

guint le_bon_mot_engine_get_current_row (LeBonMotEngine *self) {
  g_return_val_if_fail(LE_BON_MOT_IS_ENGINE(self), -1);
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);
  return priv->current_row;
}
