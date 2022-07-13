/* muttum-attempt-private.h
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

#include "muttum-attempt.h"

MuttumAttempt *muttum_attempt_new(guint n_letters);
void muttum_attempt_add_letter(MuttumAttempt *self, const char letter);
void muttum_attempt_remove_letter(MuttumAttempt *self);
void muttum_attempt_update_foreach_letter(MuttumAttempt* self, GFunc func, gpointer user_data);
GString* muttum_attempt_get_filled_letters(MuttumAttempt *self);
