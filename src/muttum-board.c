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
  N_PROPERTIES
} MuttumBoardProperty;

static GParamSpec *properties[N_PROPERTIES] = {
    NULL,
};

struct _MuttumBoard {
  GObject parent_instance;

  // Board properties
  GPtrArray *attempts;
  guint n_attempts;
  guint n_letters;
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
    MuttumAttempt *attempt = muttum_attempt_new(self->n_letters);
    g_ptr_array_add(self->attempts, attempt);
  }

  G_OBJECT_CLASS (muttum_board_parent_class)->constructed (obj);
}

static void
muttum_board_dispose (GObject *obj) {
  MUTTUM_IS_BOARD(obj);
  MuttumBoard *self = MUTTUM_BOARD(obj);

  g_ptr_array_unref(self->attempts);

  G_OBJECT_CLASS(muttum_board_parent_class)->dispose (obj);
}

static void muttum_board_class_init(MuttumBoardClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->set_property = muttum_board_set_property;
  object_class->get_property = muttum_board_get_property;

  object_class->constructed = muttum_board_constructed;

  object_class->dispose = muttum_board_dispose;

  /**
   * MuttumBoard:n-attempts:
   *
   * Number of #MuttumAttempt the user can try to guess the word.
   **/
  properties[PROP_N_ATTEMPTS] = g_param_spec_uint(
      "n-attempts", NULL, "Number of guess attempts possible with this board.",
      1,           // minimum
      G_MAXUINT32, // maximum
      6,           // default value
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  /**
   * MuttumBoard:n-letters:
   *
   * Number of letters in the word to be guessed.
   **/
  properties[PROP_N_LETTERS] = g_param_spec_uint(
      "n-letters", NULL, "Number of letters in the word to be guessed.",
      1,           // minimum
      G_MAXUINT32, // maximum
      2,           // default value
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void muttum_board_init(MuttumBoard *self) {
  self->attempts = g_ptr_array_new_with_free_func(g_object_unref);
}

/**
 * muttum_board_new:
 * @n_attempts: Number of attempts the user is able to guess the word.
 * @n_letters: Number of letters contained in the word to guess.
 *
 * Creates a #MuttumBoard to be used by a #MuttumEngine.
 * User will be able to guess all the @n_letters of the word within @n_attempts.
 *
 * Returns: a new #MuttumBoard
 */
MuttumBoard *muttum_board_new(guint n_attempts, guint n_letters) {
  GValue v_n_attempts = G_VALUE_INIT;
  GValue v_n_letters = G_VALUE_INIT;

  g_value_init(&v_n_attempts, G_TYPE_UINT);
  g_value_init(&v_n_letters, G_TYPE_UINT);

  g_value_set_uint(&v_n_attempts, n_attempts);
  g_value_set_uint(&v_n_letters, n_letters);

  const char *properties[] = {"n-attempts", "n-letters"};
  const GValue values[] = {v_n_attempts, v_n_letters};
  MuttumBoard *board = (MuttumBoard *)g_object_new_with_properties(
      MUTTUM_TYPE_BOARD, G_N_ELEMENTS(properties), properties, values);

  g_value_unset(&v_n_attempts);
  g_value_unset(&v_n_letters);

  return board;
}

/**
 * muttum_board_get_attempt:
 * @attempt_number: #MuttumAttempt number to read
 *
 * Get #MuttumAttempt by number.
 *
 * Returns: (element-type MuttumAttempt*) (transfer full): a #MuttumAttempt
 */
MuttumAttempt *muttum_board_get_attempt(MuttumBoard *self,
                                    const guint attempt_number) {
  g_return_val_if_fail(MUTTUM_IS_BOARD(self), NULL);

  // Check bounds before using g_ptr_array_index
  g_assert(attempt_number >= 1);
  g_assert(attempt_number <= self->n_attempts);

  return g_ptr_array_index(self->attempts, attempt_number - 1);
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

  MuttumAttempt *attempt = g_ptr_array_index(self->attempts, attempt_number - 1);
  muttum_attempt_add_letter(attempt, letter);
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

  MuttumAttempt *attempt = g_ptr_array_index(self->attempts, attempt_number - 1);
  muttum_attempt_remove_letter(attempt);
}

/**
 * muttum_board_for_each_attempt:
 * @func: (scope call): The callback to execute on each attempt.
 * @user_data: (closure): Data to pass to the callback.
 *
 * Call a function for each #MuttumAttempt contained in the #MuttumBoard.
 * Attempts are processed in order they were created.
 */
void muttum_board_for_each_attempt(MuttumBoard *self, GFunc func,
                                   gpointer user_data) {
  g_return_if_fail(MUTTUM_IS_BOARD(self));

  for (guint attempt_index = 0;
       attempt_index < self->attempts->len; attempt_index += 1) {
    MuttumAttempt *attempt = g_ptr_array_index(self->attempts, attempt_index);

    MuttumBoardAttempt *board_attempt = g_new(MuttumBoardAttempt, 1);
    board_attempt->attempt = attempt; /* TODO: use a copy attempt to avoid undesired changes */
    board_attempt->number = attempt_index + 1;

    func(board_attempt, user_data);

    g_free(board_attempt);
  }
}
