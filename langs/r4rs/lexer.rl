/*
 * Scheme R4RS scanner. Uses the longest match construction.
 * based on Ragel examples
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../../src/scheme-private.h"

#define ADVANCE(A)	state->token_start	= ts; \
			state->token_end	= te; \
			state->token_line	= line; \
			copy_token(ts, te, tmp); \
			state->current_cell	= token_to_##A(tmp); \
			parser_advance(parser, state->current_cell->type, state->current_cell, state)

#define ADVANCE_TOKEN(A)	parser_advance(parser, A, NULL, state)

#define PUSH_TE()	const char* tmp_te = te
#define POP_TE()	te	= tmp_te
#define PUSH_TS()	const char* tmp_ts = ts
#define POP_TS()	ts	= tmp_ts

/* EOF char used to flush out that last token. This should be a whitespace
 * token. */

#define LAST_CHAR 0

extern void*	parser_alloc(void *(*mallocProc)(size_t));
extern void	parser_free(void *p, void (*freeProc)(void*));
extern void	parser_advance(void *yyp, int yymajor, cell_t* yyminor, scheme_t* state);

%%{
	machine scanner;
	write data nofinal;

	# operator char
	op_char_left = [\+\-\*/\|\&\>\<\=\~\!\^\%\?\:]+;
	op_char_right = '/*' | '*/' | '//';
	op_char = ( op_char_left - op_char_right );
	# Floating literals.
	fract_const = digit* '.' digit+ | digit+ '.';
	exponent = [eE] [+\-]? digit+;

	action inc_line { ++line; }

	c_comment := (any | '\n'@inc_line)* :>> '*/' @{ fgoto main; };

	main := |*
		'true'						{ ADVANCE( boolean );};
		'false'						{ ADVANCE( boolean );};
		'quote'						{ ADVANCE_TOKEN( QUOTE ); };
		"'"						{ ADVANCE_TOKEN( QUOTE ); };

		# Single and double literals.
		( "'" (any - ['\\] ) "'" )			{
									PUSH_TE();
									PUSH_TS();
									++ts; --te;
									ADVANCE( char );
									POP_TS();
									POP_TE();
								};

		( "'" [\\] ([bnrt\']|([0-9]+))"'" )		{
									PUSH_TE();
									PUSH_TS();
									++ts; --te;
									ADVANCE( char );
									POP_TS();
									POP_TE();
								};

		( '"' ( [^"\\\n] | /\\./ )* '"' )		{
									PUSH_TE();
									PUSH_TS();
									++ts; --te;
									ADVANCE( string );
									POP_TS();
									POP_TE();
								};

		# Identifiers
		( [a-zA-Z_] [a-zA-Z0-9_]* )			{ ADVANCE( symbol );};
		( op_char op_char* )				{ ADVANCE( symbol );};

		# Floating literals.
		( [+\-]? fract_const exponent? | [+\-]? digit+ exponent ) 	{ ADVANCE( real64 );};


		# Integer decimal. Leading part buffered by float.
		( [+\-]? ( '0' | [1-9] [0-9]* ) )		{ ADVANCE( sint64 ); };

		( [+\-]? ( '0' | [1-9] [0-9]* ) [a-zA-Z_]+ )	{
									fprintf(stderr, "Error: invalid number:\n    ");

									for( const char* i = ts; i < te; ++i ) {
										fprintf(stderr, "%c", *i);
									}
									fprintf(stderr, "\n");
									cs	= scanner_error;
								};


		# Only buffer the second item, first buffered by symbol. */
		'('						{ ADVANCE_TOKEN( LPAR );};
		')'						{ ADVANCE_TOKEN( RPAR );};

		'\n'						{ ++line; };

		# Single char symbols.
		( punct - [_"'] )				{ printf("unexpected character %c\n", *ts); };

		# Comments and whitespace.
		'/*'						{ fgoto c_comment; };
		'//' [^\n]* '\n'@inc_line;
		( any - 33..126 )+;
	*|;
}%%

static uint32
copy_token(const char* ts, const char *te, char* dst) {
	uint32	index	= 0;
	while( ts < te ) {
		dst[index++]	= *ts;
		++ts;
	}
	dst[index] = '\0';
	return index;
}

static cell_t*
token_to_symbol(const char* b) {
	return cell_new_symbol(b);
}

static cell_t*
token_to_boolean(const char* b) {
	if( !strcmp(b, "true") ) {
		return cell_new_boolean(true);
	} else {
		return cell_new_boolean(false);
	}
}

static cell_t*
token_to_char(const char* b) {
	return cell_new_char(*b);
}

static cell_t*
token_to_sint64(const char* b) {
	sint64	v	= 0;
	sscanf(b, "%ld", &v);
	/* TODO: check limit */
	return cell_new_sint64(v);
}

static cell_t*
token_to_real64(const char* b) {
	real64	v	= 0;
	sscanf(b, "%lf", &v);
	/* TODO: check limit */
	return cell_new_real64(v);
}

static cell_t*
token_to_string(const char* b) {
	return cell_new_string(b);
}


scheme_t*
parse(const char* str)
{
	scheme_t*	state	= (scheme_t*)malloc(sizeof(scheme_t));
	void*		parser;
	size_t		line	= 1;
	const char*	ts	= str;
	const char*	te	= str;
	int		act	= 0;
	int		cs	= 0;
	char		tmp[4096];

	memset(state, 0, sizeof(scheme_t));

	parser	= parser_alloc(malloc);

	memset(tmp, 0, sizeof(tmp));

	%% write init;

	const char*	p = str;
	const char*	pe = p + strlen(str) + 1;
	const char*	eof = 0;

	%% write exec;

	/* Check if we failed. */
	if ( cs == scanner_error ) {
		/* Machine failed before finding a token. */
		printf("PARSE ERROR\n");
		exit(1);
	}

	parser_advance(parser, 0, 0, state);

	parser_free(parser, free);

	return state;

}
