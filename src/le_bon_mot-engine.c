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

#include <gio/gio.h>
#include <unicode/ucol.h>
#include <unicode/utrans.h>
#include <unicode/ustring.h>
#include <unicode/utypes.h>

#include "le_bon_mot-engine.h"

const guint LE_BON_MOT_ENGINE_ROWS = 6;
const gchar LE_BON_MOT_ENGINE_NULL_LETTER = '.';
const guint LE_BON_MOT_ENGINE_WORD_LENGTH_MIN = 5;
const guint LE_BON_MOT_ENGINE_WORD_LENGTH_MAX = 8;
const guint LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE = 100;

// Le Bon Mot is currently only developped for French users
const gchar *LE_BON_MOT_ENGINE_COLLATION = "fr_FR";
const gchar *LE_BON_MOT_ENGINE_DICTIONARY_FILE_URI = "file:///usr/share/dict/french";

static GTree *le_bon_mot_engine_dictionary_init(UCollator *collator);
static GString *le_bon_mot_engine_word_init(GTree *dictionary);
static GPtrArray *le_bon_mot_engine_alphabet_init(GString *word);
static GPtrArray *le_bon_mot_engine_board_init(GString *word);

G_DEFINE_QUARK(le-bon-mot-engine-error-quark, le_bon_mot_engine_error);

typedef struct {
  gchar letter;
  LeBonMotLetterState state;
  guint found;
  GPtrArray *position;
} LeBonMotLetterPrivate;

typedef struct {
  GString* word;
  gboolean is_playable;
} DictionaryWord;

typedef struct {
  GString *word;
  GPtrArray *alphabet;
  GPtrArray *board;
  guint current_row;
  LeBonMotEngineState state;
  // TODO collator and dictionary should be moved to class properties
  GTree *dictionary;
  UCollator *collator;
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
  g_tree_unref(priv->dictionary);

  // Board is initialized to cascade unref to rows and free letters
  g_ptr_array_unref(priv->board);

  ucol_close(priv->collator);

  G_OBJECT_CLASS (le_bon_mot_engine_parent_class)->dispose (gobject);
}

static void
le_bon_mot_engine_class_init(LeBonMotEngineClass *klass) {
  GObjectClass *g_object_class = G_OBJECT_CLASS(klass);

  g_object_class->dispose = le_bon_mot_engine_dispose;
}

static void
le_bon_mot_engine_init(LeBonMotEngine *self) {
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);

  
  // Unicode collator give more tools to create dictionary
  UErrorCode status = U_ZERO_ERROR;
  priv->collator = ucol_open(LE_BON_MOT_ENGINE_COLLATION, &status);

  if (U_FAILURE(status)) {
    g_error("Unable to open unicode collator");
  }

  // Primary strength allow to work with only base characters
  // (neither case sensitive, nor accent sensitive)
  ucol_setStrength(priv->collator, UCOL_PRIMARY);

  priv->dictionary = le_bon_mot_engine_dictionary_init(priv->collator);
  priv->word = le_bon_mot_engine_word_init(priv->dictionary);
  priv->alphabet = le_bon_mot_engine_alphabet_init(priv->word);
  priv->board = le_bon_mot_engine_board_init(priv->word); 
  priv->current_row = 0;
  priv->state = LE_BON_MOT_ENGINE_STATE_CONTINUE;
}

static void le_bon_mot_engine_dictionary_destroy_value(gpointer data) {
  DictionaryWord* dword = data;
  g_string_free(dword->word, TRUE);
  g_free(dword);
}

static gint
le_bon_mot_engine_dictionary_compare_word (
    gconstpointer string1,
    gconstpointer string2,
    G_GNUC_UNUSED gpointer user_data
) {
  const char *first = string1;
  const char *second = string2;
  return g_strcmp0(first, second);
}

static GTree *le_bon_mot_engine_dictionary_init(UCollator *collator) {
  GTree *dictionary = g_tree_new_full(
      le_bon_mot_engine_dictionary_compare_word, NULL,
      g_free, le_bon_mot_engine_dictionary_destroy_value);
  GFile *dictionary_file = g_file_new_for_uri(LE_BON_MOT_ENGINE_DICTIONARY_FILE_URI);
  GError *error = NULL;

  // Open file for read
  GFileInputStream *dictionary_stream = g_file_read(dictionary_file, NULL, &error);
  if (!dictionary_stream) {
    g_error("Error occured while opening dictionary: code: %d, message: %s", error->code, error->message);
    exit(1);
  }

  gssize read;
  gchar buffer[1024];
  GString *word = g_string_new(NULL);

  UChar u_word_buffer[LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char key_buffer[LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char* current_key_buffer = key_buffer;
  uint32_t key_buffer_size = sizeof(key_buffer);
  uint32_t key_buffer_expected_size = 0;

  while(TRUE) {
    read = g_input_stream_read(G_INPUT_STREAM(dictionary_stream), buffer, G_N_ELEMENTS(buffer), NULL, &error);
    if (read > 0) {
      for (guint i = 0; i < read; i += 1)
      {
        if (buffer[i] == '\n') {
          glong word_length = g_utf8_strlen(word->str, -1);
          if (word_length >= LE_BON_MOT_ENGINE_WORD_LENGTH_MIN
              && word_length <= LE_BON_MOT_ENGINE_WORD_LENGTH_MAX) {
            // Prepare value to store
            DictionaryWord *dword = g_new(DictionaryWord, 1);
            dword->is_playable = TRUE;
            dword->word = word;

            // Prepare key
            u_uastrcpy(u_word_buffer, word->str);
            key_buffer_expected_size = ucol_getSortKey(collator, u_word_buffer, -1, key_buffer, key_buffer_size);

            if (key_buffer_expected_size > key_buffer_size) {
              if (current_key_buffer == key_buffer) {
                current_key_buffer = g_new(unsigned char, key_buffer_expected_size);
              } else {
                current_key_buffer = g_realloc_n(current_key_buffer, key_buffer_expected_size, sizeof(unsigned char));
              }

              key_buffer_size = ucol_getSortKey(collator, u_word_buffer, -1, current_key_buffer, key_buffer_expected_size);
            }

            unsigned char* key = g_new(unsigned char, key_buffer_size);
            memcpy(key, key_buffer, key_buffer_size);

            // Store key - value
            g_tree_insert(dictionary, key, dword);

            // Prepare word variable for next value
            word = g_string_new(NULL);
          } else {
            g_string_erase(word, 0, -1);
          }
        } else {
          word = g_string_append_c(word, buffer[i]);
        }
      }
    } else if (read < 0) {
      g_error("Error occured while reading dictionary: code: %d, message: %s", error->code, error->message);
      exit(1);
    } else {
      break;
    }
  }

  if (current_key_buffer != key_buffer) {
    g_free(current_key_buffer);
  }

  g_input_stream_close(G_INPUT_STREAM(dictionary_stream), NULL, NULL);

  return dictionary;
}

static GString *le_bon_mot_engine_word_init(GTree *dictionary) {
  guint offset = g_random_int_range(0, g_tree_nnodes(dictionary));
  guint word_length = g_random_int_range(LE_BON_MOT_ENGINE_WORD_LENGTH_MIN, LE_BON_MOT_ENGINE_WORD_LENGTH_MAX);
  GTreeNode *node = g_tree_node_first(dictionary);
  GString *word = NULL;
  // First lookup for word from random range
  guint i = 0;
  while (node)
  {
    DictionaryWord *dword = g_tree_node_value(node);
    // Lookup first playable value from the random offset
    if (i >= offset && dword->is_playable && g_utf8_strlen(dword->word->str, -1) == word_length) {
      word = dword->word;
      break;
    }
    i++;
    node = g_tree_node_next(node);
  }

  // Second lookup from start (if random has given last value and its not playable)
  if (!word) {
    node = g_tree_node_first(dictionary);
    while (node)
    {
      DictionaryWord *dword = g_tree_node_value(node);
      // Lookup first playable value from the random offset
      if (dword->is_playable && g_utf8_strlen(dword->word->str, -1) == word_length) {
        word = dword->word;
        break;
      }
      node = g_tree_node_next(node);
    }
  }

  // Finally if word is still unknown give up
  if (!word) {
    g_error("Unable to find a word");
    exit(2);
  }

  // Transform the word to only base characters
  UErrorCode status = U_ZERO_ERROR;
  UChar transliterator_id [LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  u_uastrcpy(transliterator_id, "NFD; [:Nonspacing Mark:] Remove; Lower; NFC");
  UTransliterator* transliterator = utrans_openU(
    transliterator_id, -1,
    UTRANS_FORWARD,
    NULL, -1,
    NULL,
    &status);
  if (U_FAILURE(status)) {
    g_error("Unable to open unicode transliterator");
  }

  UChar u_word[LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  u_uastrcpy(u_word, word->str);
  int32_t u_word_limit = u_strlen(u_word);
  utrans_transUChars(transliterator, u_word, NULL, LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE, 0, &u_word_limit, &status);
  if (U_FAILURE(status)) {
    g_error("Unable to transliterate");
  }

  utrans_close(transliterator);

  // Save transliterated word
  char trans_word[LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  u_austrcpy(trans_word, u_word);

  g_string_erase(word, 0, -1);
  g_string_append(word, trans_word);

  return word;
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

  glong word_length = g_utf8_strlen(word->str, -1);

  for (gchar i = 'a'; i <= 'z'; i += 1)
  {
    LeBonMotLetterPrivate *letter = g_new(LeBonMotLetterPrivate, 1);
    letter->letter = i;
    letter->position = g_ptr_array_new_with_free_func(g_free);
    letter->found = 0;
    letter->state = LE_BON_MOT_LETTER_UNKOWN;
    for (glong j = 0; j < word_length; j += 1)
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
  glong word_length = g_utf8_strlen(word->str, -1);
  for (guint rowIndex = 0; rowIndex < LE_BON_MOT_ENGINE_ROWS; rowIndex += 1) {
    GPtrArray *row = g_ptr_array_new_full(word->len, g_free);
    for (glong columnIndex = 0; columnIndex < word_length; columnIndex += 1) {
      LeBonMotLetter *letter = g_new(LeBonMotLetter, 1);
      if (rowIndex == 0 && columnIndex == 0) {
        letter->letter = word->str[0];
      } else {
        letter->letter = LE_BON_MOT_ENGINE_NULL_LETTER;
      }
      letter->state = LE_BON_MOT_LETTER_UNKOWN;
      g_ptr_array_add(row, letter);
    }
    g_ptr_array_add(board, row);
  }
  return board;
}

static gpointer le_bon_mot_engine_board_copy_letter(gconstpointer src, G_GNUC_UNUSED gpointer data)
{
  LeBonMotLetter *copy = g_new(LeBonMotLetter, 1);
  const LeBonMotLetter *letter = src;
  copy->letter = letter->letter;
  copy->state = letter->state;
  return copy;
}

static gpointer le_bon_mot_engine_board_copy_row (gconstpointer src, G_GNUC_UNUSED gpointer data)
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

  if (priv->current_row >= LE_BON_MOT_ENGINE_ROWS || priv->state != LE_BON_MOT_ENGINE_STATE_CONTINUE) {
    return;
  }
  
  GPtrArray* row = g_ptr_array_index(priv->board, priv->current_row);

  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);
    if (letter->letter == LE_BON_MOT_ENGINE_NULL_LETTER) {
      // Ignore input if user write the first letter on second position
      if (col == 1 && newLetter[0] == priv->word->str[0]) {
        break;
      }
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

  if (priv->current_row >= LE_BON_MOT_ENGINE_ROWS  || priv->state != LE_BON_MOT_ENGINE_STATE_CONTINUE) {
    return;
  }
  
  GPtrArray* row = g_ptr_array_index(priv->board, priv->current_row);

  for (guint col = row->len - 1; col <= row->len; col -= 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);
    if (letter->letter != LE_BON_MOT_ENGINE_NULL_LETTER && col != 0) {
      letter->letter = LE_BON_MOT_ENGINE_NULL_LETTER;
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

void le_bon_mot_engine_validate(LeBonMotEngine *self, GError **error) {
  g_return_if_fail(LE_BON_MOT_IS_ENGINE(self));
  g_return_if_fail (error == NULL || *error == NULL);

  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);
  
  GPtrArray *row = g_ptr_array_index(priv->board, priv->current_row);
  GString *word = g_string_new(NULL);
 
  if (priv->current_row >= LE_BON_MOT_ENGINE_ROWS  || priv->state != LE_BON_MOT_ENGINE_STATE_CONTINUE) {
    return;
  }

  // Ensure all letters were given
  for (guint col = 0; col < row->len; col += 1) {
    LeBonMotLetter *letter = g_ptr_array_index(row, col);
    g_string_append_c(word, letter->letter);

    if (letter->letter == LE_BON_MOT_ENGINE_NULL_LETTER) {
      g_string_free(word, TRUE);
      g_set_error_literal(
          error, LE_BON_MOT_ENGINE_ERROR,
          LE_BON_MOT_ENGINE_ERROR_LINE_INCOMPLETE,
          "You must fill all letters.");
      return;
    }
  }

  // Check if the given word exists in dictionary
 
  // Compute u_word collapse key
  UChar u_word_buffer[LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char key_buffer[LE_BON_MOT_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char* current_key_buffer = key_buffer;
  uint32_t key_buffer_size = sizeof(key_buffer);
  uint32_t key_buffer_expected_size = 0;

  u_uastrcpy(u_word_buffer, word->str);
  key_buffer_expected_size = ucol_getSortKey(priv->collator, u_word_buffer, -1, key_buffer, key_buffer_size);

  if (key_buffer_expected_size > key_buffer_size) {
    current_key_buffer = g_new(unsigned char, key_buffer_expected_size);
    key_buffer_size = ucol_getSortKey(priv->collator, u_word_buffer, -1, current_key_buffer, key_buffer_expected_size);
  }

  gboolean word_exists = g_tree_lookup(priv->dictionary, current_key_buffer) != NULL;

  if (current_key_buffer != key_buffer) {
    g_free(current_key_buffer);
  }
  g_string_free(word, TRUE);

  if(!word_exists) {
    g_set_error_literal(
        error, LE_BON_MOT_ENGINE_ERROR,
        LE_BON_MOT_ENGINE_ERROR_WORD_UNKOWN,
        "This word doesn't exist in our dictionary.");
    return;
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
    priv->state = LE_BON_MOT_ENGINE_STATE_WON;
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
  } else {
    // Cannot play anymore game is lost
    priv->state = LE_BON_MOT_ENGINE_STATE_LOST;
  }
}

guint le_bon_mot_engine_get_current_row (LeBonMotEngine *self) {
  g_return_val_if_fail(LE_BON_MOT_IS_ENGINE(self), -1);
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);
  return priv->current_row;
}

LeBonMotEngineState le_bon_mot_engine_get_game_state(LeBonMotEngine *self) {
  g_return_val_if_fail(LE_BON_MOT_IS_ENGINE(self), -1);
  LeBonMotEnginePrivate *priv = le_bon_mot_engine_get_instance_private(self);
  return priv->state;
}
