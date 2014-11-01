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

enum scheme_port_kind {
	port_free=0,
	port_file=1,
	port_string=2,
	port_srfi6=4,
	port_input=16,
	port_output=32,
	port_saw_EOF=64
};

typedef struct port_t {
	unsigned char kind;
	union {
		struct {
			FILE *file;
			int closeit;
#if SHOW_ERROR_LINE
			int curr_line;
			char *filename;
#endif
		} stdio;
		struct {
			char *start;
			char *past_the_end;
			char *curr;
		} string;
	} rep;
} port_t;

/* cell structure */
struct cell_t {
	unsigned int _flag;
	union {
		struct {
			char   *_svalue;
			int   _length;
		} _string;
		number_t _number;
		port_t *_port;
		foreign_func _ff;
		struct {
			cell_t *_car;
			cell_t *_cdr;
		} _cons;
	} _object;
};

struct scheme_t {
	/* arrays for segments */
	func_alloc malloc;
	func_dealloc free;

	/* return code */
	int retcode;
	int tracing;


#define CELL_SEGSIZE    5000  /* # of cells in one segment */
#define CELL_NSEGMENT   10    /* # of segments for cells */
	char *alloc_seg[CELL_NSEGMENT];
	cell_ptr_t cell_seg[CELL_NSEGMENT];
	int     last_cell_seg;

	/* We use 4 registers. */
	cell_ptr_t args;            /* register for arguments of function */
	cell_ptr_t envir;           /* stack register for current environment */
	cell_ptr_t code;            /* register for current code */
	cell_ptr_t dump;            /* stack register for next evaluation */

	int interactive_repl;    /* are we in an interactive REPL? */

	cell_t _sink;
	cell_ptr_t sink;            /* when mem. alloc. fails */
	cell_t _NIL;
	cell_ptr_t NIL;             /* special cell representing empty cell */
	cell_t _HASHT;
	cell_ptr_t T;               /* special cell representing #t */
	cell_t _HASHF;
	cell_ptr_t F;               /* special cell representing #f */
	cell_t _EOF_OBJ;
	cell_ptr_t EOF_OBJ;         /* special cell representing end-of-file object */
	cell_ptr_t oblist;          /* cell_ptr_t to symbol table */
	cell_ptr_t global_env;      /* cell_ptr_t to global environment */
	cell_ptr_t c_nest;          /* stack for nested calls from C */

	/* global pointers to special symbols */
	cell_ptr_t LAMBDA;               /* cell_ptr_t to syntax lambda */
	cell_ptr_t QUOTE;           /* cell_ptr_t to syntax quote */

	cell_ptr_t QQUOTE;               /* cell_ptr_t to symbol quasiquote */
	cell_ptr_t UNQUOTE;         /* cell_ptr_t to symbol unquote */
	cell_ptr_t UNQUOTESP;       /* cell_ptr_t to symbol unquote-splicing */
	cell_ptr_t FEED_TO;         /* => */
	cell_ptr_t COLON_HOOK;      /* *colon-hook* */
	cell_ptr_t ERROR_HOOK;      /* *error-hook* */
	cell_ptr_t SHARP_HOOK;  /* *sharp-hook* */
	cell_ptr_t COMPILE_HOOK;  /* *compile-hook* */

	cell_ptr_t free_cell;       /* cell_ptr_t to top of free cells */
	long    fcells;          /* # of free cells */

	cell_ptr_t inport;
	cell_ptr_t outport;
	cell_ptr_t save_inport;
	cell_ptr_t loadport;

#define MAXFIL 64
	port_t load_stack[MAXFIL];     /* Stack of open files for port -1 (LOADing) */
	int nesting_stack[MAXFIL];
	int file_i;
	int nesting;

	char    gc_verbose;      /* if gc_verbose is not zero, print gc status */
	char    no_memory;       /* Whether mem. alloc. has failed */

#define LINESIZE 1024
	char    linebuff[LINESIZE];
#define STRBUFFSIZE 256
	char    strbuff[STRBUFFSIZE];

	FILE *tmpfp;
	int tok;
	int print_flag;
	cell_ptr_t value;
	int op;

	void *ext_data;     /* For the benefit of foreign functions */
	long gensym_cnt;

	struct scheme_interface *vptr;
	void *dump_base;    /* cell_ptr_t to base of allocated dump stack */
	int dump_size;      /* number of frames allocated for dump stack */
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
    T_PORT=10,
    T_VECTOR=11,
    T_MACRO=12,
    T_PROMISE=13,
    T_ENVIRONMENT=14,
    T_LAST_SYSTEM_TYPE=14
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
#define typeflag(p)      ((p)->_flag)
#define type(p)          (typeflag(p)&T_MASKTYPE)

#define strvalue(p)      ((p)->_object._string._svalue)
#define strlength(p)     ((p)->_object._string._length)

#define setenvironment(p)    typeflag(p) = T_ENVIRONMENT

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

extern cell_ptr_t _s_return(scheme_t *sc, cell_ptr_t a);
#define s_return(sc,a) return _s_return(sc,a)

#define s_retbool(tf)    s_return(sc,(tf) ? sc->T : sc->F)

#define procnum(p)       ivalue(p)
const char *procname(cell_ptr_t x);

/*******************************************************************************
 *
 * number.c
 *
 ******************************************************************************/
extern number_t		__s_num_zero;
extern number_t		__s_num_one;

#define num_ivalue(n)       (n.is_integer?(n).value.ivalue:(long)(n).value.rvalue)
#define num_rvalue(n)       (!n.is_integer?(n).value.rvalue:(double)(n).value.ivalue)

long binary_decode(const char *s);

#define num_is_integer(p) ((p)->_object._number.is_integer)

#define ivalue_unchecked(p)       ((p)->_object._number.value.ivalue)
#define rvalue_unchecked(p)       ((p)->_object._number.value.rvalue)
#define set_num_integer(p)   (p)->_object._number.is_integer=1;
#define set_num_real(p)      (p)->_object._number.is_integer=0;

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
cell_ptr_t mk_port(scheme_t *sc, port_t *p);
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

/*******************************************************************************
 *
 * error.c
 *
 ******************************************************************************/
#define error_1(sc, s, a) return _error_1(sc, s, a)
#define error_0(sc, s)    return _error_1(sc, s, 0)

cell_ptr_t _error_1(scheme_t *sc, const char *s, cell_ptr_t a);

/*******************************************************************************
 *
 * frame.c
 *
 ******************************************************************************/
#define new_slot_in_env(sc, variable, value)	new_slot_spec_in_env(sc, sc->envir, variable, value)
#define set_slot_in_env(sc, slot, value)	cdr(slot) = value
#define slot_value_in_env(slot)			(cdr(slot))

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
int alloc_cellseg(scheme_t *sc, int n);
cell_ptr_t get_cell_x(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t _get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t reserve_cells(scheme_t *sc, int n);
cell_ptr_t get_consecutive_cells(scheme_t *sc, int n);
int count_consecutive_cells(cell_ptr_t x, int needed);
cell_ptr_t find_consecutive_cells(scheme_t *sc, int n);
void push_recent_alloc(scheme_t *sc, cell_ptr_t recent, cell_ptr_t extra);
cell_ptr_t get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t get_vector_object(scheme_t *sc, int len, cell_ptr_t init);

#if defined TSGRIND
void check_cell_alloced(cell_ptr_t p, int expect_alloced);
void check_range_alloced(cell_ptr_t p, int n, int expect_alloced);
#endif
cell_ptr_t _cons(scheme_t *sc, cell_ptr_t a, cell_ptr_t b, int immutable);
void finalize_cell(scheme_t *sc, cell_ptr_t a);

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
 * port.c
 *
 ******************************************************************************/

int file_push(scheme_t *sc, const char *fname);
void file_pop(scheme_t *sc);
int file_interactive(scheme_t *sc);
port_t *port_rep_from_filename(scheme_t *sc, const char *fn, int prop);
cell_ptr_t port_from_filename(scheme_t *sc, const char *fn, int prop);
port_t *port_rep_from_file(scheme_t *sc, FILE *f, int prop);
cell_ptr_t port_from_file(scheme_t *sc, FILE *f, int prop);
port_t *port_rep_from_string(scheme_t *sc, char *start, char *past_the_end, int prop);
cell_ptr_t port_from_string(scheme_t *sc, char *start, char *past_the_end, int prop);
port_t *port_rep_from_scratch(scheme_t *sc);
cell_ptr_t port_from_scratch(scheme_t *sc);
void port_close(scheme_t *sc, cell_ptr_t p, int flag);
int inchar(scheme_t *sc);
int basic_inchar(port_t *pt);
void backchar(scheme_t *sc, int c);
int realloc_port_string(scheme_t *sc, port_t *p);
void putstr(scheme_t *sc, const char *s);
void putchars(scheme_t *sc, const char *s, int len);
void putcharacter(scheme_t *sc, int c);
int is_port(cell_ptr_t p);
int is_inport(cell_ptr_t p);
int is_outport(cell_ptr_t p);

/*******************************************************************************
 *
 *
 *
 ******************************************************************************/
#define cons(sc,a,b) _cons(sc,a,b,0)
#define immutable_cons(sc,a,b) _cons(sc,a,b,1)

int is_string(cell_ptr_t p);
char *string_value(cell_ptr_t p);
int is_number(cell_ptr_t p);
number_t nvalue(cell_ptr_t p);
long ivalue(cell_ptr_t p);
double rvalue(cell_ptr_t p);
int is_integer(cell_ptr_t p);
int is_real(cell_ptr_t p);
int is_character(cell_ptr_t p);
long charvalue(cell_ptr_t p);
int is_vector(cell_ptr_t p);

int is_pair(cell_ptr_t p);
cell_ptr_t pair_car(cell_ptr_t p);
cell_ptr_t pair_cdr(cell_ptr_t p);
cell_ptr_t set_car(cell_ptr_t p, cell_ptr_t q);
cell_ptr_t set_cdr(cell_ptr_t p, cell_ptr_t q);

int is_symbol(cell_ptr_t p);
char *symname(cell_ptr_t p);
int hasprop(cell_ptr_t p);

int is_syntax(cell_ptr_t p);
int is_proc(cell_ptr_t p);
int is_foreign(cell_ptr_t p);
char *syntaxname(cell_ptr_t p);
int is_closure(cell_ptr_t p);
int is_macro(cell_ptr_t p);

cell_ptr_t closure_code(cell_ptr_t p);
cell_ptr_t closure_env(cell_ptr_t p);

int is_continuation(cell_ptr_t p);
int is_promise(cell_ptr_t p);
int is_environment(cell_ptr_t p);
int is_immutable(cell_ptr_t p);
void setimmutable(cell_ptr_t p);

#define car(p)           ((p)->_object._cons._car)
#define cdr(p)           ((p)->_object._cons._cdr)

#define caar(p)          car(car(p))
#define cadr(p)          car(cdr(p))
#define cdar(p)          cdr(car(p))
#define cddr(p)          cdr(cdr(p))
#define cadar(p)         car(cdr(car(p)))
#define caddr(p)         car(cdr(cdr(p)))
#define cdaar(p)         cdr(car(car(p)))
#define cadaar(p)        car(cdr(car(car(p))))
#define cadddr(p)        car(cdr(cdr(cdr(p))))
#define cddddr(p)        cdr(cdr(cdr(cdr(p))))

#ifdef __cplusplus
}
#endif

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
