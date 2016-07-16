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
#include "embed.h"
#include "list.h"
#include "lisp_errno.h"
#include "parse.h"

int main()
{
	struct sym sym;
	union atom *parsed;
	union atom *evaled;
	int r;

	puts("Copyright (C) 2016 173210 <root.3.173210@live.com>\n"
		"This program comes with ABSOLUTELY NO WARRANTY.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; type `(copying)' for details.");

	sym.label.top = (void *)&embed_tbl[EMBED_NIL];
	sym.form.top = (void *)&embed_tbl[EMBED_NIL];

	sym.label.btm = &sym.label.top;
	sym.form.btm = &sym.form.top;

	while (1) {
		printf("> ");
		r = parse(&parsed);
		if (r == LISP_EOF) {
			break;
		} else if (r) {
			fprintf(stderr, "error: failed to parse: 0x%X\n", r);
			continue;
		}

		r = atom_eval(&evaled, parsed, &sym);
		if (r) {
			fprintf(stderr, "error: failed to evaluate: 0x%X\n", r);
			continue;
		}

		atom_free(parsed);

		atom_print(evaled);
		putchar('\n');

		atom_free(parsed);
	}

	return 0;
}
