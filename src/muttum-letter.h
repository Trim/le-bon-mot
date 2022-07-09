/* muttum-letter.h
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

#pragma once

#include <glib-2.0/glib.h>

static const gchar MUTTUM_LETTER_NULL = '.';

/**
 * MuttumLetterState:
 *
 * State of a letter inside the engine
 */
typedef enum {
  MUTTUM_LETTER_UNKOWN,
  MUTTUM_LETTER_NOT_PRESENT,
  MUTTUM_LETTER_PRESENT,
  MUTTUM_LETTER_WELL_PLACED,
} MuttumLetterState;

/**
 * MuttumLetter:
 *
 * Structure containing a letter symbol and its state
 */
typedef struct {
  gchar letter;
  MuttumLetterState state;
} MuttumLetter;
