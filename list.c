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

#include <stddef.h>
#include "lisp_errno.h"
#include "list.h"

int list_position(const union atom *item, const struct cons *list)
{
	int n;

	n = 0;
	while (list->common.type != ATOM_TYPE_NIL) {
		if (atom_eq(list->reg[CONS_CAR], item))
			return n;

		list = &list->reg[CONS_CDR]->cons;
		n++;
	}

	return LISP_EINVAL_ATOM_TYPE;
}

int list_length(const struct cons *p)
{
	int i;

	i = 0;
	while (p->common.type != ATOM_TYPE_NIL) {
		if (p->common.type != ATOM_TYPE_CONS)
			return LISP_EINVAL_ATOM_TYPE;

		i++;
		p = &p->reg[CONS_CDR]->cons;
	}

	return i;
}

union atom *list_nth(unsigned int n, const struct cons *p)
{
	while (true) {
		if (p->common.type != ATOM_TYPE_CONS)
			return NULL;

		if (n <= 0)
			break;

		p = &p->reg[CONS_CDR]->cons;
		n--;
	}

	return p->reg[CONS_CAR];
}
