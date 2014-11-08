/* scheme_t-private.h */

#ifndef _SCHEME_PRIVATE_H
#define _SCHEME_PRIVATE_H

#include "scheme.h"

#if USE_STRCASECMP
#include <strings.h>
# ifndef __APPLE__
#  define stricmp strcasecmp
# endif
#endif

#ifndef WIN32
# include <unistd.h>
#endif
#ifdef WIN32
#define snprintf _snprintf
#endif
#if USE_DL
# include "dynload.h"
#endif
#if USE_MATH
# include <math.h>
#endif

#include <limits.h>
#include <float.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

/* Used for documentation purposes, to signal functions in 'interface' */
#define INTERFACE

/*------------------ Ugly internals -----------------------------------*/
/*------------------ Of interest only to FFI users --------------------*/

#ifdef __cplusplus
extern "C" {
#endif

/* cell structure */
struct cell_t {
	unsigned short flag;
	union {
		struct {
			unsigned int	length;
			char*		svalue;
		} string;
		number_t number;
		foreign_func ff;
		struct {
			cell_ptr_t	pcar;
			cell_ptr_t	pcdr;
		} ocons;
	} object;
};

/* predefined/special cell positions */
typedef enum SEPCIAL_CELL {
	SPCELL_NIL		= 0,		/* special cell representing empty cell */
	SPCELL_TRUE		= 1,		/* special cell representing #t */
	SPCELL_FALSE		= 2,		/* special cell representing #f */
	SPCELL_EOF_OBJ		= 3,		/* special cell representing end-of-file object */

	/* global pointers to special symbols */
	SPCELL_LAMBDA		= 4,		/* cell_ptr_t to syntax lambda */
	SPCELL_QUOTE		= 5,		/* cell_ptr_t to syntax quote */
	SPCELL_QQUOTE		= 6,		/* cell_ptr_t to symbol quasiquote */
	SPCELL_UNQUOTE		= 7,		/* cell_ptr_t to symbol unquote */
	SPCELL_UNQUOTESP	= 8,		/* cell_ptr_t to symbol unquote-splicing */
	SPCELL_FEED_TO		= 9,		/* => */
	SPCELL_COLON_HOOK	= 10,		/* *colon-hook* */
	SPCELL_ERROR_HOOK	= 11,		/* *error-hook* */
	SPCELL_SHARP_HOOK	= 12,		/* *sharp-hook* */
	SPCELL_COMPILE_HOOK	= 13,		/* *compile-hook* */

	SPCELL_SINK		= 14,

	SPCELL_LAST		= 14		/* last special cell */
} SPECIAL_CELL;

struct scheme_t {
	/* arrays for segments */
	func_alloc		malloc;
	func_dealloc		free;

	/* return code */
	int			retcode;
	int			tracing;


#define CELL_MAX_COUNT    512 * 1024  /* # of cells in the memory segment */
	cell_t			cell_seg[CELL_MAX_COUNT];

	/* We use 4 registers. */
	cell_ptr_t		args;		/* register for arguments of function */
	cell_ptr_t		envir;		/* stack register for current environment */
	cell_ptr_t		code;		/* register for current code */
	cell_ptr_t		dump;		/* stack register for next evaluation */

	cell_ptr_t		sink;		/* when mem. alloc. fails */
	cell_ptr_t		oblist;		/* cell_ptr_t to symbol table */
	cell_ptr_t		global_env;	/* cell_ptr_t to global environment */
	cell_ptr_t		c_nest;          /* stack for nested calls from C */


	cell_ptr_t		free_cell;       /* cell_ptr_t to top of free cells */
	uint32			fcells;          /* # of free cells */

	const char*		loaded_file;
	const char*		file_position;

#define MAXFIL 64
	bool			gc_verbose;      /* if gc_verbose is not zero, print gc status */
	bool			no_memory;       /* Whether mem. alloc. has failed */

#define LINESIZE 1024
	char			linebuff[LINESIZE];
#define STRBUFFSIZE 256
	char			strbuff[STRBUFFSIZE];

	number_t		num_zero;
	number_t		num_one;


	FILE*			tmpfp;
	int			tok;
	int			print_flag;
	cell_ptr_t		value;
	int			op;

	void*			ext_data;     /* For the benefit of foreign functions */
	long			gensym_cnt;

	void*			dump_base;    /* cell_ptr_t to base of allocated dump stack */
	int			dump_size;      /* number of frames allocated for dump stack */
};


/* operator code */
enum scheme_opcodes {
#define _OP_DEF(A,B,C,D,E,OP) OP,
#include "opdefines.h"
    OP_MAXDEFINED
};

/*******************************************************************************
 *
 * General purpose helpers
 *
 ******************************************************************************/
enum scheme_types {
    T_STRING=1,
    T_NUMBER=2,
    T_SYMBOL=3,
    T_PROC=4,
    T_PAIR=5,
    T_CLOSURE=6,
    T_CONTINUATION=7,
    T_FOREIGN=8,
    T_CHARACTER=9,
    T_VECTOR=10,
    T_MACRO=11,
    T_PROMISE=12,
    T_ENVIRONMENT=13,
    T_LAST_SYSTEM_TYPE=13
};

/* ADJ is enough slack to align cells in a TYPE_BITS-bit boundary */
#define ADJ		32
#define TYPE_BITS	 5
#define T_MASKTYPE      31    /* 0000000000011111 */
#define T_SYNTAX      4096    /* 0001000000000000 */
#define T_IMMUTABLE   8192    /* 0010000000000000 */
#define T_ATOM       16384    /* 0100000000000000 */   /* only for gc */
#define CLRATOM      49151    /* 1011111111111111 */   /* only for gc */
#define MARK         32768    /* 1000000000000000 */
#define UNMARK       32767    /* 0111111111111111 */

/* macros for cell operations */
#define ptr_typeflag(sc, p)	(cell_ptr_to_cell(sc, p)->flag)
#define ptr_type(sc, p)		(ptr_typeflag(sc, p) & T_MASKTYPE)

#define strvalue(sc, p)		(cell_ptr_to_cell(sc, p)->object.string.svalue)
#define strlength(sc, p)	(cell_ptr_to_cell(sc, p)->object.string.length)

#define setenvironment(sc, p)	ptr_typeflag(sc, p) = T_ENVIRONMENT

#define is_atom(p)       (typeflag(p)&T_ATOM)
#define setatom(p)       typeflag(p) |= T_ATOM
#define clratom(p)       typeflag(p) &= CLRATOM

#define is_mark(p)       (typeflag(p)&MARK)
#define setmark(p)       typeflag(p) |= MARK
#define clrmark(p)       typeflag(p) &= UNMARK

/* true or false value macro */
/* () is #t in R5RS */
#define is_true(p)       ((p) != sc->F)
#define is_false(p)      ((p) == sc->F)

/* Too small to turn into function */
# define  BEGIN     do {
# define  END  } while (0)
#define s_goto(sc,a) BEGIN                                  \
	sc->op = (int)(a);                                  \
	return sc->T; END

cell_ptr_t			_s_return(scheme_t *sc, cell_ptr_t a);
#define s_return(sc,a)		return _s_return(sc,a)

#define s_retbool(tf)		s_return(sc,(tf) ? sc->T : sc->F)

#define procnum(sc, p)		ivalue(sc, p)
const char*			procname(scheme_t* sc, cell_ptr_t x);

/*******************************************************************************
 *
 * number.c
 *
 ******************************************************************************/
#define num_ivalue(n)		(n.is_integer ? (n).value.ivalue : (long)(n).value.rvalue)
#define num_rvalue(n)		(!n.is_integer ? (n).value.rvalue : (double)(n).value.ivalue)

long binary_decode(const char *s);

#define num_is_integer(sc, p)	(cell_ptr_to_cell(sc, p)->object.number.is_integer)

#define ivalue_unchecked(sc, p)	(cell_ptr_to_cell(sc, p)->object.number.value.ivalue)
#define rvalue_unchecked(sc, p)	(cell_ptr_to_cell(sc, p)->object.number.value.rvalue)
#define set_num_integer(sc, p)	cell_ptr_to_cell(sc, p)->object.number.is_integer	= 1;
#define set_num_real(sc, p)	cell_ptr_to_cell(sc, p)->object.number.is_integer	= 0;

cell_ptr_t op_number(scheme_t *sc, enum scheme_opcodes op);
/*******************************************************************************
 *
 * predicate.c
 *
 ******************************************************************************/
int eqv(cell_ptr_t a, cell_ptr_t b);
cell_ptr_t op_predicate(scheme_t *sc, enum scheme_opcodes op);

/*******************************************************************************
 *
 * atoms.c
 *
 ******************************************************************************/
cell_ptr_t oblist_initial_value(scheme_t *sc);
cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name);
cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name);
cell_ptr_t oblist_all_symbols(scheme_t *sc);
cell_ptr_t mk_foreign_func(scheme_t *sc, foreign_func f);
cell_ptr_t mk_character(scheme_t *sc, int c);
cell_ptr_t mk_integer(scheme_t *sc, long number_t);
cell_ptr_t mk_real(scheme_t *sc, double n);
char *store_string(scheme_t *sc, int len_str, const char *str, char fill);
cell_ptr_t mk_string(scheme_t *sc, const char *str);
cell_ptr_t mk_counted_string(scheme_t *sc, const char *str, int len);
cell_ptr_t mk_empty_string(scheme_t *sc, int len, char fill);
cell_ptr_t mk_vector(scheme_t *sc, int len);
void fill_vector(cell_ptr_t vec, cell_ptr_t obj);
cell_ptr_t vector_elem(cell_ptr_t vec, int ielem);
cell_ptr_t set_vector_elem(cell_ptr_t vec, int ielem, cell_ptr_t a);
cell_ptr_t mk_symbol(scheme_t *sc, const char *name);
cell_ptr_t gensym(scheme_t *sc);
cell_ptr_t mk_atom(scheme_t *sc, char *q);
cell_ptr_t mk_sharp_const(scheme_t *sc, char *name);
void atom2str(scheme_t *sc, cell_ptr_t l, int f, char **pp, int *plen);
void printatom(scheme_t *sc, cell_ptr_t l, int f);

cell_ptr_t op_atom(scheme_t *sc, enum scheme_opcodes op);
/*******************************************************************************
 *
 * error.c
 *
 ******************************************************************************/
#define error_1(sc, s, a) return _error_1(sc, s, a)
#define error_0(sc, s)    return _error_1(sc, s, cell_ptr(0))

cell_ptr_t _error_1(scheme_t *sc, const char *s, cell_ptr_t a);

/*******************************************************************************
 *
 * frame.c
 *
 ******************************************************************************/
#define new_slot_in_env(sc, variable, value)	new_slot_spec_in_env(sc, sc->envir, variable, value)
#define set_slot_in_env(sc, slot, value)	cdr(sc, slot) = value
#define slot_value_in_env(sc, slot)		(cdr(sc, slot))

#if !defined(USE_ALIST_ENV) || !defined(USE_OBJECT_LIST)
int hash_fn(const char *key, int table_size);
#endif

void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env);
cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t sym, int all);
void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env,	cell_ptr_t variable, cell_ptr_t value);
cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all);

/*******************************************************************************
 *
 * cell.c
 *
 ******************************************************************************/
int			alloc_cellseg(scheme_t *sc);
cell_ptr_t		get_cell_x(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t		_get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t		reserve_cells(scheme_t *sc, int n);
cell_ptr_t		get_consecutive_cells(scheme_t *sc, int n);
int			count_consecutive_cells(scheme_t* sc, cell_ptr_t x, int needed);
cell_ptr_t		find_consecutive_cells(scheme_t *sc, int n);
void			push_recent_alloc(scheme_t *sc, cell_ptr_t recent, cell_ptr_t extra);
cell_ptr_t		get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t		get_vector_object(scheme_t *sc, int len, cell_ptr_t init);

#if defined TSGRIND
void check_cell_alloced(cell_ptr_t p, int expect_alloced);
void check_range_alloced(cell_ptr_t p, int n, int expect_alloced);
#endif
cell_ptr_t _cons(scheme_t *sc, cell_ptr_t a, cell_ptr_t b, int immutable);
void finalize_cell(scheme_t *sc, cell_ptr_t a);

cell_ptr_t list_star(scheme_t *sc, cell_ptr_t d);
cell_ptr_t reverse(scheme_t *sc, cell_ptr_t a);
cell_ptr_t reverse_in_place(scheme_t *sc, cell_ptr_t term, cell_ptr_t list);
cell_ptr_t revappend(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);

/*******************************************************************************
 *
 * stack.c
 *
 ******************************************************************************/
void dump_stack_reset(scheme_t *sc);
void dump_stack_initialize(scheme_t *sc);
void dump_stack_free(scheme_t *sc);
cell_ptr_t _s_return(scheme_t *sc, cell_ptr_t a);
void s_save(scheme_t *sc, enum scheme_opcodes op, cell_ptr_t args, cell_ptr_t code);
void dump_stack_mark(scheme_t *sc);

/*******************************************************************************
 *
 * gc.c
 *
 ******************************************************************************/
void mark(cell_ptr_t a);
void gc(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);

/*******************************************************************************
 *
 * parse.c
 *
 ******************************************************************************/
cell_ptr_t op_parse(scheme_t *sc, enum scheme_opcodes op);

/*******************************************************************************
 *
 * eval.c
 *
 ******************************************************************************/
cell_ptr_t op_eval(scheme_t *sc, enum scheme_opcodes op);

/*******************************************************************************
 *
 * ioctl.c
 *
 ******************************************************************************/
cell_ptr_t op_ioctl(scheme_t *sc, enum scheme_opcodes op);


/*******************************************************************************
 *
 *
 *
 ******************************************************************************/
#define cons(sc, a, b) _cons(sc, a, b, 0)
#define immutable_cons(sc, a, b) _cons(sc, a, b, 1)

bool		is_string(scheme_t* sc, cell_ptr_t p);
char*		string_value(scheme_t* sc, cell_ptr_t p);
bool		is_number(scheme_t* sc, cell_ptr_t p);
number_t	nvalue(scheme_t* sc, cell_ptr_t p);
long		ivalue(scheme_t* sc, cell_ptr_t p);
double		rvalue(scheme_t* sc, cell_ptr_t p);
bool		is_integer(scheme_t* sc, cell_ptr_t p);
bool		is_real(scheme_t* sc, cell_ptr_t p);
bool		is_character(scheme_t* sc, cell_ptr_t p);
long		charvalue(scheme_t* sc, cell_ptr_t p);
bool		is_vector(scheme_t* sc, cell_ptr_t p);
bool		is_pair(scheme_t* sc, cell_ptr_t p);
cell_ptr_t	pair_car(scheme_t* sc, cell_ptr_t p);
cell_ptr_t	pair_cdr(scheme_t* sc, cell_ptr_t p);
cell_ptr_t	set_car(scheme_t* sc, cell_ptr_t p, cell_ptr_t q);
cell_ptr_t	set_cdr(scheme_t* sc, cell_ptr_t p, cell_ptr_t q);

bool		is_symbol(scheme_t* sc, cell_ptr_t p);
char*		symname(scheme_t* sc, cell_ptr_t p);
bool		hasprop(scheme_t* sc, cell_ptr_t p);

bool		is_proc(scheme_t* sc, cell_ptr_t p);
bool		is_foreign(scheme_t* sc, cell_ptr_t p);
char*		syntaxname(scheme_t* sc, cell_ptr_t p);
bool		is_closure(scheme_t* sc, cell_ptr_t p);
bool		is_macro(scheme_t* sc, cell_ptr_t p);

cell_ptr_t	closure_code(scheme_t* sc, cell_ptr_t p);
cell_ptr_t	closure_env(scheme_t* sc, cell_ptr_t p);

bool		is_continuation(scheme_t* sc, cell_ptr_t p);
bool		is_promise(scheme_t* sc, cell_ptr_t p);
bool		is_environment(scheme_t* sc, cell_ptr_t p);
bool		is_immutable(scheme_t* sc, cell_ptr_t p);
void		setimmutable(scheme_t* sc, cell_ptr_t p);

#define car(sc, p)	(cell_ptr_to_cell(sc, p)->object.ocons.pcar)
#define cdr(sc, p)	(cell_ptr_to_cell(sc, p)->object.ocons.pcdr)

#define caar(sc, p)	car(sc, car(sc, p))
#define cadr(sc, p)	car(sc, cdr(sc, p))
#define cdar(sc, p)	cdr(sc, car(sc, p))
#define cddr(sc, p)	cdr(sc, cdr(sc, p))
#define cadar(sc, p)	car(sc, cdr(sc, car(sc, p)))
#define caddr(sc, p)	car(sc, cdr(sc, cdr(sc, p)))
#define cdaar(sc, p)	cdr(sc, car(sc, car(sc, p)))
#define cadaar(sc, p)	car(sc, cdr(sc, car(sc, car(sc, p))))
#define cadddr(sc, p)	car(sc, cdr(sc, cdr(sc, cdr(sc, p))))
#define cddddr(sc, p)	cdr(sc, cdr(sc, cdr(sc, cdr(sc, p))))

#ifdef __cplusplus
}
#endif

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
