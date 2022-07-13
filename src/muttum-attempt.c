/* muttum-attempt.c
 *
 * Copyright 2022 Adrien Dorsaz <adrien@adorsaz.ch>
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

#include "muttum-attempt-private.h"

typedef enum { PROP_N_LETTERS = 1, N_PROPERTIES } MuttumAttemptProperty;

static GParamSpec *properties[N_PROPERTIES] = {
    NULL,
};

struct _MuttumAttempt {
  GObject parent_instance;

  // Attempt properties
  GPtrArray *data;
  guint n_letters;
};

G_DEFINE_TYPE(MuttumAttempt, muttum_attempt, G_TYPE_OBJECT);

static void muttum_attempt_get_property(GObject *object, guint property_id,
                                        GValue *value, GParamSpec *pspec) {
  MuttumAttempt *self = MUTTUM_ATTEMPT(object);

  switch ((MuttumAttemptProperty)property_id) {
  case PROP_N_LETTERS:
    g_value_set_uint(value, self->n_letters);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void muttum_attempt_set_property(GObject *object, guint property_id,
                                        const GValue *value,
                                        GParamSpec *pspec) {
  MuttumAttempt *self = MUTTUM_ATTEMPT(object);

  switch ((MuttumAttemptProperty)property_id) {
  case PROP_N_LETTERS:
    self->n_letters = g_value_get_uint(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    break;
  }
}

static void muttum_attempt_constructed(GObject *obj) {
  MuttumAttempt *self = MUTTUM_ATTEMPT(obj);

  // We can fill data as we have all construction properties available
  for (glong letter_index = 0; letter_index < self->n_letters;
       letter_index += 1) {
    MuttumLetter *letter = g_new(MuttumLetter, 1);
    letter->letter = MUTTUM_LETTER_NULL;
    letter->state = MUTTUM_LETTER_UNKOWN;
    g_ptr_array_add(self->data, letter);
  }

  G_OBJECT_CLASS(muttum_attempt_parent_class)->constructed(obj);
}

static void muttum_attempt_class_init(MuttumAttemptClass *klass) {
  GObjectClass *object_class = G_OBJECT_CLASS(klass);

  object_class->set_property = muttum_attempt_set_property;
  object_class->get_property = muttum_attempt_get_property;

  object_class->constructed = muttum_attempt_constructed;

  /**
   * MuttumAttempt:n-letters:
   *
   * Number of letters contained in the word to guess.
   **/
  properties[PROP_N_LETTERS] = g_param_spec_uint(
      "n-letters", NULL, "Number of letters of the word to guess",
      1,           // minimum
      G_MAXUINT32, // maximum
      2,           // default value
      G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

  g_object_class_install_properties(object_class, N_PROPERTIES, properties);
}

static void muttum_attempt_init(MuttumAttempt *self) {
  self->data = g_ptr_array_new_with_free_func(g_free);
}

/**
 * muttum_attempt_new:
 * @n_letters: Number of letters contained in the word to guess.
 *
 * Creates a #MuttumAttempt to be used by a #MuttumBoard to store the @n_letters
 * given by the user.
 *
 * Returns: (transfer full): a new #MuttumAttempt
 */
MuttumAttempt *muttum_attempt_new(guint n_letters) {
  GValue v_n_letters = G_VALUE_INIT;

  g_value_init(&v_n_letters, G_TYPE_UINT);

  g_value_set_uint(&v_n_letters, n_letters);

  const char *properties[] = {"n-letters"};
  const GValue values[] = {v_n_letters};
  MuttumAttempt *attempt = (MuttumAttempt *)g_object_new_with_properties(
      MUTTUM_TYPE_ATTEMPT, G_N_ELEMENTS(properties), properties, values);

  g_value_unset(&v_n_letters);

  return attempt;
}

/**
 * muttum_attempt_add_letter:
 * @letter: A letter to add. The letter must be available in the alphabet.
 *
 * Add a letter to the attempt. The addition is effective only if an empty
 * cell exists. Adding first letter again on second position is ignored.
 */
void muttum_attempt_add_letter(MuttumAttempt *self, const char letter) {
  g_return_if_fail(MUTTUM_IS_ATTEMPT(self));

  char first_letter;
  for (guint letter_index = 0; letter_index < self->data->len;
       letter_index += 1) {
    MuttumLetter *attempt_letter = g_ptr_array_index(self->data, letter_index);
    if (letter_index == 0) {
      first_letter = attempt_letter->letter;
    }

    if (attempt_letter->letter == MUTTUM_LETTER_NULL) {
      // Ignore input if try to add the first letter on second position
      if (letter_index == 1 && letter == first_letter) {
        break;
      }
      attempt_letter->letter = letter;
      attempt_letter->state = MUTTUM_LETTER_UNKOWN;
      break;
    }
  }
}

/**
 * muttum_attempt_remove_letter:
 *
 * Remove last letter from the attempt. Removing first letter of the attempt is
 * ignored.
 */
void muttum_attempt_remove_letter(MuttumAttempt *self) {
  g_return_if_fail(MUTTUM_IS_ATTEMPT(self));

  for (guint letter_index = self->data->len - 1;
       letter_index <= self->data->len; letter_index -= 1) {
    MuttumLetter *letter = g_ptr_array_index(self->data, letter_index);
    if (letter->letter != MUTTUM_LETTER_NULL && letter_index != 0) {
      letter->letter = MUTTUM_LETTER_NULL;
      letter->state = MUTTUM_LETTER_UNKOWN;
      break;
    }
  }
}

/**
 * muttum_attempt_foreach_letter:
 * @func: (scope call): The callback to execute on each letter.
 * @user_data: (closure): Data to pass to the callback.
 *
 * Call a function for each letter contained in the word attempt.
 * Letters are processed in the order of the word.
 */
void muttum_attempt_foreach_letter(MuttumAttempt *self, GFunc func,
                                    gpointer user_data) {
  g_return_if_fail(MUTTUM_IS_ATTEMPT(self));

  for (guint letter_index = 0;
       letter_index < self->data->len; letter_index += 1) {
    MuttumLetter *letter = g_ptr_array_index(self->data, letter_index);

    MuttumAttemptLetter *attempt_letter = g_new(MuttumAttemptLetter, 1);
    attempt_letter->letter = muttum_letter_copy(letter, NULL);
    attempt_letter->index = letter_index;

    func(attempt_letter, user_data);

    g_free(attempt_letter->letter);
    g_free(attempt_letter);
  }
}

/**
 * muttum_attempt_update_foreach_letter:
 *
 * Call a function for each letter contained in the word attempt.
 * Letters are processed in the order of the word.
 * Letter data can be updated.
 */
void muttum_attempt_update_foreach_letter(MuttumAttempt* self, GFunc func, gpointer user_data) {
  g_return_if_fail(MUTTUM_IS_ATTEMPT(self));

  for (guint letter_index = 0;
       letter_index < self->data->len; letter_index += 1) {
    MuttumLetter *letter = g_ptr_array_index(self->data, letter_index);

    MuttumAttemptLetter *attempt_letter = g_new(MuttumAttemptLetter, 1);
    attempt_letter->letter = letter;
    attempt_letter->index = letter_index;

    func(attempt_letter, user_data);

    g_free(attempt_letter);
  }
}

/**
 * muttum_attempt_get_word:
 *
 * Returns: (element-type GString*) (transfer full): A GString with all filled
 * letters. If not any letter where given, the returned string will be empty.
 */
GString* muttum_attempt_get_filled_letters(MuttumAttempt *self) {
  GString *word = g_string_new(NULL);
  for (guint letter_index = 0; letter_index < self->data->len;
       letter_index += 1) {
    MuttumLetter *letter = g_ptr_array_index(self->data, letter_index);

    // If a "null" letter is found, all next ones will be null too
    if (letter->letter == MUTTUM_LETTER_NULL) {
      break;
    }

    g_string_append_c(word, letter->letter);
  }
  return word;
}
