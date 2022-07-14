/* muttum-engine.c
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

#define GETTEXT_PACKAGE "muttum"
#include <glib/gi18n.h>

#include "muttum-engine.h"
#include "muttum-attempt-private.h"
#include "muttum-board-private.h"

// Default French dictionary path uri if not defined
#ifndef FRENCH_DICTIONARY_PATH_URI
  #define FRENCH_DICTIONARY_PATH_URI "file:///use/share/dict/french"
#endif

const guint MUTTUM_ENGINE_N_ATTEMPTS = 6;
const guint MUTTUM_ENGINE_WORD_LENGTH_MIN = 5;
const guint MUTTUM_ENGINE_WORD_LENGTH_MAX = 8;
const guint MUTTUM_ENGINE_UCHAR_BUFFER_SIZE = 100;

// Muttum is currently only developped for French users
const gchar *MUTTUM_ENGINE_COLLATION = "fr_FR";
const gchar *MUTTUM_ENGINE_DICTIONARY_FILE_URI = FRENCH_DICTIONARY_PATH_URI;

static GTree *muttum_engine_class_dictionary_init(UCollator *collator);
static void muttum_engine_word_init(MuttumEngine* self);
static GPtrArray *muttum_engine_alphabet_init(GString *word);

G_DEFINE_QUARK(muttum-engine-error-quark, muttum_engine_error);

typedef struct {
  gchar letter;
  MuttumLetterState state;
  guint found;
  GPtrArray *position;
} MuttumLetterPrivate;

typedef struct {
  GString* word;
  gboolean is_playable;
} DictionaryWord;

typedef struct {
  MuttumEngine *engine;
  guint *n_well_placed_letters;
} UserDataValidate;

struct _MuttumEngine {
  GObject parent_instance;

  // Engine properties
  GString *word;
  guint n_letters;
  GString *dictionary_word;
  GPtrArray *alphabet;
  MuttumBoard *board;
  guint current_attempt;
  MuttumEngineState state;
};

struct _MuttumEngineClass {
  GObjectClass parent_class;

  // Class Members
  GTree *dictionary;
  UCollator *collator;
};

G_DEFINE_TYPE(MuttumEngine, muttum_engine, G_TYPE_OBJECT);

static void
muttum_engine_dispose (GObject *gobject)
{
  MUTTUM_IS_ENGINE(gobject);
  MuttumEngine *self = MUTTUM_ENGINE(gobject);

  g_ptr_array_unref(self->alphabet);
  g_clear_object(&self->board);

  G_OBJECT_CLASS (muttum_engine_parent_class)->dispose (gobject);
}

static void
muttum_engine_finalize (GObject *gobject)
{
  MUTTUM_IS_ENGINE(gobject);
  MuttumEngine *self = MUTTUM_ENGINE(gobject);

  g_string_free(self->word, TRUE);
  g_string_free(self->dictionary_word, TRUE);

  G_OBJECT_CLASS (muttum_engine_parent_class)->finalize (gobject);
}

static void
muttum_engine_class_init(MuttumEngineClass *klass) {
  GObjectClass *g_object_class = G_OBJECT_CLASS(klass);

  // Override methods
  g_object_class->dispose = muttum_engine_dispose;
  g_object_class->finalize = muttum_engine_finalize;

  // Setup class members

  // Unicode collator give more tools to create dictionary
  UErrorCode status = U_ZERO_ERROR;
  klass->collator = ucol_open(MUTTUM_ENGINE_COLLATION, &status);

  if (U_FAILURE(status)) {
    g_error("Unable to open unicode collator");
  }

  // Primary strength allow to work with only base characters
  // (neither case sensitive, nor accent sensitive)
  ucol_setStrength(klass->collator, UCOL_PRIMARY);

  // Read the dictionary file once
  klass->dictionary = muttum_engine_class_dictionary_init(klass->collator);
}

static void
muttum_engine_init(MuttumEngine *self) {
  muttum_engine_word_init(self);
  self->n_letters = g_utf8_strlen(self->word->str, -1);
  self->alphabet = muttum_engine_alphabet_init(self->word);
  self->board = muttum_board_new(MUTTUM_ENGINE_N_ATTEMPTS, self->n_letters);
  self->current_attempt = 1;
  self->state = MUTTUM_ENGINE_STATE_CONTINUE;

  // First letter is mandatory and cannot be changed
  muttum_board_add_letter(self->board, self->current_attempt,
                          self->word->str[0]);
}

static void muttum_engine_dictionary_destroy_value(gpointer data) {
  DictionaryWord* dword = data;
  g_string_free(dword->word, TRUE);
  g_free(dword);
}

static gint
muttum_engine_dictionary_compare_word (
    gconstpointer string1,
    gconstpointer string2,
    G_GNUC_UNUSED gpointer user_data
) {
  const char *first = string1;
  const char *second = string2;
  return g_strcmp0(first, second);
}

static GTree *muttum_engine_class_dictionary_init(UCollator *collator) {
  GTree *dictionary = g_tree_new_full(
      muttum_engine_dictionary_compare_word, NULL,
      g_free, muttum_engine_dictionary_destroy_value);
  GFile *dictionary_file = g_file_new_for_uri(MUTTUM_ENGINE_DICTIONARY_FILE_URI);
  GError *error = NULL;

  // Open file for read
  GFileInputStream *dictionary_stream = g_file_read(dictionary_file, NULL, &error);
  if (!dictionary_stream) {
    g_error("Error occured while opening dictionary: code: %d, message: %s", error->code, error->message);
  }

  gssize read;
  gchar buffer[1024];
  GString *word = g_string_new(NULL);

  UChar u_word_buffer[MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char key_buffer[MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
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
          if (word_length >= MUTTUM_ENGINE_WORD_LENGTH_MIN
              && word_length <= MUTTUM_ENGINE_WORD_LENGTH_MAX) {
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

static void muttum_engine_word_init(MuttumEngine* self) {
  MuttumEngineClass* klass = MUTTUM_ENGINE_GET_CLASS(self);
  guint offset = g_random_int_range(0, g_tree_nnodes(klass->dictionary));
  guint word_length = g_random_int_range(MUTTUM_ENGINE_WORD_LENGTH_MIN, MUTTUM_ENGINE_WORD_LENGTH_MAX);
  GTreeNode *node = g_tree_node_first(klass->dictionary);
  GString *word = NULL;
  // First lookup for word from random range
  guint i = 0;
  while (node)
  {
    DictionaryWord *dword = g_tree_node_value(node);
    // Lookup first playable value from the random offset
    if (i >= offset && dword->is_playable && g_utf8_strlen(dword->word->str, -1) == word_length) {
      word = g_string_new(dword->word->str);
      break;
    }
    i++;
    node = g_tree_node_next(node);
  }

  // Second lookup from start (if random has given last value and its not playable)
  if (!word) {
    node = g_tree_node_first(klass->dictionary);
    while (node)
    {
      DictionaryWord *dword = g_tree_node_value(node);
      // Lookup first playable value from the random offset
      if (dword->is_playable && g_utf8_strlen(dword->word->str, -1) == word_length) {
        word = g_string_new(dword->word->str);
        break;
      }
      node = g_tree_node_next(node);
    }
  }

  // Finally if word is still unknown give up
  if (!word) {
    g_error("Unable to find a word");
  }

#ifdef MUTTUM_ENGINE_FORCE_WORD
  g_string_erase(word, 0, -1);
  g_string_append(word, MUTTUM_ENGINE_FORCE_WORD);
#endif

  // Save the word from dictionary to display it in case of loose
  self->dictionary_word = g_string_new(word->str);

  // Transform the word to only base characters
  UErrorCode status = U_ZERO_ERROR;
  UChar transliterator_id [MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
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

  UChar u_word[MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
  u_uastrcpy(u_word, word->str);
  int32_t u_word_limit = u_strlen(u_word);
  utrans_transUChars(transliterator, u_word, NULL, MUTTUM_ENGINE_UCHAR_BUFFER_SIZE, 0, &u_word_limit, &status);
  if (U_FAILURE(status)) {
    g_error("Unable to transliterate");
  }

  utrans_close(transliterator);

  // Save transliterated word
  char trans_word[MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
  u_austrcpy(trans_word, u_word);

  g_string_erase(word, 0, -1);
  g_string_append(word, trans_word);

  self->word = word;
}

static void muttum_engine_letter_private_free(gpointer data) {
  MuttumLetterPrivate* letter = data;
  g_ptr_array_unref(letter->position);
  g_free(letter);
}

static GPtrArray *muttum_engine_alphabet_init(GString *word)
{
  GPtrArray *alphabet = g_ptr_array_new_full(
      26,
      muttum_engine_letter_private_free
  );

  glong word_length = g_utf8_strlen(word->str, -1);

  for (gchar i = 'a'; i <= 'z'; i += 1)
  {
    MuttumLetterPrivate *letter = g_new(MuttumLetterPrivate, 1);
    letter->letter = i;
    letter->position = g_ptr_array_new_with_free_func(g_free);
    letter->found = 0;
    letter->state = MUTTUM_LETTER_UNKOWN;
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

/**
 * muttum_engine_get_board_state:
 *
 * Returns: (transfer full): the current #MuttumBoard
 * (rows and columns) of MuttumLetter
 */
MuttumBoard *muttum_engine_get_board_state(MuttumEngine *self) {
  g_return_val_if_fail(MUTTUM_IS_ENGINE(self), NULL);

  return self->board;
}

/**
 * muttum_engine_get_alphabet_state:
 *
 * Returns: (type GPtrArray(MuttumLetter)) (transfer full): the current alphabet state for this engine
 */
GPtrArray* muttum_engine_get_alphabet_state (MuttumEngine *self) {
  g_return_val_if_fail(MUTTUM_IS_ENGINE(self), NULL);

  return g_ptr_array_copy(self->alphabet, muttum_letter_copy, NULL);
}

/**
 * muttum_engine_add_letter:
 * @letter: A letter to add. The letter must be available in the alphabet.
 *
 * Action to call when player wants to add a new letter on the current attempt.
 */
void muttum_engine_add_letter (MuttumEngine *self, const char letter)
{
  g_return_if_fail(MUTTUM_IS_ENGINE(self));

  if (self->current_attempt > MUTTUM_ENGINE_N_ATTEMPTS || self->state != MUTTUM_ENGINE_STATE_CONTINUE) {
    return;
  }

  muttum_board_add_letter(self->board, self->current_attempt, letter);
}

/**
 * muttum_engine_remove_letter:
 *
 * Action to call when player wants to remove a letter on the current attempt.
 */
void muttum_engine_remove_letter (MuttumEngine *self)
{
  g_return_if_fail(MUTTUM_IS_ENGINE(self));

  if (self->current_attempt > MUTTUM_ENGINE_N_ATTEMPTS  || self->state != MUTTUM_ENGINE_STATE_CONTINUE) {
    return;
  }

  muttum_board_remove_letter(self->board, self->current_attempt);
}

static gboolean
muttum_engine_compare_letter_private_with_letter (
    gconstpointer letter_private,
    gconstpointer letter
) {
  const MuttumLetterPrivate *first = letter_private;
  const MuttumLetter *second = letter;
  return first->letter == second->letter;
}

/**
 * muttum_engine_validate_find_well_placed_letters:
 *
 * This is a callback to be used while validating attempt.
 * This finds all well placed letters and update the alphabet of the
 *#MuttumEngine.
 **/
static void
muttum_engine_validate_find_well_placed_letters(gpointer data,
                                                gpointer user_data) {
  g_return_if_fail(MUTTUM_IS_ENGINE(((UserDataValidate *)user_data)->engine));
  MuttumEngine *self = ((UserDataValidate *)user_data)->engine;
  guint *well_placed = ((UserDataValidate *)user_data)->n_well_placed_letters;

  MuttumAttemptLetter *attempt_letter = (MuttumAttemptLetter *)data;
  MuttumLetter *letter = attempt_letter->letter;
  guint letter_index = attempt_letter->index;

  // Alphabet is used to store found letter count
  guint *alphabet_index = g_new(guint, 1);
  MuttumLetterPrivate *alphabet = NULL;
  if (g_ptr_array_find_with_equal_func(
          self->alphabet, letter,
          muttum_engine_compare_letter_private_with_letter, alphabet_index)) {
    alphabet = g_ptr_array_index(self->alphabet, *alphabet_index);
  }
  g_free(alphabet_index);
  g_return_if_fail(alphabet);

  if (self->word->str[letter_index] == letter->letter) {
    letter->state = MUTTUM_LETTER_WELL_PLACED;
    alphabet->state = MUTTUM_LETTER_WELL_PLACED;
    alphabet->found++;
    (*well_placed)++;
  }
}

static void muttum_engine_validate_find_present_letters(gpointer data,
                                                        gpointer user_data) {
  g_return_if_fail(MUTTUM_IS_ENGINE(user_data));
  MuttumEngine *self = (MuttumEngine *)user_data;

  MuttumAttemptLetter *attempt_letter = (MuttumAttemptLetter *)data;
  MuttumLetter *letter = attempt_letter->letter;
  guint letter_index = attempt_letter->index;

  // Alphabet is used to store found letter count
  guint *alphabet_index = g_new(guint, 1);
  MuttumLetterPrivate *alphabet = NULL;
  if (g_ptr_array_find_with_equal_func(
          self->alphabet, letter,
          muttum_engine_compare_letter_private_with_letter, alphabet_index)) {
    alphabet = g_ptr_array_index(self->alphabet, *alphabet_index);
  }
  g_free(alphabet_index);
  g_return_if_fail(alphabet);

  if (self->word->str[letter_index] != letter->letter) {
    if (alphabet->found < alphabet->position->len) {
      letter->state = MUTTUM_LETTER_PRESENT;
      if (alphabet->state != MUTTUM_LETTER_WELL_PLACED) {
        alphabet->state = MUTTUM_LETTER_PRESENT;
      }
      alphabet->found++;
    } else {
      letter->state = MUTTUM_LETTER_NOT_PRESENT;
      if (alphabet->state == MUTTUM_LETTER_UNKOWN) {
        alphabet->state = MUTTUM_LETTER_NOT_PRESENT;
      }
    }
  }
}
/**
 * muttum_engine_validate:
 *
 * Action to call when a player wants to validate the word on the current attempt.
 *
 * This function can return `MuttumEngineError` if the word was invalid.
 *
 * If the word was valid, it updates states of the game, alphabet and board.
 */
void muttum_engine_validate(MuttumEngine *self, GError **error) {
  g_return_if_fail(MUTTUM_IS_ENGINE(self));
  g_return_if_fail (error == NULL || *error == NULL);

  MuttumEngineClass* klass = MUTTUM_ENGINE_GET_CLASS(self);

  MuttumAttempt *attempt =
      muttum_board_get_attempt(self->board, self->current_attempt);
  g_return_if_fail(attempt);

  if (self->current_attempt > MUTTUM_ENGINE_N_ATTEMPTS  || self->state != MUTTUM_ENGINE_STATE_CONTINUE) {
    return;
  }

  GString *word = muttum_attempt_get_filled_letters(attempt);

  // Ensure all letters were given
  //
  if (self->n_letters != g_utf8_strlen(word->str, -1)) {
    g_string_free(word, TRUE);
    g_set_error_literal(error, MUTTUM_ENGINE_ERROR,
                        MUTTUM_ENGINE_ERROR_LINE_INCOMPLETE,
                        _("You must fill all letters."));
    return;
  }

  // Check if the given word exists in dictionary

  // Compute u_word collapse key
  UChar u_word_buffer[MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char key_buffer[MUTTUM_ENGINE_UCHAR_BUFFER_SIZE];
  unsigned char* current_key_buffer = key_buffer;
  uint32_t key_buffer_size = sizeof(key_buffer);
  uint32_t key_buffer_expected_size = 0;

  u_uastrcpy(u_word_buffer, word->str);
  key_buffer_expected_size = ucol_getSortKey(klass->collator, u_word_buffer, -1, key_buffer, key_buffer_size);

  if (key_buffer_expected_size > key_buffer_size) {
    current_key_buffer = g_new(unsigned char, key_buffer_expected_size);
    key_buffer_size = ucol_getSortKey(klass->collator, u_word_buffer, -1, current_key_buffer, key_buffer_expected_size);
  }

  gboolean word_exists = g_tree_lookup(klass->dictionary, current_key_buffer) != NULL;

  if (current_key_buffer != key_buffer) {
    g_free(current_key_buffer);
  }
  g_string_free(word, TRUE);

  if(!word_exists) {
    g_set_error_literal(
        error, MUTTUM_ENGINE_ERROR,
        MUTTUM_ENGINE_ERROR_WORD_UNKOWN,
        _("This word doesn't exist in our dictionary."));
    return;
  }

  // Reset alphabet found
  for (guint i = 0; i < self->alphabet->len; i += 1) {
    MuttumLetterPrivate *alphabet = g_ptr_array_index(self->alphabet, i);
    alphabet->found = 0;
  }

  // Validate state for each letter on current attempt

  // First pass find all well placed letters
  guint *well_placed = g_new(guint, 1);
  *well_placed = 0;

  UserDataValidate *user_data = g_new(UserDataValidate, 1);
  user_data->engine = self;
  user_data->n_well_placed_letters = well_placed;

  muttum_attempt_update_foreach_letter(
      attempt, muttum_engine_validate_find_well_placed_letters, user_data);

  guint well_placed_letters =
      *well_placed; /* Copy well placed count in a local variable to free
                       pointers used by the loop */
  g_free(well_placed);
  g_free(user_data);

  if (well_placed_letters == self->n_letters) {
    self->state = MUTTUM_ENGINE_STATE_WON;
    return;
  }

  // Second pass find all letters present but not well placed
  muttum_attempt_update_foreach_letter(
      attempt, muttum_engine_validate_find_present_letters, self);

  // Move to next attempt
  self->current_attempt++;

  if (self->current_attempt <= MUTTUM_ENGINE_N_ATTEMPTS) {
    muttum_board_add_letter(self->board, self->current_attempt, self->word->str[0]);
  } else {
    // Cannot play anymore game is lost
    self->state = MUTTUM_ENGINE_STATE_LOST;
  }
}

/**
 * muttum_engine_get_current_attempt:
 *
 * Return value: the attempt number currently played.
 */
guint muttum_engine_get_current_attempt (MuttumEngine *self) {
  g_return_val_if_fail(MUTTUM_IS_ENGINE(self), -1);
  return self->current_attempt;
}

/**
 * muttum_engine_get_game_state:
 *
 * Return value: The state of the current game represented by
 * #MuttumEngineState.
 */
MuttumEngineState muttum_engine_get_game_state(MuttumEngine *self) {
  g_return_val_if_fail(MUTTUM_IS_ENGINE(self), -1);
  return self->state;
}

/**
 * muttum_engine_get_word:
 *
 * Returns: (transfer full): The word the player should find by itself.
 */
GString *muttum_engine_get_word(MuttumEngine *self) {
  g_return_val_if_fail(MUTTUM_IS_ENGINE(self), NULL);
  return g_string_new(self->dictionary_word->str);
}
