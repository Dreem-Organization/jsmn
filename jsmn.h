#ifndef __JSMN_H_
#define __JSMN_H_

#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_PRIMITIVE = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_KEY = 4
} jsmntype_t;

typedef enum {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
} jsmnerr_t;

/**
 * JSON token description.
 * @param		type	type (object, array, string etc.)
 * @param		start	start position in JSON data string
 * @param		end		end position in JSON data string
 */
typedef struct {
	jsmntype_t type;
	int start;
	int end;
	int size;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also stores
 * the string being parsed now and current position in that string
 */
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each describing
 * a single JSON object.
 */
jsmnerr_t jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens);
jsmnerr_t jsmn_parse_offset(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens, int offset);

#ifdef __cplusplus
}
#endif


// Patch to add a few higher level get functions
typedef struct json_parse_str
{
	char * json_buffer;
	jsmntok_t * json_tokens;
	int tokens_count;
	char verbose;
} json_parse_str;

int json_parse_conf_file(json_parse_str * ctx, char * filename);

int json_parse_expand_conf_file(json_parse_str * ctx, char * filename);

int json_get_value_int(json_parse_str * ctx, char * key_str, int * val_int);

int json_get_value_float(json_parse_str * ctx, char * key_str, float * val_datatype);

int json_get_value_double(json_parse_str * ctx, char * key_str, double * val_datatype);

int json_get_value_string(json_parse_str * ctx, char * key_str, char ** val_str);

int json_get_nstring (json_parse_str * ctx, char * key_str, char * val_str, int n);

int json_get_array_float(json_parse_str * ctx, char * key_str, float ** val_array);

int json_get_array_double(json_parse_str * ctx, char * key_str, double ** val_array);

int json_get_array_string(json_parse_str * ctx, char * key_str, char *** val_array, int * array_size);

int json_get_value_string_into_array(json_parse_str * ctx, char * key_str, int ind, char ** val_str);

int json_get_indice_string_into_array(json_parse_str * ctx, char * key_str, char * val_str, int * ind);

int json_get_array_size(json_parse_str * ctx, char * key_str, int * array_size);

int json_get_array_int(json_parse_str * ctx, char * key_str, int ** val_array);

int json_get_array_nint(json_parse_str * ctx, char * key_str, int * val_array, int n);

int json_free_array_string(char *** val_array, int array_size);

int json_parse_free(json_parse_str * ctx);


#endif /* __JSMN_H_ */
