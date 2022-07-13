/* muttum-attempt.h
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

#pragma once

#include <glib-2.0/glib.h>
#include <glib-object.h>

#include "muttum-letter.h"

G_BEGIN_DECLS

#define MUTTUM_TYPE_ATTEMPT (muttum_attempt_get_type())

G_DECLARE_FINAL_TYPE(MuttumAttempt, muttum_attempt, MUTTUM, ATTEMPT, GObject);

/*
 * Public structures
 */

typedef struct {
  MuttumLetter *letter;
  guint index;
} MuttumAttemptLetter;

/*
 * Public method definitions.
 */

void muttum_attempt_foreach_letter(MuttumAttempt* self, GFunc func, gpointer user_data);

G_END_DECLS
