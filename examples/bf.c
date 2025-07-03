/* Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* bf.c - simple bf parser */

#include <stdlib.h>
#include <stdio.h>
#include <err.h>

#include "../pco.h"

static unsigned level = 0;	/* current loop level */

/* print leveled opcode name */
static void print_leveled(const char* msg)
{
	unsigned i;

	for (i = 0; i < level; i++)
		printf("| ");

	printf("%s\n", msg);
}

/* funtions for printing opcodes */
static void plus(struct pco_ctx* ctx, struct pco_result* result)	{ print_leveled("plus"); }
static void minus(struct pco_ctx* ctx, struct pco_result* result)	{ print_leveled("minus"); }
static void left(struct pco_ctx* ctx, struct pco_result* result)	{ print_leveled("left"); }
static void right(struct pco_ctx* ctx, struct pco_result* result)	{ print_leveled("right"); }
static void put(struct pco_ctx* ctx, struct pco_result* result)		{ print_leveled("put"); }
static void get(struct pco_ctx* ctx, struct pco_result* result)		{ print_leveled("get"); }

/* function for changing levels */
static void open(struct pco_ctx* ctx, struct pco_result* result)	{ level++; }
static void close(struct pco_ctx* ctx, struct pco_result* result)	{ level--; }

/* main function */
int main(int argc, char* argv[])
{
	/* check arguments */
	if (argc != 2)
		errx(EXIT_FAILURE, "invalid argument format");

	const char* input = argv[1];	/* get input */
	struct pco_ctx ctx;
	pco_create_ctx(&ctx);		/* create context */

	/* define parser */
	struct pco_parser bf_parser = pco_repeat(&ctx, pco_branch(&ctx, (struct pco_branch) {
		.count   = 2,
		.parsers = {
			/* normal opcodes */
			pco_branch(&ctx, (struct pco_branch) {
				.count   = 6,
				.parsers = {
					pco_map(&ctx, pco_char(&ctx, '+'), plus),
					pco_map(&ctx, pco_char(&ctx, '-'), minus),
					pco_map(&ctx, pco_char(&ctx, '<'), left),
					pco_map(&ctx, pco_char(&ctx, '>'), right),
					pco_map(&ctx, pco_char(&ctx, '.'), put),
					pco_map(&ctx, pco_char(&ctx, ','), get),
				},
			}),

			/* loops */
			pco_sequence(&ctx, (struct pco_branch) {
				.count   = 3,
				.parsers = {
					pco_map(&ctx, pco_char(&ctx, '['), open),
					pco_ptr(&ctx, &bf_parser),
					pco_map(&ctx, pco_char(&ctx, ']'), close),
				},
			}),
		},
	}));

	/* execute parser */
	struct pco_result result = pco_run_parser(&ctx, &bf_parser, input);

	/* check parser result */
	switch (result.status) {
	case PCO_OK:
		break;

	case PCO_UNEXEPTED:
		printf("unexepted character %c\n", result.data.unexepted);
		break;

	case PCO_END_OF_INPUT:
		printf("unexepted end of input\n");
		break;
	}

	/* free context */
	pco_free_ctx(&ctx);

	return 0;
}
