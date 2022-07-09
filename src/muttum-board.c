/* muttum-board.c
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

#include "muttum-board-private.h"

typedef enum {
  PROP_N_ATTEMPTS = 1,
  PROP_N_LETTERS,
  PROP_FIRST_LETTER,
  N_PROPERTIES
} MuttumBoardProperty;

static GParamSpec *properties[N_PROPERTIES] = {
    NULL,
};

struct _MuttumBoard {
  GObject parent_instance;

  // Board properties
  GPtrArray *data;
  guint n_attempts;
  guint n_letters;
  gchar first_letter;
};

G_DEFINE_TYPE(MuttumBoard, muttum_board, G_TYPE_OBJECT);

static void muttum_board_get_property(GObject *object, guint property_id,
                                      GValue *value, GParamSpec *pspec) {
  MuttumBoard *self = MUTTUM_BOARD(object);

  switch ((MuttumBoardProperty)property_id) {
  case PROP_N_ATTEMPTS:
    g_value_set_uint(value, self->n_attempts);
    break;
  case PROP_N_LETTERS:
    g_value_set_uint(value, self->n_letters);
    break;
  case PROP_FIRST_LETTER:
    g_value_set_schar(value, self->first_letter);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void muttum_board_set_property(GObject *object, guint property_id,
                                      const GValue *value, GParamSpec *pspec) {
  MuttumBoard *self = MUTTUM_BOARD(object);

  switch ((MuttumBoardProperty)property_id) {
  case PROP_N_ATTEMPTS:
    self->n_attempts = g_value_get_uint(value);
    break;
  case PROP_N_LETTERS:
    self->n_letters = g_value_get_uint(value);
    break;
  case PROP_FIRST_LETTER:
    self->first_letter = g_value_get_schar(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void
muttum_board_constructed (GObject *obj)
{
  MuttumBoard *self = MUTTUM_BOARD(obj);

  // We can fill data as we have all construction properties available
  for (guint attempt_index = 0; attempt_index < self->n_attempts;
       attempt_index += 1) {
    GPtrArray *attempt = g_ptr_array_new_full(self->n_letters, g_free);
    for (glong letter_index = 0; letter_index < self->n_letters;
         letter_index += 1) {
      MuttumLetter *letter = g_new(MuttumLetter, 1);
      if (attempt_index == 0 && letter_index == 0) {
        letter->letter = self->first_letter;
      } else {
        letter->letter = MUTTUM_LETTER_NULL;
      }
      letter->state = MUTTUM_LETTER_UNKOWN;
      g_ptr_array_add(attempt, letter);
    }
    g_ptr_array_add(self->data, attempt);
  }

  G_OBJECT_CLASS (muttum_board_parent_class)->constructed (obj);
}

static void muttum_board_class_init(MuttumBoardClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->set_property = muttum_board_set_property;
  object_class->get_property = muttum_board_get_property;

  object_class->constructed = muttum_board_constructed;

  /**
   * MuttumBoard:n-attempts:
   *
   * Number of attempts the user is able to guess the word.
   **/
  properties[PROP_N_ATTEMPTS] = g_param_spec_uint(
      "n-attempts", NULL, "Number of attempts possible with this board",
      1,           // minimum
      G_MAXUINT32, // maximum
      6,           // default value
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  /**
   * MuttumBoard:n-letters:
   *
   * Number of letters contained in the word to guess.
   **/
  properties[PROP_N_LETTERS] = g_param_spec_uint(
      "n-letters", NULL, "Number of letters of the word to guess",
      1,           // minimum
      G_MAXUINT32, // maximum
      2,           // default value
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  /**
   * MuttumBoard:first-letter:
   *
   * First letter of the word to guess
   **/
  properties[PROP_FIRST_LETTER] = g_param_spec_char(
      "first-letter", NULL, "First letter of the word to guess",
      0, // minimum
      G_MAXINT8, // maximum
      0, // default
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void muttum_board_destroy_attempt(gpointer data) {
  g_ptr_array_unref(data);
}

static void muttum_board_init(MuttumBoard *self) {
  self->data = g_ptr_array_new_with_free_func(muttum_board_destroy_attempt);
}

/**
 * muttum_board_new:
 * @n_attempts: Number of attempts the user is able to guess the word.
 * @n_letters: Number of letters contained in the word to guess.
 * @first_letter: First letter of the word to guess
 *
 * Creates a #MuttumBoard to be used by a #MuttumEngine.
 * User will be able to guess all the @n_letters of the word within @n_attempts.
 * The board gives the @first_letter of the word has a hint to the user.
 *
 * Returns: a new #MuttumBoard
 */
MuttumBoard *muttum_board_new(guint n_attempts, guint n_letters,
                              const char first_letter) {
  GValue v_n_attempts = G_VALUE_INIT;
  GValue v_n_letters = G_VALUE_INIT;
  GValue v_first_letter = G_VALUE_INIT;

  g_value_init(&v_n_attempts, G_TYPE_UINT);
  g_value_init(&v_n_letters, G_TYPE_UINT);
  g_value_init(&v_first_letter, G_TYPE_CHAR);

  g_value_set_uint(&v_n_attempts, n_attempts);
  g_value_set_uint(&v_n_letters, n_letters);
  g_value_set_schar(&v_first_letter, first_letter);

  const char *properties[] = {"n-attempts", "n-letters", "first-letter"};
  const GValue values[] = {v_n_attempts, v_n_letters, v_first_letter};
  MuttumBoard *board = (MuttumBoard *)g_object_new_with_properties(
      MUTTUM_TYPE_BOARD, G_N_ELEMENTS(properties), properties, values);

  g_value_unset(&v_n_attempts);
  g_value_unset(&v_n_letters);
  g_value_unset(&v_first_letter);

  return board;
}

static gpointer muttum_board_copy_letter(gconstpointer src,
                                         G_GNUC_UNUSED gpointer data) {
  MuttumLetter *copy = g_new(MuttumLetter, 1);
  const MuttumLetter *letter = src;
  copy->letter = letter->letter;
  copy->state = letter->state;
  return copy;
}

static gpointer muttum_board_copy_attempt(gconstpointer src,
                                          G_GNUC_UNUSED gpointer data) {
  return g_ptr_array_copy((GPtrArray *)src, muttum_board_copy_letter, NULL);
}

/**
 * muttum_board_get_data:
 *
 * Returns: (element-type GPtrArray(MuttumLetter)) (transfer full): the current
 * game board state inside a matrix of GPtrArray (rows and columns) of
 * MuttumLetter
 */
GPtrArray *muttum_board_get_data(MuttumBoard *self) {
  g_return_val_if_fail(MUTTUM_IS_BOARD(self), NULL);

  return g_ptr_array_copy(self->data, muttum_board_copy_attempt, NULL);
}

/**
 * muttum_board_get_attempt:
 * @attempt_number: attempt number to read
 *
 * Returns: (element-type GPtrArray*) (transfer full): an array of MuttumLetter
 * for requested attempt
 */
GPtrArray *muttum_board_get_attempt(MuttumBoard *self,
                                    const guint attempt_number) {
  g_return_val_if_fail(MUTTUM_IS_BOARD(self), NULL);

  // Check bounds before using g_ptr_array_index
  g_assert(attempt_number >= 1);
  g_assert(attempt_number <= self->n_attempts);

  return g_ptr_array_index(self->data, attempt_number - 1);
}

/**
 * muttum_board_add_letter:
 * @attempt_number: attempt number where to add the letter.
 * @letter: A letter to add. The letter must be available in the alphabet.
 */
void muttum_board_add_letter(MuttumBoard *self, const guint attempt_number,
                             const char letter) {
  g_return_if_fail(MUTTUM_IS_BOARD(self));

  // Check bounds before using g_ptr_array_index
  g_assert(attempt_number >= 1);
  g_assert(attempt_number <= self->n_attempts);

  GPtrArray *attempt = g_ptr_array_index(self->data, attempt_number - 1);

  for (guint letter_index = 0; letter_index < attempt->len; letter_index += 1) {
    MuttumLetter *attempt_letter = g_ptr_array_index(attempt, letter_index);
    if (attempt_letter->letter == MUTTUM_LETTER_NULL) {
      // Ignore input if try to add the first letter on second position
      if (letter_index == 1 && letter == self->first_letter) {
        break;
      }
      attempt_letter->letter = letter;
      attempt_letter->state = MUTTUM_LETTER_UNKOWN;
      break;
    }
  }
}

/**
 * muttum_board_remove_letter:
 * @attempt_number: attempt number where to remove a letter.
 */
void muttum_board_remove_letter(MuttumBoard *self, const guint attempt_number) {
  g_return_if_fail(MUTTUM_IS_BOARD(self));

  // Check bounds before using g_ptr_array_index
  g_assert(attempt_number >= 1);
  g_assert(attempt_number <= self->n_attempts);

  GPtrArray *attempt = g_ptr_array_index(self->data, attempt_number - 1);

  for (guint letter_index = attempt->len - 1; letter_index <= attempt->len;
       letter_index -= 1) {
    MuttumLetter *letter = g_ptr_array_index(attempt, letter_index);
    if (letter->letter != MUTTUM_LETTER_NULL && letter_index != 0) {
      letter->letter = MUTTUM_LETTER_NULL;
      letter->state = MUTTUM_LETTER_UNKOWN;
      break;
    }
  }
}
