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

#ifndef ATOM_H
#define ATOM_H

#include <stdbool.h>

#define WORD_SIZE 16

struct sym_list {
	union atom *top;
	union atom **btm;
};

struct sym {
	struct sym_list label;
	struct sym_list form;
};

enum atom_type {
	ATOM_TYPE_NIL,
	ATOM_TYPE_T,
	ATOM_TYPE_NUM,
	ATOM_TYPE_STR,
	ATOM_TYPE_SYM,
	ATOM_TYPE_CONS,
	ATOM_TYPE_FUNC,
	ATOM_TYPE_SPECIAL,
	ATOM_TYPE_LAMBDA,

	ATOM_TYPE_MAX
};

enum atom_st_type {
	ATOM_ST_TYPE_NUM,
	ATOM_ST_TYPE_STR,
	ATOM_ST_TYPE_CONS,
	ATOM_ST_TYPE_EMBED,

	ATOM_ST_TYPE_MAX
};

struct common {
	enum atom_type type;
	unsigned int refcount;
};

struct num {
	struct common common;
	long double v;
};

struct str {
	struct common common;
	char v[WORD_SIZE];
};

enum cons_reg {
	CONS_CAR,
	CONS_CDR,

	CONS_MAX
};

struct cons {
	struct common common;
	union atom *reg[CONS_MAX];
};

struct embed {
	enum atom_type type;
	char str[WORD_SIZE];
	int (* func)(union atom **dst, struct cons *argp, struct sym *);
};

union atom {
	struct common common;
	struct num num;
	struct str str;
	struct cons cons;
	const struct embed embed;
};

int atom_init(union atom * restrict *p, enum atom_type type);
void atom_bool(union atom **p, bool b);
int atom_eval(union atom **dst, union atom *src, struct sym *sym);
void atom_print(const union atom *p);
bool atom_eq(const union atom *p, const union atom *q);
void atom_ref(union atom **dst, union atom *src);
void atom_free(union atom *p);

#endif
