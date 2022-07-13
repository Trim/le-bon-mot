/* muttum-board-private.h
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

#include "muttum-board.h"
#include "muttum-attempt-private.h"

/*
 * Private Method Definition
 * */

MuttumBoard *muttum_board_new(guint n_attempts, guint n_letters);

MuttumAttempt *muttum_board_get_attempt(MuttumBoard *board,
                                        const guint attempt_number);

void muttum_board_add_letter(MuttumBoard *board, const guint attempt_number,
                             const char letter);

void muttum_board_remove_letter(MuttumBoard *board, const guint attempt_number);
