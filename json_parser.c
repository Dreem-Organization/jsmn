/**
**

** \file json_parser.c
** \brief parse argument of a main function
** \author Samuel Beaussier
** \date 19/12/2014
**
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsmn.h"
#include <fcntl.h>
#include <unistd.h>
# include <errno.h>
# include <syslog.h>

#define JSON_TOKENS 10
#define JSON_ERROR "JSON"
#define CLEAN_ERRNO() (errno == 0 ? "None" : strerror(errno))

// Malloc
#define MALLOC(ctx,destination, type, size) \
	CHECK_ERR_RET(ctx,destination != NULL, "adsadsAlready allocated !!"); \
	destination = (type *) malloc(size * sizeof(type))

// Calloc
#define CALLOC(ctx,destination, type, size) \
	CHECK_ERR_RET(ctx,destination != NULL, "aaAlready allocated !!"); \
	destination = (type *) calloc(size, sizeof(type))

#define FREE_ARRAY_LOOP(a,n,i) \
	if (a != NULL) \
	{ \
		for (i = 0; i < n; i++) \
		{ \
			if ((a)[i] != NULL) \
			{ \
				free((a)[i]); \
				(a)[i] = NULL; \
			} \
		} \
		free(a); \
		a = NULL; \
	}

// Free
#define FREE(a) \
	if (a != NULL) \
	{ \
		free(a); \
		a = NULL; \
	}

#define _PRINT_ERR_GOTO(ctx,M, ...)								\
	do {\
		if (ctx->verbose) \
		{\
			printf("[%s][ERROR] [errno: %s] " M "%s\n", JSON_ERROR, CLEAN_ERRNO(), __VA_ARGS__); \
			syslog(LOG_ERR, "[%s][ERROR] [errno: %s] " M "%s\n", JSON_ERROR, CLEAN_ERRNO(), __VA_ARGS__); \
		}\
		errno = 0;									\
		goto error;									\
} while (0)

#define _PRINT_ERR_RET(ctx,M, ...)								\
	do {\
		if (ctx->verbose) \
		{\
			printf("[%s][ERROR] [errno: %s] " M "%s\n", JSON_ERROR, CLEAN_ERRNO(), __VA_ARGS__); \
			syslog(LOG_ERR, "[%s][ERROR] [errno: %s] " M "%s\n", JSON_ERROR, CLEAN_ERRNO(), __VA_ARGS__); \
		}\
		errno = 0;									\
		return -1;									\
	} while (0)
#define PRINT_ERR_GOTO(ctx,...) _PRINT_ERR_GOTO(ctx,__VA_ARGS__, "")
#define PRINT_ERR_RET(ctx,...) _PRINT_ERR_RET(ctx,__VA_ARGS__, "")
#define CHECK_ERR_GOTO(ctx,A, ...) if((A)) { _PRINT_ERR_GOTO(ctx,__VA_ARGS__, "");}
#define CHECK_ERR_RET(ctx,A, ...) if((A)) { _PRINT_ERR_RET(ctx,__VA_ARGS__, "");}


static inline int json_token_streq(char *js, jsmntok_t *t, char *s)
{
	return (strncmp(js + t->start, s, t->end - t->start) == 0
			&& strlen(s) == (size_t) (t->end - t->start));
}

static int json_tokenise(json_parse_str * ctx)
{
	jsmn_parser parser;
	unsigned int n = JSON_TOKENS;
	int ret;

	jsmn_init(&parser);

	n = jsmn_parse(&parser, ctx->json_buffer, strlen(ctx->json_buffer), NULL, 0);

	CALLOC(ctx, ctx->json_tokens, jsmntok_t, n);
	CHECK_ERR_RET(ctx, ctx->json_tokens == NULL, "malloc failed\n");

	jsmn_init(&parser);

	ret = jsmn_parse(&parser, ctx->json_buffer, strlen(ctx->json_buffer), ctx->json_tokens, n);

	CHECK_ERR_RET(ctx, ret == JSMN_ERROR_NOMEM,
				"Invalid JSON string\n");

	CHECK_ERR_RET(ctx, ret == JSMN_ERROR_INVAL,
			"Invalid JSON string\n");

	CHECK_ERR_RET(ctx, ret == JSMN_ERROR_PART,
			"Truncated JSON string\n");

	ctx->tokens_count = n;

	return 0;
}

static int json_tokenise_expand(json_parse_str * ctx, int pos)
{
	jsmn_parser parser;
	unsigned int n = JSON_TOKENS;
	int ret;
	char *buffer = &ctx->json_buffer[pos];

	jsmn_init(&parser);

	n = jsmn_parse(&parser, buffer, strlen(buffer), NULL, 0);

	ctx->json_tokens = realloc(ctx->json_tokens, sizeof(jsmntok_t) * (n + ctx->tokens_count));
	CHECK_ERR_RET(ctx, ctx->json_tokens == NULL, "malloc failed\n");

	jsmn_init(&parser);

	ret = jsmn_parse_offset(&parser, buffer, strlen(buffer), &ctx->json_tokens[ctx->tokens_count], n, pos);

	CHECK_ERR_RET(ctx, ret == JSMN_ERROR_NOMEM,
				"Invalid JSON string\n");

	CHECK_ERR_RET(ctx, ret == JSMN_ERROR_INVAL,
			"Invalid JSON string\n");

	CHECK_ERR_RET(ctx, ret == JSMN_ERROR_PART,
			"Truncated JSON string\n");

	ctx->tokens_count += n;

	return 0;
}

int json_parse_conf_file(json_parse_str * ctx, char * filename)
{

	int json_file;
	int nb_caract;
	int current_char;

	// Open file
	json_file = open(filename, O_RDONLY, 0640);
	CHECK_ERR_RET(ctx, json_file < 0, "Could not open JSON file\n");

	//Get file size
	nb_caract = lseek(json_file, 0, SEEK_END);

	// Copy file into temporary buffer
	MALLOC(ctx, ctx->json_buffer, char, nb_caract + 1);
	lseek(json_file, 0, SEEK_SET);
	current_char = read(json_file, ctx->json_buffer, nb_caract);

	CHECK_ERR_RET(ctx, current_char != nb_caract,
			"Could not extract data from file\n");

	ctx->json_buffer[current_char] = '\0';

	// Parse the buffer
	CHECK_ERR_RET(ctx, json_tokenise(ctx),
			"Error while extracting JSON tokens \n");

	close(json_file);

	return 0;
}

int json_parse_expand_conf_file(json_parse_str * ctx, char * filename)
{

	int json_file;
	int nb_caract;
	int current_char;
	int pos = ctx->json_tokens[0].end + 1;

	// Open file
	json_file = open(filename, O_RDONLY, 0640);
	if (json_file < 0)
		return -1;

	//Get file size
	nb_caract = lseek(json_file, 0, SEEK_END);

	// Copy file into temporary buffer
	ctx->json_buffer = realloc (ctx->json_buffer, pos + nb_caract + 1);
	lseek(json_file, 0, SEEK_SET);
	current_char = read(json_file, &ctx->json_buffer[pos], nb_caract);

	CHECK_ERR_RET(ctx, current_char != nb_caract,
			"Could not extract data from file\n");

	ctx->json_buffer[pos + nb_caract] = '\0';

	// Parse the buffer
	CHECK_ERR_RET(ctx, json_tokenise_expand(ctx, pos),
			"Error while extracting JSON tokens \n");

	close(json_file);

	return 0;
}


int json_get_key_token(json_parse_str * ctx, char * key_str)
{
	int i;

	for (i = ctx->tokens_count - 1; i >= 0; i--)
	{
		if (ctx->json_tokens[i].type == JSMN_KEY)
		{
			if (json_token_streq(ctx->json_buffer, &ctx->json_tokens[i], key_str))
			{
				return i;
			}
		}
	}

	// Not found
	return -1;
}

int json_get_int(json_parse_str * ctx, int key_ind, int * val_int)
{
	jsmntok_t * int_token = &ctx->json_tokens[key_ind];

	if (int_token->type != JSMN_PRIMITIVE)
	{
		return -1;
	}

	*val_int = atoi(&ctx->json_buffer[int_token->start]);

	return 0;
}

int json_get_value_int(json_parse_str * ctx, char * key_str, int * val_int)
{
	int key_ind;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	CHECK_ERR_RET(ctx, json_get_int(ctx, key_ind + 1, val_int),
			"Value is not a primitive \n");

	return 0;
}

static int json_get_float(json_parse_str * ctx, int key_ind, float * val_datatype)
{
	jsmntok_t * datatype_token = &ctx->json_tokens[key_ind];

	if (datatype_token->type != JSMN_PRIMITIVE)
	{
		return -1;
	}

	*val_datatype = atof(&ctx->json_buffer[datatype_token->start]);

	return 0;
}

static int json_get_double(json_parse_str * ctx, int key_ind, double * val_datatype)
{
	jsmntok_t * datatype_token = &ctx->json_tokens[key_ind];

	if (datatype_token->type != JSMN_PRIMITIVE)
	{
		return -1;
	}

	*val_datatype = atof(&ctx->json_buffer[datatype_token->start]);

	return 0;
}

int json_get_value_float(json_parse_str * ctx, char * key_str, float * val_datatype)
{
	int key_ind;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	CHECK_ERR_RET(ctx, json_get_float(ctx, key_ind + 1, val_datatype),
			"Value is not a primitive \n");

	return 0;
}

int json_get_value_double(json_parse_str * ctx, char * key_str, double * val_datatype)
{
	int key_ind;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	CHECK_ERR_RET(ctx, json_get_double(ctx, key_ind + 1, val_datatype),
			"Value is not a primitive \n");

	return 0;
}

static int json_get_string(json_parse_str * ctx, int key_ind, char ** val_str)
{
	jsmntok_t * string_token = &ctx->json_tokens[key_ind];
	char * tmp_buf = NULL;

	if (string_token->type != JSMN_STRING)
	{
		return -1;
	}

	// Alloc & Copy
	tmp_buf = realloc(*val_str, string_token->end - string_token->start + 1);
	memcpy (tmp_buf, &ctx->json_buffer[string_token->start], string_token->end - string_token->start);
	tmp_buf[string_token->end - string_token->start] = '\0';

	*val_str = tmp_buf;

	return 0;
}

int json_get_value_string(json_parse_str * ctx, char * key_str, char ** val_str)
{
	int key_ind;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	CHECK_ERR_RET(ctx, json_get_string(ctx, key_ind + 1, val_str),
			"Value is not a string \n");

	return 0;
}

int json_get_nstring (json_parse_str * ctx, char * key_str, char * val_str, int n)
{
	int key_ind;
	jsmntok_t * string_token;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	if (key_ind < 0)
		return -1;

	string_token = &ctx->json_tokens[key_ind+1];
	if (string_token->type != JSMN_STRING)
		return -1;

	if (n < string_token->end - string_token->start + 1)
		return string_token->end - string_token->start + 1;

	memcpy (val_str, &ctx->json_buffer[string_token->start], string_token->end - string_token->start);
	val_str[string_token->end - string_token->start] = '\0';

	return 0;
}

int json_get_array_int(json_parse_str * ctx, char * key_str, int ** val_array)
{
	jsmntok_t * array_token = NULL;
	int key_ind;
	int i;
	int * tmp_array = NULL;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	// Alloc
	MALLOC(ctx, tmp_array, int, array_token->size);

	for (i = 0 ; i < array_token->size ; i++)
	{
		CHECK_ERR_RET(ctx, json_get_int(ctx, key_ind + 2 + i, &tmp_array[i]),
				"Value is not a primitive \n");
	}

	*val_array = tmp_array;

	return 0;
}

int json_get_array_nint(json_parse_str * ctx, char * key_str, int * val_array, int n)
{
	jsmntok_t * array_token = NULL;
	int key_ind;
	int i;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	// Check size
	CHECK_ERR_RET(ctx, n < array_token->size, "Given array is too small %d < %d", n, array_token->size);

	for (i = 0 ; i < array_token->size ; i++)
	{
		CHECK_ERR_RET(ctx, json_get_int(ctx, key_ind + 2 + i, &val_array[i]),
				"Value is not a primitive \n");
	}

	return 0;
}


int json_get_array_float(json_parse_str * ctx, char * key_str, float ** val_array)
{
	jsmntok_t * array_token = NULL;
	int key_ind;
	int i;
	float * tmp_array = NULL;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	// Alloc
	MALLOC(ctx, tmp_array, float, array_token->size);

	for (i = 0 ; i < array_token->size ; i++)
	{
		CHECK_ERR_RET(ctx, json_get_float(ctx, key_ind + 2 + i, &tmp_array[i]),
				"Value is not a primitive \n");
	}

	*val_array = tmp_array;

	return 0;
}

int json_get_array_double(json_parse_str * ctx, char * key_str, double ** val_array)
{
	jsmntok_t * array_token = NULL;
	int key_ind;
	int i;
	double * tmp_array = NULL;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	// Alloc
	MALLOC(ctx, tmp_array, double, array_token->size);

	for (i = 0 ; i < array_token->size ; i++)
	{
		CHECK_ERR_RET(ctx, json_get_double(ctx, key_ind + 2 + i, &tmp_array[i]),
				"Value is not a primitive \n");
	}

	*val_array = tmp_array;

	return 0;
}

int json_get_array_string(json_parse_str * ctx, char * key_str, char *** val_array, int * array_size)
{
	jsmntok_t * array_token = NULL;
	int key_ind;
	int i;
	char ** tmp_array = NULL;
	int ret;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	// Alloc
	MALLOC(ctx, tmp_array, char *, array_token->size);

	ret = 0;
	for (i = 0 ; i < array_token->size ; i++)
	{
		tmp_array[i] = NULL;

		ret |= json_get_string(ctx, key_ind + 2 + i, &tmp_array[i]);
	}


	*val_array = tmp_array;
	*array_size = array_token->size;

	if (ret)
	{
		printf("value is not a primitive \n");
		return -2;
	}

	return 0;
}

int json_is_key_exist(json_parse_str * ctx, char * key_str)
{
	if (json_get_key_token(ctx, key_str) < 0) return 0;
	return 1;
}

int json_get_value_string_into_array(json_parse_str * ctx, char * key_str, int ind, char ** val_str)
{
	int key_ind;
	jsmntok_t * array_token;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	// Find string into array
	CHECK_ERR_RET(ctx, json_get_string(ctx, key_ind + 2 + ind, val_str),
			"Value is not a string \n");

	return 0;
}

int json_get_indice_string_into_array(json_parse_str * ctx, char * key_str, char * val_str, int * ind)
{
	jsmntok_t * array_token;
	int key_ind;
	char * curr_str = NULL;
	int i;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_GOTO(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_GOTO(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	for (i = 0 ; i < array_token->size ; i++)
	{
		CHECK_ERR_GOTO(ctx, json_get_string(ctx, key_ind + 2 + i, &curr_str),
				"Value is not a primitive \n");

		if (strcmp(val_str, curr_str) == 0)
		{
			*ind = i;
			FREE(curr_str);
			return 0;
		}

	}

	error:
		printf("Indice not found in array");
		FREE(curr_str);
		return -1;

}

int json_get_array_size(json_parse_str * ctx, char * key_str, int * array_size)
{
	jsmntok_t * array_token;
	int key_ind;

	// Find key into tokens
	key_ind = json_get_key_token(ctx, key_str);

	CHECK_ERR_RET(ctx, key_ind < 0,
			"Key \"%s\" not found into tokens \n", key_str);

	// Get array token
	array_token = &ctx->json_tokens[key_ind + 1];
	CHECK_ERR_RET(ctx, array_token->type != JSMN_ARRAY,
			"Value is not an array \n");

	*array_size = array_token->size;

	return 0;
}

int json_free_array_string(char *** val_array, int array_size)
{
	int i;

	FREE_ARRAY_LOOP(*val_array, array_size, i);

	return 0;
}

int json_parse_free(json_parse_str * ctx)
{
	FREE(ctx->json_buffer);


	FREE(ctx->json_tokens);

	return 0;
}

