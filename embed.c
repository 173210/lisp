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

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "COPYING.h"
#include "embed.h"
#include "lisp_errno.h"
#include "list.h"

static enum lisp_error getargs(
	union atom **dst, const struct cons *argp, unsigned int n)
{
	unsigned int i;

	i = 0;
	for (i = 0; i < n; i++) {
		if (argp->common.type != ATOM_TYPE_CONS)
			return LISP_EINVAL_ATOM_TYPE;

		dst[i] = argp->reg[CONS_CAR];
		argp = &argp->reg[CONS_CDR]->cons;
	}

	return argp->common.type == ATOM_TYPE_NIL ?
		LISP_EOK : LISP_EINVAL_ATOM_TYPE;
}

static int embed_copying(union atom **dst, struct cons *argp, struct sym *sym)
{
	int r;

	r = getargs(NULL, argp, 0);
	if (r != LISP_EOK)
		return r;

	r = fwrite(_binary_COPYING_TXT_start,
		(size_t)_binary_COPYING_TXT_size, 1, stdout);
	if (r < 0)
		return r;

	atom_init(dst, ATOM_TYPE_SYM);
	(*dst)->str.v[0] = 0;

	return 0;
}

static int embed_add(union atom **dst, struct cons *argp, struct sym *sym)
{
	const struct num *car;
	enum lisp_error r;
	double v;

	v = 0;
	while (argp->common.type != ATOM_TYPE_NIL) {
		car = &argp->reg[CONS_CAR]->num;
		if (car->common.type != ATOM_TYPE_NUM)
			return LISP_EINVAL_ATOM_TYPE;

		v += car->v;
		argp = &argp->reg[CONS_CDR]->cons;
	}

	r = atom_init(dst, ATOM_TYPE_NUM);
	if (r)
		return r;

	(*dst)->num.v = v;

	return LISP_EOK;
}

static int embed_atom(union atom **dst, struct cons *argp, struct sym *sym)
{
	const unsigned int argc = 1;
	union atom *args[argc];
	enum lisp_error r;

	r = getargs(args, argp, argc);
	if (r != LISP_EOK)
		return r;

	atom_bool(dst, args[0]->common.type != ATOM_TYPE_CONS);

	return 0;
}

static int embed_eq(union atom **dst, struct cons *argp, struct sym *sym)
{
	const unsigned int argc = 2;
	union atom *args[argc];
	enum lisp_error r;

	r = getargs(args, argp, argc);
	if (r != LISP_EOK)
		return r;

	atom_bool(dst, atom_eq(args[0], args[1]));

	return 0;
}

static int embed_templ_getreg(union atom **dst, struct cons *argp,
	enum cons_reg reg)
{
	const unsigned int argc = 1;
	union atom *args[argc];
	enum lisp_error r;

	r = getargs(args, argp, argc);
	if (r != LISP_EOK)
		return r;

	if (args[0]->common.type != ATOM_TYPE_CONS)
		return LISP_EINVAL_ATOM_TYPE;

	atom_ref(dst, args[0]->cons.reg[reg]);

	return LISP_EOK;
}

static int embed_car(union atom **dst, struct cons *argp, struct sym *sym)
{
	return embed_templ_getreg(dst, argp, CONS_CAR);
}

static int embed_cdr(union atom **dst, struct cons *argp, struct sym *sym)
{
	return embed_templ_getreg(dst, argp, CONS_CDR);
}

static int embed_position(union atom **dst, struct cons *argp, struct sym *sym)
{
	enum {
		ARG_ITEM,
		ARG_LIST,

		ARG_COUNT
	};

	union atom *args[ARG_COUNT];
	int r;
	int n;

	r = getargs(args, argp, ARG_COUNT);
	if (r != LISP_EOK)
		return r;

	n = list_position(args[ARG_ITEM], &args[ARG_LIST]->cons);
	if (n < 0)
		return n;

	r = atom_init(dst, ATOM_TYPE_NUM);
	if (r)
		return r;

	(*dst)->num.v = n;

	return 0;
}

static int embed_nth(union atom **dst, struct cons *argp, struct sym *sym)
{
	enum {
		ARG_NTH,
		ARG_LIST,

		ARG_COUNT
	};

	struct num *n;
	struct cons *list;
	union atom *nth;
	union atom *args[ARG_COUNT];
	enum lisp_error r;

	r = getargs(args, argp, ARG_COUNT);
	if (r != LISP_EOK)
		return r;

	n = &args[ARG_NTH]->num;
	if (n->common.type != ATOM_TYPE_NUM)
		return LISP_EINVAL_ATOM_TYPE;

	list = &args[ARG_LIST]->cons;
	if (list->common.type != ATOM_TYPE_CONS)
		return LISP_EINVAL_ATOM_TYPE;

	if (n->v < 0 || n->v > UINT_MAX || fmodl(n->v, 1))
		return LISP_EINVAL_NUM;

	nth = list_nth(n->v, list);
	if (nth == NULL)
		return LISP_EINVAL_ATOM_TYPE;

	atom_ref(dst, nth);

	return 0;
}

static int embed_cons(union atom **dst, struct cons *argp, struct sym *sym)
{
	enum {
		ARG_CAR,
		ARG_CDR,

		ARG_COUNT
	};

	union atom *args[ARG_COUNT];
	int r;

	r = getargs(args, argp, ARG_COUNT);
	if (r != LISP_EOK)
		return r;

	r = atom_init(dst, ATOM_TYPE_CONS);
	if (r)
		return r;

	atom_ref(&(*dst)->cons.reg[CONS_CAR], args[ARG_CAR]);
	atom_ref(&(*dst)->cons.reg[CONS_CDR], args[ARG_CDR]);

	return 0;
}

static int embed_length(union atom **dst, struct cons *argp, struct sym *sym)
{
	const unsigned int argc = 1;
	union atom *args[argc];
	int length;
	int r;

	r = getargs(args, argp, argc);
	if (r != LISP_EOK)
		return r;

	length = list_length(&args[0]->cons);
	if (length < 0)
		return length;

	r = atom_init(dst, ATOM_TYPE_NUM);
	if (r)
		return r;

	(*dst)->num.v = length;

	return 0;
}

static int embed_eval(union atom **dst, struct cons *argp, struct sym *sym)
{
	const unsigned int argc = 1;
	union atom *args[argc];
	enum lisp_error r;

	r = getargs(args, argp, argc);
	if (r != LISP_EOK)
		return r;

	r = atom_eval(dst, args[0], sym);
	if (r != LISP_EOK)
		return r;

	return 0;
}

static int embed_special_if(union atom **dst, struct cons *argp, struct sym *sym)
{
	enum {
		ARG_COND,
		ARG_T,
		ARG_NIL,

		ARG_COUNT
	};

	enum lisp_error r;
	union atom *args[ARG_COUNT];
	union atom *evaled_cond;

	r = getargs(args, argp, ARG_COUNT);
	if (r != LISP_EOK)
		return r;

	r = atom_eval(&evaled_cond, args[ARG_COND], sym);
	if (r)
		return r;

	r = atom_eval(dst,
		args[args[ARG_COND]->embed.type == ATOM_TYPE_NIL
			? ARG_NIL : ARG_T],
		sym);

	atom_free(evaled_cond);

	return 0;
}

static int embed_special_define(union atom **dst, struct cons *argp,
	struct sym *sym)
{
	enum {
		ARG_LABEL,
		ARG_FORM,

		ARG_COUNT
	};

	union atom *args[ARG_COUNT];
	union atom *label;
	union atom *form;
	int r;

	r = getargs(args, argp, ARG_COUNT);
	if (r)
		return r;

	if (args[ARG_LABEL]->common.type != ATOM_TYPE_SYM)
		return LISP_EINVAL_ATOM_TYPE;

	r = atom_init(&label, ATOM_TYPE_CONS);
	if (r)
		return r;

	r = atom_init(&form, ATOM_TYPE_CONS);
	if (r) {
		free(label);
		return r;
	}

	atom_ref(&label->cons.reg[CONS_CAR], args[ARG_LABEL]);
	atom_ref(&form->cons.reg[CONS_CAR], args[ARG_FORM]);

	*sym->label.btm = label;
	*sym->form.btm = form;

	sym->label.btm = &label->cons.reg[CONS_CDR];
	sym->form.btm = &form->cons.reg[CONS_CDR];

	label->cons.reg[CONS_CDR] = (void *)&embed_tbl[EMBED_NIL];
	form->cons.reg[CONS_CDR] = (void *)&embed_tbl[EMBED_NIL];

	atom_init(dst, ATOM_TYPE_SYM);
	(*dst)->str.v[0] = 0;

	return 0;
}

static int embed_special_lambda(union atom **dst, struct cons *argp,
	struct sym *sym)
{
	enum {
		ARG_LAMBDA,
		ARG_FORM,

		ARG_COUNT
	};

	union atom *args[ARG_COUNT];
	enum lisp_error r;

	r = getargs(args, argp, ARG_COUNT);
	if (r != LISP_EOK)
		return r;

	if (args[ARG_LAMBDA]->common.type != ATOM_TYPE_CONS
		&& args[ARG_LAMBDA]->common.type != ATOM_TYPE_NIL)
	{
		return LISP_EINVAL_ATOM_TYPE;
	}

	atom_init(dst, ATOM_TYPE_LAMBDA);
	atom_ref(&(*dst)->cons.reg[CONS_CAR], args[ARG_LAMBDA]);
	atom_ref(&(*dst)->cons.reg[CONS_CDR], args[ARG_FORM]);

	return 0;
}

static int embed_special_quote(union atom **dst, struct cons *argp,
	struct sym *sym)
{
	const unsigned int argc = 1;
	union atom *args[argc];
	enum lisp_error r;

	r = getargs(args, argp, argc);
	if (r != LISP_EOK)
		return r;

	atom_ref(dst, args[0]);

	return LISP_EOK;
}

const struct embed embed_tbl[EMBED_MAX] = {
	[EMBED_NIL] = { ATOM_TYPE_NIL, "nil", NULL },
	[EMBED_T] = { ATOM_TYPE_T, "T", NULL },
	[EMBED_COPYING] = { ATOM_TYPE_FUNC, "copying", embed_copying },
	[EMBED_ADD] = { ATOM_TYPE_FUNC, "+", embed_add },
	[EMBED_EQ] = { ATOM_TYPE_FUNC, "eq", embed_eq },
	[EMBED_CAR] = { ATOM_TYPE_FUNC, "car", embed_car },
	[EMBED_CDR] = { ATOM_TYPE_FUNC, "cdr", embed_cdr },
	[EMBED_NTH] = { ATOM_TYPE_FUNC, "nth", embed_nth },
	[EMBED_CONS] = { ATOM_TYPE_FUNC, "cons", embed_cons },
	[EMBED_ATOM] = { ATOM_TYPE_FUNC, "atom", embed_atom },
	[EMBED_EVAL] = { ATOM_TYPE_FUNC, "eval", embed_eval },
	[EMBED_LENGTH] = { ATOM_TYPE_FUNC, "length", embed_length },
	[EMBED_POSITION] = { ATOM_TYPE_FUNC, "position", embed_position },
	[EMBED_IF] = { ATOM_TYPE_SPECIAL, "if", embed_special_if },
	[EMBED_DEFINE] = { ATOM_TYPE_SPECIAL, "define", embed_special_define },
	[EMBED_LAMBDA] = { ATOM_TYPE_SPECIAL, "lambda", embed_special_lambda },
	[EMBED_QUOTE] = { ATOM_TYPE_SPECIAL, "quote", embed_special_quote }
};
