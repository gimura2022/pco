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

#ifndef _pco_h
#define _pco_h

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

#endif

#ifdef PCO_IMPLEMENTATION

/* Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* pco.c - parser combinators library for c */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "pco.h"

#define size_alloc(x) malloc(sizeof(x))	/* allocate sizeof(x) bytes */

/* create context */
void pco_create_ctx(struct pco_ctx* ctx)
{
	ctx->parsers_data = NULL;
	ctx->size         = 0;
}

/* free context */
void pco_free_ctx(struct pco_ctx* ctx)
{
	unsigned i;

	for (i = 0; i < ctx->size; i++)
		free(ctx->parsers_data[i]);

	free(ctx->parsers_data);
}

/* add data to ctx */
static void add_to_ctx(struct pco_ctx* ctx, void* data)
{
	ctx->size++;
	ctx->parsers_data                = realloc(ctx->parsers_data, ctx->size * sizeof(void*));
	ctx->parsers_data[ctx->size - 1] = data;
}

/* initialize pco_result_array */
static void create_arr(struct pco_result_array* arr)
{
	arr->results = NULL;
	arr->size    = 0;
}

/* add data to arr */
static void add_to_arr(struct pco_result_array* arr, void* data)
{
	arr->size++;
	arr->results                = realloc(arr->results, arr->size * sizeof(void*));
	arr->results[arr->size - 1] = data;
}

/* parser function for pco_char */
static struct pco_result char_parser(struct pco_ctx* ctx, char* data, const char* str)
{
	struct pco_result result = {
		.status = PCO_OK,
		.rest   = str + 1,
	};

	if (*str == '\0') {
		result.status = PCO_END_OF_INPUT;

		goto fail;
	}

	if (*str != *data) {
		result.status         = PCO_UNEXEPTED;
		result.data.unexepted = *str;

		goto fail;
	}

	char* c            = size_alloc(char);
	*c                 = *str;
	result.data.result = c;

	add_to_ctx(ctx, c);

fail:
	return result;
}

/* parse one character, sets result to char* from one character */
struct pco_parser pco_char(struct pco_ctx* ctx, char c)
{
	char* data = size_alloc(c);
	*data      = c;

	add_to_ctx(ctx, data);

	return (struct pco_parser) {
		.data   = data,
		.parser = (pco_parser_f) char_parser,
	};
}

/* parse \n character */
struct pco_parser pco_new_line(struct pco_ctx* ctx)
{
	return pco_char(ctx, '\n');
}

/* parse \t character */
struct pco_parser pco_tab(struct pco_ctx* ctx)
{
	return pco_char(ctx, '\t');
}

/* parse space character */
struct pco_parser pco_space(struct pco_ctx* ctx)
{
	return pco_char(ctx, ' ');
}

/* parse \t, \n or space character */
struct pco_parser pco_anyspace(struct pco_ctx* ctx)
{
	return pco_branch(ctx, (struct pco_branch) {
				.count = 3,
				.parsers = {
					pco_tab(ctx),
					pco_space(ctx),
					pco_new_line(ctx),
				},
			});
}

/* parser for pco_str */
static struct pco_result str_parser(struct pco_ctx* ctx, char* data, const char* str)
{
	struct pco_result result = {
		.status      = PCO_OK,
		.rest        = str + strlen(data),
		.data.result = data,
	};

	if (strlen(str) < strlen(data)) {
		result.status = PCO_END_OF_INPUT;

		goto fail;
	}

	if (strncmp(str, data, strlen(data))) {
		result.status         = PCO_UNEXEPTED;
		result.data.unexepted = *str;
	}

fail:
	return result;
}

/* parse string, sets result to char* from excepted string */
struct pco_parser pco_str(struct pco_ctx* ctx, const char* str)
{
	char* data = malloc(strlen(str) + 1);
	strcpy(data, str);

	add_to_ctx(ctx, data);

	return (struct pco_parser) {
		.data   = data,
		.parser = (pco_parser_f) str_parser,
	};
}

/* parser for pco_repeat */
static struct pco_result repeat_parser(struct pco_ctx* ctx, struct pco_parser* parser, const char* str)
{
	struct pco_result parser_result;
	struct pco_result_array arr;
	const char* rest         = str;
	struct pco_result result = {
		.status = PCO_OK,
	};

	create_arr(&arr);

	while ((parser_result = parser->parser(ctx, parser->data, rest)).status == PCO_OK) {
		rest = parser_result.rest;

		add_to_arr(&arr, parser_result.data.result);
	}
                     
	result.data.result                               = size_alloc(struct pco_result_array);
	*((struct pco_result_array*) result.data.result) = arr;
	result.rest                                      = rest;

	add_to_ctx(ctx, arr.results);
	add_to_ctx(ctx, result.data.result);

	return result;
}

/* apply parser many times while it not throw error */
struct pco_parser pco_repeat(struct pco_ctx* ctx, struct pco_parser parser)
{
	struct pco_parser* data = size_alloc(parser);
	*data                   = parser;

	add_to_ctx(ctx, data);

	return (struct pco_parser) {
		.parser = (pco_parser_f) repeat_parser,
		.data   = data,
	};
}

/* parser for pco_branch */
static struct pco_result branch_parser(struct pco_ctx* ctx, struct pco_branch* branch, const char* str)
{
	struct pco_result result;
	unsigned i;

	for (i = 0; i < branch->count; i++)
		if ((result = branch->parsers[i].parser(ctx, branch->parsers[i].data, str)).status == PCO_OK)
			return result;

	return result;
}

/* apply parsers from branch while parser not throw error */
struct pco_parser pco_branch(struct pco_ctx* ctx, struct pco_branch branch)
{
	struct pco_branch* data = size_alloc(branch);
	*data                   = branch;

	add_to_ctx(ctx, data);

	return (struct pco_parser) {
		.parser = (pco_parser_f) branch_parser,
		.data   = data,
	};
}

/* filter for digits */
static bool integer_filter(char c)
{
	return isdigit(c);
}

/* map function for conversion const char* to int in result type */
static void integer_map(struct pco_ctx* ctx, struct pco_result* result)
{
	int* data = size_alloc(int);
	*data     = atoi(result->data.result);

	add_to_ctx(ctx, data);

	result->data.result = data;
}

/* parse integer */
struct pco_parser pco_integer(struct pco_ctx* ctx)
{
	return pco_map(ctx, pco_filter(ctx, integer_filter), integer_map);
}

/* parse \t or space character many times or parse nothing */
struct pco_parser pco_manyspace(struct pco_ctx* ctx)
{
	return pco_not_empty_repeat(ctx, pco_anyspace(ctx));
}

/* parser function for pco_filter */
static struct pco_result filter_parser(struct pco_ctx* ctx, pco_filter_f filter, const char* str)
{
	const char* c;
	unsigned len;
	struct pco_result result = {
		.status = PCO_OK,
	};

	for (c = str, len = 0; *c != '\0'; c++)
		if (filter(*c))
			len++;

	result.rest        = str + len;
	result.data.result = malloc(len + 1);
	strncpy(result.data.result, str, len);
	((char*) result.data.result)[len] = '\0';

	add_to_ctx(ctx, result.data.result);

	return result;
}

/* parse \t or space character many times or parse nothing */
struct pco_parser pco_filter(struct pco_ctx* ctx, pco_filter_f filter)
{
	return (struct pco_parser) {
		.parser = (pco_parser_f) filter_parser,
		.data   = filter,
	};
}

/* parser function for pco_sequence */
static struct pco_result sequence_parser(struct pco_ctx* ctx, struct pco_branch* branch, const char* str)
{
	struct pco_result result = {
		.status      = PCO_OK,
	}, parser_result;
	const char* rest = str;
	struct pco_result_array arr;
	unsigned i;

	create_arr(&arr);

	for (i = 0; i < branch->count; i++) {
		if ((parser_result = branch->parsers[i].parser(ctx, branch->parsers[i].data, rest)).status
				!= PCO_OK)
			return parser_result;

		rest = parser_result.rest;
		add_to_arr(&arr, parser_result.data.result);
	}

	result.data.result                               = size_alloc(struct pco_result_array);
	*((struct pco_result_array*) result.data.result) = arr;
	result.rest                                      = rest;

	add_to_ctx(ctx, arr.results);
	add_to_ctx(ctx, result.data.result);

	return result;
}

/* apply all parsers from sequence */
struct pco_parser pco_sequence(struct pco_ctx* ctx, struct pco_branch sequence)
{
	struct pco_branch* data = size_alloc(sequence);
	*data                   = sequence;

	add_to_ctx(ctx, data);

	return (struct pco_parser) {
		.parser = (pco_parser_f) sequence_parser,
		.data   = data,
	};
}

/* structure for data in map parser */
struct map_data {
	struct pco_parser parser;
	pco_map_f map;
};

/* parser function for pco_map */
static struct pco_result map_parser(struct pco_ctx* ctx, struct map_data* map_data, const char* str)
{
	struct pco_result result = map_data->parser.parser(ctx, map_data->parser.data, str);

	if (result.status != PCO_OK)
		return result;

	map_data->map(ctx, &result);

	return result;
}

/* process other parser result */
struct pco_parser pco_map(struct pco_ctx* ctx, struct pco_parser parser, pco_map_f map)
{
	struct map_data* data = size_alloc(struct map_data);
	*data                 = (struct map_data) {
		.map    = map,
		.parser = parser,
	};

	add_to_ctx(ctx, data);

	return (struct pco_parser) {
		.parser = (pco_parser_f) map_parser,
		.data   = data,
	};
}

/* map function for pco_not_empty_repeat */
static void not_empty_repeat_map(struct pco_ctx* ctx, struct pco_result* result)
{
	struct pco_result_array* arr = result->data.result;

	if (arr->size == 0 && *result->rest == '\0')
		result->status = PCO_END_OF_INPUT;
	else if (arr->size == 0) {
		result->status         = PCO_UNEXEPTED;
		result->data.unexepted = *result->rest;
	}
}

/* apply parser many times while it not throw error but output should be not empty */
struct pco_parser pco_not_empty_repeat(struct pco_ctx* ctx, struct pco_parser parser)
{
	return pco_map(ctx, pco_repeat(ctx, parser), not_empty_repeat_map);
}

/* parser function for pco_ptr */
static struct pco_result ptr_parser(struct pco_ctx* ctx, struct pco_parser* parser, const char* str)
{
	return parser->parser(ctx, parser->data, str);
}

/* apply parser from parser (useful in recursive parsers) */
struct pco_parser pco_ptr(struct pco_ctx* ctx, struct pco_parser* parser)
{
	return (struct pco_parser) {
		.parser = (pco_parser_f) ptr_parser,
		.data   = parser,
	};
}

/* run parser on str */
struct pco_result pco_run_parser(struct pco_ctx* ctx, const struct pco_parser* parser, const char* str)
{
	struct pco_result result = parser->parser(ctx, parser->data, str);

	if (result.status != PCO_OK)
		goto fail;

	if (*result.rest != '\0') {
		result.status         = PCO_UNEXEPTED;
		result.data.unexepted = *result.rest;
	}

fail:
	return result;
}

#endif
