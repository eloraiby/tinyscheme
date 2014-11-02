
#line 1 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
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


#line 39 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.c"
static const char _scanner_actions[] = {
	0, 1, 0, 1, 1, 1, 2, 1, 
	3, 1, 4, 1, 14, 1, 15, 1, 
	16, 1, 17, 1, 18, 1, 19, 1, 
	21, 1, 22, 1, 23, 1, 24, 1, 
	25, 1, 26, 1, 27, 1, 28, 1, 
	29, 1, 30, 1, 31, 2, 0, 20, 
	2, 4, 5, 2, 4, 6, 2, 4, 
	7, 2, 4, 8, 2, 4, 9, 2, 
	4, 10, 2, 4, 11, 2, 4, 12, 
	2, 4, 13
};

static const short _scanner_key_offsets[] = {
	0, 0, 3, 3, 4, 11, 12, 15, 
	17, 21, 23, 28, 29, 31, 34, 71, 
	73, 86, 88, 105, 109, 111, 121, 126, 
	135, 145, 147, 160, 174, 181, 189, 197, 
	205, 213, 221, 229, 237, 245, 253, 261, 
	269
};

static const char _scanner_trans_keys[] = {
	10, 34, 92, 39, 39, 98, 110, 114, 
	116, 48, 57, 39, 39, 48, 57, 48, 
	57, 43, 45, 48, 57, 48, 57, 46, 
	69, 101, 48, 57, 10, 10, 42, 10, 
	42, 47, 10, 33, 34, 39, 40, 41, 
	44, 46, 47, 48, 59, 64, 94, 95, 
	102, 113, 116, 124, 126, 35, 36, 37, 
	42, 43, 45, 49, 57, 58, 63, 65, 
	90, 91, 96, 97, 122, 123, 125, 33, 
	126, 33, 45, 47, 58, 94, 124, 126, 
	37, 38, 42, 43, 60, 63, 39, 92, 
	33, 46, 48, 58, 94, 124, 126, 37, 
	38, 42, 43, 45, 47, 49, 57, 60, 
	63, 69, 101, 48, 57, 48, 57, 46, 
	69, 95, 101, 48, 57, 65, 90, 97, 
	122, 95, 65, 90, 97, 122, 43, 45, 
	95, 48, 57, 65, 90, 97, 122, 46, 
	69, 95, 101, 48, 57, 65, 90, 97, 
	122, 48, 57, 33, 45, 47, 58, 94, 
	124, 126, 37, 38, 42, 43, 60, 63, 
	10, 33, 45, 47, 58, 94, 124, 126, 
	37, 38, 42, 43, 60, 63, 95, 48, 
	57, 65, 90, 97, 122, 95, 97, 48, 
	57, 65, 90, 98, 122, 95, 108, 48, 
	57, 65, 90, 97, 122, 95, 115, 48, 
	57, 65, 90, 97, 122, 95, 101, 48, 
	57, 65, 90, 97, 122, 95, 117, 48, 
	57, 65, 90, 97, 122, 95, 111, 48, 
	57, 65, 90, 97, 122, 95, 116, 48, 
	57, 65, 90, 97, 122, 95, 101, 48, 
	57, 65, 90, 97, 122, 95, 114, 48, 
	57, 65, 90, 97, 122, 95, 117, 48, 
	57, 65, 90, 97, 122, 95, 101, 48, 
	57, 65, 90, 97, 122, 0
};

static const char _scanner_single_lengths[] = {
	0, 3, 0, 1, 5, 1, 1, 0, 
	2, 0, 3, 1, 2, 3, 19, 0, 
	7, 2, 7, 2, 0, 4, 1, 3, 
	4, 0, 7, 8, 1, 2, 2, 2, 
	2, 2, 2, 2, 2, 2, 2, 2, 
	0
};

static const char _scanner_range_lengths[] = {
	0, 0, 0, 0, 1, 0, 1, 1, 
	1, 1, 1, 0, 0, 0, 9, 1, 
	3, 0, 5, 1, 1, 3, 2, 3, 
	3, 1, 3, 3, 3, 3, 3, 3, 
	3, 3, 3, 3, 3, 3, 3, 3, 
	0
};

static const short _scanner_index_offsets[] = {
	0, 0, 4, 5, 7, 14, 16, 19, 
	21, 25, 27, 32, 34, 37, 41, 70, 
	72, 83, 86, 99, 103, 105, 113, 117, 
	124, 132, 134, 145, 157, 162, 168, 174, 
	180, 186, 192, 198, 204, 210, 216, 222, 
	228
};

static const char _scanner_indicies[] = {
	1, 2, 3, 0, 0, 5, 4, 6, 
	6, 6, 6, 6, 7, 4, 8, 4, 
	8, 7, 4, 10, 9, 12, 12, 13, 
	11, 13, 11, 10, 16, 16, 15, 14, 
	18, 17, 20, 21, 19, 20, 21, 22, 
	19, 24, 25, 0, 27, 28, 29, 26, 
	31, 32, 33, 26, 26, 25, 35, 36, 
	37, 38, 25, 25, 26, 25, 30, 34, 
	25, 35, 26, 35, 26, 23, 11, 23, 
	25, 25, 25, 25, 25, 25, 25, 25, 
	25, 25, 39, 40, 42, 41, 25, 43, 
	33, 25, 25, 25, 25, 25, 25, 25, 
	34, 25, 39, 16, 16, 10, 44, 13, 
	44, 10, 47, 46, 47, 15, 46, 46, 
	45, 46, 46, 46, 48, 12, 12, 46, 
	13, 46, 46, 48, 10, 47, 46, 47, 
	34, 46, 46, 45, 10, 49, 25, 25, 
	50, 25, 25, 25, 25, 25, 25, 25, 
	39, 18, 50, 50, 50, 50, 50, 50, 
	50, 50, 50, 50, 17, 35, 35, 35, 
	35, 11, 35, 52, 35, 35, 35, 51, 
	35, 53, 35, 35, 35, 51, 35, 54, 
	35, 35, 35, 51, 35, 55, 35, 35, 
	35, 51, 35, 56, 35, 35, 35, 51, 
	35, 57, 35, 35, 35, 51, 35, 58, 
	35, 35, 35, 51, 35, 59, 35, 35, 
	35, 51, 35, 60, 35, 35, 35, 51, 
	35, 61, 35, 35, 35, 51, 35, 62, 
	35, 35, 35, 51, 1, 0
};

static const char _scanner_trans_targs[] = {
	1, 0, 14, 2, 14, 14, 5, 6, 
	14, 14, 19, 14, 9, 20, 14, 10, 
	8, 11, 14, 12, 12, 13, 40, 15, 
	15, 16, 14, 17, 14, 14, 18, 25, 
	26, 21, 24, 28, 29, 33, 37, 14, 
	14, 3, 4, 7, 14, 14, 22, 23, 
	14, 14, 27, 14, 30, 31, 32, 28, 
	34, 35, 36, 28, 38, 39, 28
};

static const char _scanner_trans_actions[] = {
	0, 0, 15, 0, 37, 11, 0, 0, 
	13, 39, 60, 43, 0, 0, 41, 0, 
	0, 0, 45, 0, 1, 0, 3, 72, 
	69, 0, 21, 9, 17, 19, 9, 0, 
	0, 63, 0, 57, 0, 0, 0, 27, 
	23, 0, 0, 0, 29, 31, 0, 66, 
	33, 35, 9, 25, 0, 0, 0, 51, 
	0, 0, 0, 54, 0, 0, 48
};

static const char _scanner_to_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 5, 0, 5, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const char _scanner_from_state_actions[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 7, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0, 0, 0, 0, 0, 0, 0, 0, 
	0
};

static const short _scanner_eof_trans[] = {
	0, 0, 0, 5, 5, 5, 5, 10, 
	12, 12, 15, 10, 0, 0, 0, 12, 
	40, 41, 40, 45, 45, 46, 49, 49, 
	46, 50, 40, 40, 12, 52, 52, 52, 
	52, 52, 52, 52, 52, 52, 52, 52, 
	0
};

static const int scanner_start = 14;
static const int scanner_error = 0;

static const int scanner_en_c_comment = 12;
static const int scanner_en_main = 14;


#line 121 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"


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

	memset(state, 0, sizeof(state_t));

	parser	= parser_alloc(malloc);

	memset(tmp, 0, sizeof(tmp));

	
#line 288 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.c"
	{
	cs = scanner_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 194 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"

	const char*	p = str;
	const char*	pe = p + strlen(str) + 1;
	const char*	eof = 0;

	
#line 303 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.c"
	{
	int _klen;
	unsigned int _trans;
	const char *_acts;
	unsigned int _nacts;
	const char *_keys;

	if ( p == pe )
		goto _test_eof;
	if ( cs == 0 )
		goto _out;
_resume:
	_acts = _scanner_actions + _scanner_from_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 3:
#line 1 "NONE"
	{ts = p;}
	break;
#line 324 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.c"
		}
	}

	_keys = _scanner_trans_keys + _scanner_key_offsets[cs];
	_trans = _scanner_index_offsets[cs];

	_klen = _scanner_single_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + _klen - 1;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + ((_upper-_lower) >> 1);
			if ( (*p) < *_mid )
				_upper = _mid - 1;
			else if ( (*p) > *_mid )
				_lower = _mid + 1;
			else {
				_trans += (unsigned int)(_mid - _keys);
				goto _match;
			}
		}
		_keys += _klen;
		_trans += _klen;
	}

	_klen = _scanner_range_lengths[cs];
	if ( _klen > 0 ) {
		const char *_lower = _keys;
		const char *_mid;
		const char *_upper = _keys + (_klen<<1) - 2;
		while (1) {
			if ( _upper < _lower )
				break;

			_mid = _lower + (((_upper-_lower) >> 1) & ~1);
			if ( (*p) < _mid[0] )
				_upper = _mid - 2;
			else if ( (*p) > _mid[1] )
				_lower = _mid + 2;
			else {
				_trans += (unsigned int)((_mid - _keys)>>1);
				goto _match;
			}
		}
		_trans += _klen;
	}

_match:
	_trans = _scanner_indicies[_trans];
_eof_trans:
	cs = _scanner_trans_targs[_trans];

	if ( _scanner_trans_actions[_trans] == 0 )
		goto _again;

	_acts = _scanner_actions + _scanner_trans_actions[_trans];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 )
	{
		switch ( *_acts++ )
		{
	case 0:
#line 47 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{ ++line; }
	break;
	case 1:
#line 49 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{ {cs = 14; goto _again;} }
	break;
	case 4:
#line 1 "NONE"
	{te = p+1;}
	break;
	case 5:
#line 52 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 1;}
	break;
	case 6:
#line 53 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 2;}
	break;
	case 7:
#line 54 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 3;}
	break;
	case 8:
#line 86 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 8;}
	break;
	case 9:
#line 90 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 10;}
	break;
	case 10:
#line 94 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 11;}
	break;
	case 11:
#line 96 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 12;}
	break;
	case 12:
#line 111 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 15;}
	break;
	case 13:
#line 119 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{act = 19;}
	break;
	case 14:
#line 58 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;{
									PUSH_TE();
									PUSH_TS();
									++ts; --te;
									ADVANCE( char );
									POP_TS();
									POP_TE();
								}}
	break;
	case 15:
#line 67 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;{
									PUSH_TE();
									PUSH_TS();
									++ts; --te;
									ADVANCE( char );
									POP_TS();
									POP_TE();
								}}
	break;
	case 16:
#line 76 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;{
									PUSH_TE();
									PUSH_TS();
									++ts; --te;
									ADVANCE( string );
									POP_TS();
									POP_TE();
								}}
	break;
	case 17:
#line 108 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;{ ADVANCE_TOKEN( LPAR );}}
	break;
	case 18:
#line 109 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;{ ADVANCE_TOKEN( RPAR );}}
	break;
	case 19:
#line 114 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;{ printf("unexpected character %c\n", *ts); }}
	break;
	case 20:
#line 118 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p+1;}
	break;
	case 21:
#line 55 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{ ADVANCE_TOKEN( QUOTE ); }}
	break;
	case 22:
#line 86 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{ ADVANCE( symbol );}}
	break;
	case 23:
#line 87 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{ ADVANCE( symbol );}}
	break;
	case 24:
#line 90 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{ ADVANCE( real64 );}}
	break;
	case 25:
#line 94 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{ ADVANCE( sint64 ); }}
	break;
	case 26:
#line 96 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{
									fprintf(stderr, "Error: invalid number:\n    ");

									for( const char* i = ts; i < te; ++i ) {
										fprintf(stderr, "%c", *i);
									}
									fprintf(stderr, "\n");
									cs	= scanner_error;
								}}
	break;
	case 27:
#line 114 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{te = p;p--;{ printf("unexpected character %c\n", *ts); }}
	break;
	case 28:
#line 55 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{{p = ((te))-1;}{ ADVANCE_TOKEN( QUOTE ); }}
	break;
	case 29:
#line 87 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{{p = ((te))-1;}{ ADVANCE( symbol );}}
	break;
	case 30:
#line 94 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"
	{{p = ((te))-1;}{ ADVANCE( sint64 ); }}
	break;
	case 31:
#line 1 "NONE"
	{	switch( act ) {
	case 1:
	{{p = ((te))-1;} ADVANCE( boolean );}
	break;
	case 2:
	{{p = ((te))-1;} ADVANCE( boolean );}
	break;
	case 3:
	{{p = ((te))-1;} ADVANCE_TOKEN( QUOTE ); }
	break;
	case 8:
	{{p = ((te))-1;} ADVANCE( symbol );}
	break;
	case 10:
	{{p = ((te))-1;} ADVANCE( real64 );}
	break;
	case 11:
	{{p = ((te))-1;} ADVANCE( sint64 ); }
	break;
	case 12:
	{{p = ((te))-1;}
									fprintf(stderr, "Error: invalid number:\n    ");

									for( const char* i = ts; i < te; ++i ) {
										fprintf(stderr, "%c", *i);
									}
									fprintf(stderr, "\n");
									cs	= scanner_error;
								}
	break;
	case 15:
	{{p = ((te))-1;} ++line; }
	break;
	default:
	{{p = ((te))-1;}}
	break;
	}
	}
	break;
#line 576 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.c"
		}
	}

_again:
	_acts = _scanner_actions + _scanner_to_state_actions[cs];
	_nacts = (unsigned int) *_acts++;
	while ( _nacts-- > 0 ) {
		switch ( *_acts++ ) {
	case 2:
#line 1 "NONE"
	{ts = 0;}
	break;
#line 589 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.c"
		}
	}

	if ( cs == 0 )
		goto _out;
	if ( ++p != pe )
		goto _resume;
	_test_eof: {}
	if ( p == eof )
	{
	if ( _scanner_eof_trans[cs] > 0 ) {
		_trans = _scanner_eof_trans[cs] - 1;
		goto _eof_trans;
	}
	}

	_out: {}
	}

#line 200 "/home/kin/Projects/tinyscheme141/langs/r4rs/lexer.rl"

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
