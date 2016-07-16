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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atom.h"
#include "embed.h"
#include "lisp_errno.h"
#include "parse.h"

enum char_type {
	CHAR_SPACE,
	CHAR_EOF,
	CHAR_SYMBOL,
	CHAR_INWORD
};

static enum char_type getchar_type(int c)
{
	switch (c) {
	case '\n':
	case '\t':
	case '\r':
	case ' ':
		return CHAR_SPACE;

	case EOF:
		return CHAR_EOF;

	case '(':
	case ')':
		return CHAR_SYMBOL;

	default:
		return CHAR_INWORD;
	}
}

static enum lisp_error getword(char *p)
{
	static int buffer = 0;
	size_t count;
	int c;
	enum char_type ctype;

	if (buffer) {
		c = buffer;
		buffer = 0;
	} else {
		c = getchar();
	}

	while (1) {
		ctype = getchar_type(c);
		switch (ctype) {
		case CHAR_SPACE:
			break;

		case CHAR_EOF:
			return LISP_EOF;

		default:
			for (count = 0; count < WORD_SIZE; count++) {
				switch (ctype) {
				case CHAR_SPACE:
					p[count] = 0;
					return LISP_EOK;

				case CHAR_EOF:
					p[count] = 0;
					exit(0);

				case CHAR_SYMBOL:
					if (count > 0) {
						buffer = c;
					} else {
						p[count] = c;
						count++;
					}

					p[count] = 0;
					return LISP_EOK;

				default:
					p[count] = c;
					c = getchar();
					ctype = getchar_type(c);
					break;
				}
			}

			return LISP_ETOOLONG_WORD;
		}

		c = getchar();
	}
}

static int process_word(union atom ** restrict p, char * restrict word)
{
	char *endptr;
	long double num;
	int i, r;

	if (!strcmp(word, "(")) {
		while (1) {
			getword(word);
			if (!strcmp(word, ")")) {
				*p = (void *)&embed_tbl[EMBED_NIL];
				break;
			}

			r = atom_init(p, ATOM_TYPE_CONS);
			if (r)
				return r;

			process_word(&(*p)->cons.reg[CONS_CAR], word);
			p = &(*p)->cons.reg[CONS_CDR];
		}

		return 0;
	}

	if (word[0] == '\"')
		return atom_init(p, ATOM_TYPE_STR);

	num = strtold(word, &endptr);
	if (*endptr == 0) {
		r = atom_init(p, ATOM_TYPE_NUM);
		if (r)
			return r;

		(*p)->num.v = num;
		return 0;
	}

	for (i = 0; i < EMBED_MAX; i++) {
		if (!strcmp(embed_tbl[i].str, word)) {
			*p = (void *)&embed_tbl[i];
			return 0;
		}
	}

	r = atom_init(p, ATOM_TYPE_SYM);
	if (r)
		return r;

	strcpy((*p)->str.v, word);

	return 0;
}

int parse(union atom ** restrict p)
{
	char word[WORD_SIZE];
	int r;

	r = getword(word);
	if (r)
		return r;

	return process_word(p, word);
}
