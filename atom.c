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

#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "lisp_errno.h"
#include "embed.h"
#include "list.h"
#include "atom.h"

struct atom_info {
	void (* print)(const union atom *);
	enum atom_st_type st_type;
};

static const struct atom_info atom_info[ATOM_TYPE_MAX];

void atom_ref(union atom **dst, union atom *src)
{
	if (atom_info[src->common.type].st_type != ATOM_ST_TYPE_EMBED)
		src->common.refcount++;

	*dst = src;
}

static void cons_free(struct cons *p)
{
	int i;

	p->common.refcount--;
	if (p->common.refcount <= 0)
		for (i = 0; i < CONS_MAX; i++)
			atom_free(p->reg[i]);
}

void atom_free(union atom *p)
{
	switch (atom_info[p->common.type].st_type) {
	case ATOM_ST_TYPE_CONS:
		cons_free(&p->cons);
		break;

	case ATOM_ST_TYPE_EMBED:
		break;

	default:
		p->common.refcount--;
		if (p->common.refcount <= 0)
			free(p);

		break;
	}
}

void atom_bool(union atom **p, bool b)
{
	*p = (void *)&embed_tbl[b ? EMBED_T : EMBED_NIL];
}

bool atom_eq(const union atom *p, const union atom *q)
{
	if (p->common.type != q->common.type)
		return false;

	switch (p->common.type) {
	case ATOM_TYPE_CONS:
		return atom_eq(p->cons.reg[CONS_CAR], q->cons.reg[CONS_CAR])
			&& atom_eq(p->cons.reg[CONS_CDR], q->cons.reg[CONS_CDR]);

	case ATOM_TYPE_NUM:
		return p->num.v == q->num.v;

	case ATOM_TYPE_STR:
	case ATOM_TYPE_SYM:
		return strcmp(p->str.v, q->str.v) == 0;

	case ATOM_TYPE_T:
		return true;

	default:
		return false;
	}
}

static void atom_embed_print(const union atom *p)
{
	printf("%s", p->embed.str);
}

static void atom_num_print(const union atom *p)
{
	printf("%Lf", p->num.v);
}

static void atom_str_print(const union atom *p)
{
	printf("\"%s\"", p->str.v);
}

static void atom_sym_print(const union atom *p)
{
	printf("%s", p->str.v);
}

static void atom_cons_print(const union atom *p)
{
	putchar('(');
	atom_print(p->cons.reg[CONS_CAR]);
	printf(" . ");
	atom_print(p->cons.reg[CONS_CDR]);
	putchar(')');
}

static void atom_lambda_print(const union atom *p)
{
	printf("(lambda ");
	atom_print(p->cons.reg[CONS_CAR]);
	putchar(' ');
	atom_print(p->cons.reg[CONS_CDR]);
	putchar(')');
}

static int atom_sym_eval(union atom **dst, union atom *src, struct sym *sym)
{
	union atom *nth;
	union atom *ref;
	int r;

	r = list_position(src, &sym->label.top->cons);
	if (r < 0)
		return r;

	nth = list_nth(r, &sym->form.top->cons);
	if (nth == NULL)
		return LISP_ESYM;

	atom_ref(&ref, nth);
	r = atom_eval(dst, ref, sym);
	atom_free(ref);

	return r;
}

static int atom_cons_eval(union atom **dst, union atom *src, struct sym *sym)
{
	union atom *reg[CONS_MAX];
	union atom **lambda_end;
	union atom **args_end;
	union atom **cdr_end;
	struct cons *list;
	unsigned int n;
	int r;

	list = &src->cons;

	r = atom_eval(&reg[CONS_CAR], list->reg[CONS_CAR], sym);
	if (r)
		goto end;

	switch (reg[CONS_CAR]->common.type) {
	case ATOM_TYPE_LAMBDA:
		n = 0;
		lambda_end = &reg[CONS_CAR]->cons.reg[CONS_CAR];
		while ((*lambda_end)->common.type != ATOM_TYPE_NIL) {
			n++;
			lambda_end = &(*lambda_end)->cons.reg[CONS_CDR];
		}

		args_end = &list->reg[CONS_CDR];
		while ((*args_end)->common.type != ATOM_TYPE_NIL) {
			if (n <= 0) {
				r = LISP_EINVAL_ATOM_TYPE;
				goto end_car;
			}

			n--;
			args_end = &(*args_end)->cons.reg[CONS_CDR];
		}

		*lambda_end = (void *)sym->label.top;
		*args_end = (void *)sym->form.top;

		if (sym->label.top->common.type == ATOM_TYPE_NIL) {
			sym->label.btm = (void *)lambda_end;
			sym->form.btm = (void *)args_end;
		}

		atom_ref(&sym->label.top, reg[CONS_CAR]->cons.reg[CONS_CAR]);
		atom_ref(&sym->form.top, list->reg[CONS_CDR]);

		r = atom_eval(dst, reg[CONS_CAR]->cons.reg[CONS_CDR], sym);

		atom_free(sym->label.top);
		atom_free(sym->form.top);

		sym->label.top = *sym->label.btm;
		sym->form.top = *sym->form.btm;

		if (sym->label.top->common.type == ATOM_TYPE_NIL) {
			sym->label.btm = &sym->label.top;
			sym->form.btm = &sym->form.top;
		}

		*lambda_end = (void *)&embed_tbl[EMBED_NIL];
		*args_end = (void *)&embed_tbl[EMBED_NIL];

		goto end_car;

	case ATOM_TYPE_FUNC:
		cdr_end = &reg[CONS_CDR];
		while ((list = &list->reg[CONS_CDR]->cons)->common.type
			!= ATOM_TYPE_NIL)
		{
			if (list->common.type != ATOM_TYPE_CONS) {
				*cdr_end = (void *)&embed_tbl[EMBED_NIL];
				r = LISP_EINVAL_ATOM_TYPE;
				goto end_cdr;
			}

			r = atom_init(cdr_end, ATOM_TYPE_CONS);
			if (r) {
				*cdr_end = (void *)&embed_tbl[EMBED_NIL];
				goto end_cdr;
			}

			r = atom_eval(&(*cdr_end)->cons.reg[CONS_CAR],
				list->reg[CONS_CAR],
				sym);
			if (r) {
				free(*cdr_end);
				*cdr_end = (void *)&embed_tbl[EMBED_NIL];
				goto end_cdr;
			}

			cdr_end = &(*cdr_end)->cons.reg[CONS_CDR];
		}

		*cdr_end = (void *)&embed_tbl[EMBED_NIL];
		break;

	case ATOM_TYPE_SPECIAL:
		atom_ref(&reg[CONS_CDR], list->reg[CONS_CDR]);
		break;

	default:
		r = LISP_EINVAL_ATOM_TYPE;
		goto end_car;
	}

	r = reg[CONS_CAR]->embed.func(
		dst, &reg[CONS_CDR]->cons, sym);

end_cdr:
	atom_free(reg[CONS_CDR]);
end_car:
	atom_free(reg[CONS_CAR]);
end:
	return r;
}

static const struct atom_info atom_info[ATOM_TYPE_MAX] = {
	[ATOM_TYPE_NIL] = { atom_embed_print, ATOM_ST_TYPE_EMBED },
	[ATOM_TYPE_T] = { atom_embed_print, ATOM_ST_TYPE_EMBED },
	[ATOM_TYPE_NUM] = { atom_num_print, ATOM_ST_TYPE_NUM },
	[ATOM_TYPE_STR] = { atom_str_print, ATOM_ST_TYPE_STR },
	[ATOM_TYPE_SYM] = { atom_sym_print, ATOM_ST_TYPE_STR },
	[ATOM_TYPE_CONS] = { atom_cons_print, ATOM_ST_TYPE_CONS },
	[ATOM_TYPE_FUNC] = { atom_embed_print, ATOM_ST_TYPE_EMBED },
	[ATOM_TYPE_SPECIAL] = { atom_embed_print, ATOM_ST_TYPE_EMBED },
	[ATOM_TYPE_LAMBDA] = { atom_lambda_print, ATOM_ST_TYPE_CONS }
};

int atom_init(union atom * restrict *p, enum atom_type type)
{
	static const size_t tbl[ATOM_ST_TYPE_MAX] = {
		[ATOM_ST_TYPE_NUM] = sizeof(struct num),
		[ATOM_ST_TYPE_STR] = sizeof(struct str),
		[ATOM_ST_TYPE_CONS] = sizeof(struct cons),
		[ATOM_ST_TYPE_EMBED] = sizeof(struct embed)
	};

	union atom *q;

	q = malloc(tbl[atom_info[type].st_type]);
	if (q == NULL)
		return -errno;

	q->common.type = type;
	q->common.refcount = 1;
	*p = q;

	return 0;
}

void atom_print(const union atom *atom)
{
	atom_info[atom->common.type].print(atom);
}

int atom_eval(union atom **dst, union atom *src, struct sym *sym)
{
	switch (src->common.type) {
	case ATOM_TYPE_CONS:
		return atom_cons_eval(dst, src, sym);

	case ATOM_TYPE_SYM:
		return atom_sym_eval(dst, src, sym);

	default:
		atom_ref(dst, src);
		return 0;
	}
}
