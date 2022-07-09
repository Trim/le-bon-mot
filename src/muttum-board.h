/* muttum-board.h
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
#include <glib-object.h>

#include "muttum-letter.h"

G_BEGIN_DECLS

#define MUTTUM_TYPE_BOARD (muttum_board_get_type())

G_DECLARE_FINAL_TYPE(MuttumBoard, muttum_board, MUTTUM, BOARD, GObject);

/*
 * Public method definitions.
 * */

MuttumBoard *muttum_board_new(guint n_attempts, guint n_letters,
                              const char first_letter);

G_END_DECLS
