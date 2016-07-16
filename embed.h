/*
 * Copyright (C) 2016 173210 <root.3.173210@live.com>
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

#ifndef EMBED_H
#define EMBED_H

#include "atom.h"

enum embed_id {
	EMBED_NIL,
	EMBED_T,
	EMBED_COPYING,
	EMBED_ADD,
	EMBED_EQ,
	EMBED_CAR,
	EMBED_CDR,
	EMBED_NTH,
	EMBED_CONS,
	EMBED_ATOM,
	EMBED_EVAL,
	EMBED_DEFINE,
	EMBED_LENGTH,
	EMBED_POSITION,
	EMBED_IF,
	EMBED_LAMBDA,
	EMBED_QUOTE,

	EMBED_MAX
};

extern const struct embed embed_tbl[EMBED_MAX];

#endif
