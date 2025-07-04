/* Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* pco.h - parser combinators library for c */

#include <stdbool.h>

#define PCO_BRANCH_PARSERS_COUNT 128	/* max parsers in branch */

/* parsers context */
struct pco_ctx {
	void** parsers_data;
	unsigned size;
};

/* exit status */
enum pco_status {
	PCO_OK = 0,		/* no errors */
	PCO_END_OF_INPUT,	/* excepted character but string ends */
	PCO_UNEXEPTED,		/* unexepted character */
};


/* parser result type */
struct pco_result {
	enum pco_status status;	/* status code */
	const char* rest;	/* unprocessed string */

	union {
		void* result;	/* parser result, real type depends on parser */
		char unexepted;	/* unexepted character */
	} data;
};

/* parser function type */
typedef struct pco_result (*pco_parser_f)(struct pco_ctx* ctx, void* data, const char* str);

/* parser */
struct pco_parser {
	pco_parser_f parser;	/* parser function */
	void* data;		/* arguments for parser function */
};

/* array type for parser result */
struct pco_result_array {
	void** results;	/* elements */
	unsigned size;	/* element count */
};

/* array for parsers */
struct pco_branch {
	struct pco_parser parsers[PCO_BRANCH_PARSERS_COUNT];
	unsigned count;
};

/* create context */
void pco_create_ctx(struct pco_ctx* ctx);

/* free context */
void pco_free_ctx(struct pco_ctx* ctx);		

/* parse one character, sets result to char* from one character */
struct pco_parser pco_char(struct pco_ctx* ctx, char c);

/* parse \n character */
struct pco_parser pco_new_line(struct pco_ctx* ctx);

/* parse \t character */
struct pco_parser pco_tab(struct pco_ctx* ctx);

/* parse space character */
struct pco_parser pco_space(struct pco_ctx* ctx);

/* parse \t, \n or space character */
struct pco_parser pco_anyspace(struct pco_ctx* ctx);

/* parse \t or space character many times or parse nothing */
struct pco_parser pco_manyspace(struct pco_ctx* ctx);

/* parse integer */
struct pco_parser pco_integer(struct pco_ctx* ctx);

/* parse string, sets result to char* from excepted string */
struct pco_parser pco_str(struct pco_ctx* ctx, const char* str);

typedef bool (*pco_filter_f)(char);					/* filter function */
typedef void (*pco_map_f)(struct pco_ctx*, struct pco_result* result);	/* map function */

/* parse characters while filter return true, sets result to char* from parsed characters */
struct pco_parser pco_filter(struct pco_ctx* ctx, pco_filter_f filter);

/* process other parser result */
struct pco_parser pco_map(struct pco_ctx* ctx, struct pco_parser parser, pco_map_f map);

/* apply parser many times while it not throw error */
struct pco_parser pco_repeat(struct pco_ctx* ctx, struct pco_parser parser);

/* apply parser many times while it not throw error but output should be not empty */
struct pco_parser pco_not_empty_repeat(struct pco_ctx* ctx, struct pco_parser parser);

/* apply parsers from branch while parser not throw error */
struct pco_parser pco_branch(struct pco_ctx* ctx, struct pco_branch branch);

/* apply all parsers from sequence */
struct pco_parser pco_sequence(struct pco_ctx* ctx, struct pco_branch sequence);

/* apply parser from parser (useful in recursive parsers) */
struct pco_parser pco_ptr(struct pco_ctx* ctx, struct pco_parser* parser);

/* run parser on str */
struct pco_result pco_run_parser(struct pco_ctx* ctx, const struct pco_parser* parser, const char* str);
