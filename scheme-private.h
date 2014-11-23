/* scheme_t-private.h */

#ifndef _SCHEME_PRIVATE_H
#define _SCHEME_PRIVATE_H

#include "scheme.h"

/*------------------ Ugly internals -----------------------------------*/
/*------------------ Of interest only to FFI users --------------------*/

enum scheme_types {
	T_STRING	= 1,
	T_NUMBER	= 2,
	T_SYMBOL	= 3,
	T_PROC		= 4,
	T_PAIR		= 5,
	T_CLOSURE	= 6,
	T_CONTINUATION	= 7,
	T_FOREIGN	= 8,
	T_CHARACTER	= 9,
	T_PORT		= 10,
	T_VECTOR	= 11,
	T_MACRO		= 12,
	T_PROMISE	= 13,
	T_ENVIRONMENT	= 14,
	T_LAST_SYSTEM_TYPE = 14
};

/* ADJ is enough slack to align cells in a TYPE_BITS-bit boundary */
#define ADJ		32
#define TYPE_BITS	5
#define T_MASKTYPE      31    /* 0000000000011111 */
#define T_SYNTAX      4096    /* 0001000000000000 */
#define T_IMMUTABLE   8192    /* 0010000000000000 */
#define T_ATOM       16384    /* 0100000000000000 */   /* only for gc */
#define CLRATOM      49151    /* 1011111111111111 */   /* only for gc */
#define MARK         32768    /* 1000000000000000 */
#define UNMARK       32767    /* 0111111111111111 */

/* Used for documentation purposes, to signal functions in 'interface' */
#define INTERFACE

#ifdef __cplusplus
extern "C" {
#endif

enum scheme_port_kind {
	port_free	= 0,
	port_file	= 1,
	port_string	= 2,
	port_srfi6	= 4,
	port_input	= 16,
	port_output	= 32,
	port_saw_EOF	= 64
};

typedef struct {
	unsigned char value;
} port_kind_t;

typedef struct port_t {
	port_kind_t	kind;
	union {
		struct {
			FILE*	file;
			int	closeit;
#if SHOW_ERROR_LINE
			int	curr_line;
			char*	filename;
#endif
		} stdio;
		struct {
			char*	start;
			char*	past_the_end;
			char*	curr;
		} string;
	} rep;
} port_t;

/* cell structure */
struct cell_t {
	unsigned int flag;
	union {
		struct {
			char*		svalue;
			unsigned int	length;
		} string;
		number_t	number;
		port_t*		port;
		foreign_func	ff;
		struct {
			cell_t*		head;
			cell_t*		tail;
		} pair;
	} object;
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
	struct {
		char*		alloc_seg[CELL_NSEGMENT];
		cell_ptr_t	cell_seg[CELL_NSEGMENT];
		int		last_cell_seg;
		cell_ptr_t	free_cell;       /* cell_ptr_t to top of free cells */
		int		fcells;          /* # of free cells */

		bool		gc_verbose;      /* if gc_verbose is not zero, print gc status */
		bool		no_memory;       /* Whether mem. alloc. has failed */
	} memory;

	/* We use 4 registers. */
	struct {
		cell_ptr_t args;            /* register for arguments of function */
		cell_ptr_t envir;           /* stack register for current environment */
		cell_ptr_t code;            /* register for current code */
		cell_ptr_t dump;            /* stack register for next evaluation */
	} regs;

	int interactive_repl;    /* are we in an interactive REPL? */

	number_t num_zero;
	number_t num_one;

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


	cell_ptr_t inport;
	cell_ptr_t outport;
	cell_ptr_t save_inport;
	cell_ptr_t loadport;

#define MAXFIL 64
	port_t load_stack[MAXFIL];     /* Stack of open files for port -1 (LOADing) */
	int nesting_stack[MAXFIL];
	int file_i;
	int nesting;


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


#define cons(sc,a,b) _cons(sc,a,b,0)
#define immutable_cons(sc,a,b) _cons(sc,a,b,1)


number_t num_add(number_t a, number_t b);
number_t num_mul(number_t a, number_t b);
number_t num_div(number_t a, number_t b);
number_t num_intdiv(number_t a, number_t b);
number_t num_sub(number_t a, number_t b);
number_t num_rem(number_t a, number_t b);
number_t num_mod(number_t a, number_t b);
int num_eq(number_t a, number_t b);
int num_gt(number_t a, number_t b);
int num_ge(number_t a, number_t b);
int num_lt(number_t a, number_t b);
int num_le(number_t a, number_t b);

#if USE_MATH
double round_per_R5RS(double x);
#endif

INLINE int num_is_integer(cell_ptr_t p) {
	return ((p)->object.number.is_integer);
}

/* macros for cell operations */
#define typeflag(p)		((p)->flag)
#define type(p)			(typeflag(p) & T_MASKTYPE)

#define strvalue(p)		((p)->object.string.svalue)
#define strlength(p)		((p)->object.string.length)

#define ivalue_unchecked(p)	((p)->object.number.value.ivalue)
#define rvalue_unchecked(p)	((p)->object.number.value.rvalue)
#define set_num_integer(p)	(p)->object.number.is_integer = 1;
#define set_num_real(p)		(p)->object.number.is_integer = 0;

#define car(p)			((p)->object.pair.head)
#define cdr(p)			((p)->object.pair.tail)

#define s_retbool(tf)		s_return(sc, (tf) ? sc->T : sc->F)

#define procnum(p)		ivalue(p)

#define cont_dump(p)		cdr(p)

/* true or false value macro */
/* () is #t in R5RS */
#define is_true(p)		((p) != sc->F)
#define is_false(p)		((p) == sc->F)


#define is_atom(p)		(typeflag(p) & T_ATOM)
#define setatom(p)		typeflag(p) |= T_ATOM
#define clratom(p)		typeflag(p) &= CLRATOM

#define is_mark(p)		(typeflag(p) & MARK)
#define setmark(p)		typeflag(p) |= MARK
#define clrmark(p)		typeflag(p) &= UNMARK

#define caar(p)			car(car(p))
#define cadr(p)			car(cdr(p))
#define cdar(p)			cdr(car(p))
#define cddr(p)			cdr(cdr(p))
#define cadar(p)		car(cdr(car(p)))
#define caddr(p)		car(cdr(cdr(p)))
#define cdaar(p)		cdr(car(car(p)))
#define cadaar(p)		car(cdr(car(car(p))))
#define cadddr(p)		car(cdr(cdr(cdr(p))))
#define cddddr(p)		cdr(cdr(cdr(cdr(p))))

#define num_ivalue(n)		(n.is_integer ? (n).value.ivalue : (long)(n).value.rvalue)
#define num_rvalue(n)		(!n.is_integer ? (n).value.rvalue : (double)(n).value.ivalue)

#define error_1(sc,s, a)	return _error_1(sc, s, a)
#define error_0(sc,s)		return _error_1(sc, s, 0)

#define setenvironment(p)	typeflag(p) = T_ENVIRONMENT

/* Too small to turn into function */
# define  BEGIN			do {
# define  END			} while (0)

#define s_goto(sc,a)		BEGIN \
				sc->op = (int)(a); \
				return sc->T; END

#define s_return(sc,a)		return _s_return(sc,a)

cell_ptr_t	_s_return(scheme_t *sc, cell_ptr_t a);
void		s_save(scheme_t *sc, enum scheme_opcodes op, cell_ptr_t args, cell_ptr_t code);

int		hash_fn(const char *key, int table_size);
void		new_frame_in_env(scheme_t *sc, cell_ptr_t old_env);
cell_ptr_t	find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all);

cell_ptr_t	mk_proc(scheme_t *sc, enum scheme_opcodes op);
cell_ptr_t	mk_closure(scheme_t *sc, cell_ptr_t c, cell_ptr_t e);
cell_ptr_t	mk_continuation(scheme_t *sc, cell_ptr_t d);

cell_ptr_t	list_star(scheme_t *sc, cell_ptr_t d);
cell_ptr_t	reverse(scheme_t *sc, cell_ptr_t a);
cell_ptr_t	reverse_in_place(scheme_t *sc, cell_ptr_t term, cell_ptr_t list);
cell_ptr_t	revappend(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
cell_ptr_t	oblist_initial_value(scheme_t *sc);
cell_ptr_t	oblist_add_by_name(scheme_t *sc, const char *name);
cell_ptr_t	oblist_all_symbols(scheme_t *sc);
cell_ptr_t	oblist_find_by_name(scheme_t *sc, const char *name);

cell_ptr_t	mk_number(scheme_t *sc, number_t n);
char*		store_string(scheme_t *sc, int len, const char *str, char fill);
cell_ptr_t	mk_vector(scheme_t *sc, int len);
cell_ptr_t	mk_atom(scheme_t *sc, char *q);
cell_ptr_t	mk_sharp_const(scheme_t *sc, char *name);
cell_ptr_t	mk_port(scheme_t *sc, port_t *p);


void		printatom(scheme_t *sc, cell_ptr_t l, int f);
void		atom2str(scheme_t *sc, cell_ptr_t l, int f, char **pp, int *plen) ;
cell_ptr_t	get_vector_object(scheme_t *sc, int len, cell_ptr_t init);

void		fill_vector(cell_ptr_t vec, cell_ptr_t obj);
cell_ptr_t	vector_elem(cell_ptr_t vec, int ielem);
cell_ptr_t	set_vector_elem(cell_ptr_t vec, int ielem, cell_ptr_t a);


int		syntaxnum(cell_ptr_t p);
cell_ptr_t	_error_1(scheme_t *sc, const char *s, cell_ptr_t a);

/* ports */
cell_ptr_t	port_from_filename(scheme_t *sc, const char *fn, int prop);
cell_ptr_t	port_from_file(scheme_t *sc, FILE *, int prop);
cell_ptr_t	port_from_string(scheme_t *sc, char *start, char *past_the_end, int prop);
port_t*		port_rep_from_filename(scheme_t *sc, const char *fn, int prop);
port_t*		port_rep_from_file(scheme_t *sc, FILE *, int prop);
port_t*		port_rep_from_string(scheme_t *sc, char *start, char *past_the_end, int prop);
void		port_close(scheme_t *sc, cell_ptr_t p, int flag);
int		basic_inchar(port_t *pt);
int		inchar(scheme_t *sc);
void		backchar(scheme_t *sc, int c);
void		putchars(scheme_t *sc, const char *s, int len);
void		putcharacter(scheme_t *sc, int c);
port_t*		port_rep_from_scratch(scheme_t *sc);
cell_ptr_t	port_from_scratch(scheme_t *sc);


cell_ptr_t	get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
int		alloc_cellseg(scheme_t *sc, int n);
void		push_recent_alloc(scheme_t *sc, cell_ptr_t recent, cell_ptr_t extra);

void		gc(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);

cell_ptr_t	eval_op(scheme_t *sc, enum scheme_opcodes op);


INLINE int is_string(cell_ptr_t p)		{ return (type(p) == T_STRING); }
INLINE int is_list(scheme_t *sc, cell_ptr_t a)	{ return list_length(sc,a) >= 0; }
INLINE int is_vector(cell_ptr_t p)		{ return (type(p) == T_VECTOR); }
INLINE int is_number(cell_ptr_t p)		{ return (type(p)==T_NUMBER); }
INLINE int is_real(cell_ptr_t p)		{ return is_number(p) && (!(p)->object.number.is_integer); }
INLINE int is_character(cell_ptr_t p)		{ return (type(p) == T_CHARACTER); }
INLINE int is_port(cell_ptr_t p)		{ return (type(p) == T_PORT); }
INLINE int is_inport(cell_ptr_t p)		{ return is_port(p) && p->object.port->kind.value & port_input; }
INLINE int is_outport(cell_ptr_t p)		{ return is_port(p) && p->object.port->kind.value & port_output; }
INLINE int is_pair(cell_ptr_t p)		{ return (type(p) == T_PAIR); }
INLINE int is_symbol(cell_ptr_t p)		{ return (type(p) == T_SYMBOL); }
INLINE int is_syntax(cell_ptr_t p)		{ return (typeflag(p) & T_SYNTAX); }
INLINE int is_proc(cell_ptr_t p)		{ return (type(p) == T_PROC); }
INLINE int is_foreign(cell_ptr_t p)		{ return (type(p) == T_FOREIGN); }
INLINE int is_closure(cell_ptr_t p)		{ return (type(p) == T_CLOSURE); }
INLINE int is_macro(cell_ptr_t p)		{ return (type(p) == T_MACRO); }
INLINE int is_continuation(cell_ptr_t p)	{ return (type(p) == T_CONTINUATION); }
/* To do: promise should be forced ONCE only */
INLINE int is_promise(cell_ptr_t p)		{ return (type(p) == T_PROMISE); }
INLINE int is_environment(cell_ptr_t p)		{ return (type(p) == T_ENVIRONMENT); }

INLINE char *string_value(cell_ptr_t p)		{ return strvalue(p); }
INLINE number_t nvalue(cell_ptr_t p)		{ return ((p)->object.number); }
INLINE long ivalue(cell_ptr_t p)		{ return (num_is_integer(p) ? (p)->object.number.value.ivalue : (long)(p)->object.number.value.rvalue); }
INLINE double rvalue(cell_ptr_t p)		{ return (!num_is_integer(p) ? (p)->object.number.value.rvalue : (double)(p)->object.number.value.ivalue); }
INLINE long charvalue(cell_ptr_t p)		{ return ivalue_unchecked(p); }

INLINE int is_integer(cell_ptr_t p)		{ if (!is_number(p) ) return 0; return ( num_is_integer(p) || (double)ivalue(p) == rvalue(p) ) ? 1 : 0; }

INLINE cell_ptr_t pair_car(cell_ptr_t p)	{ return car(p); }
INLINE cell_ptr_t pair_cdr(cell_ptr_t p)	{ return cdr(p); }
INLINE cell_ptr_t set_car(cell_ptr_t p, cell_ptr_t q)	{ return car(p) = q; }
INLINE cell_ptr_t set_cdr(cell_ptr_t p, cell_ptr_t q)	{ return cdr(p) = q; }

INLINE char *symname(cell_ptr_t p)		{ return strvalue(car(p)); }

#if USE_PLIST
SCHEME_EXPORT INLINE int hasprop(cell_ptr_t p)     { return (typeflag(p)&T_SYMBOL); }
#define symprop(p)       cdr(p)
#endif

INLINE char *syntaxname(cell_ptr_t p)		{ return strvalue(car(p)); }
const char *procname(cell_ptr_t x);

INLINE cell_ptr_t closure_code(cell_ptr_t p)	{ return car(p); }
INLINE cell_ptr_t closure_env(cell_ptr_t p)	{ return cdr(p); }

INLINE int file_interactive(scheme_t *sc)	{ return sc->file_i == 0 && sc->load_stack[0].rep.stdio.file == stdin && sc->inport->object.port->kind.value & port_file; }

INLINE void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t variable, cell_ptr_t value)
{
	cell_ptr_t slot = immutable_cons(sc, variable, value);

	if (is_vector(car(env))) {
		int location = hash_fn(symname(variable), ivalue_unchecked(car(env)));

		set_vector_elem(car(env), location,
				immutable_cons(sc, slot, vector_elem(car(env), location)));
	} else {
		car(env) = immutable_cons(sc, slot, car(env));
	}
}

INLINE void new_slot_in_env(scheme_t *sc, cell_ptr_t variable, cell_ptr_t value) { new_slot_spec_in_env(sc, sc->regs.envir, variable, value); }

INLINE void set_slot_in_env(scheme_t* sc UNUSED, cell_ptr_t slot, cell_ptr_t value)	{ cdr(slot) = value; }

INLINE cell_ptr_t slot_value_in_env(cell_ptr_t slot)	{ return cdr(slot); }

#if USE_CHAR_CLASSIFIERS
INLINE int Cisalpha(int c)			{ return isascii(c) && isalpha(c); }
INLINE int Cisdigit(int c)			{ return isascii(c) && isdigit(c); }
INLINE int Cisspace(int c)			{ return isascii(c) && isspace(c); }
INLINE int Cisupper(int c)			{ return isascii(c) && isupper(c); }
INLINE int Cislower(int c)			{ return isascii(c) && islower(c); }
#endif

INTERFACE INLINE int is_immutable(cell_ptr_t p) { return (typeflag(p)&T_IMMUTABLE); }
/*#define setimmutable(p)  typeflag(p) |= T_IMMUTABLE*/
INTERFACE INLINE void setimmutable(cell_ptr_t p) { typeflag(p) |= T_IMMUTABLE; }


#ifdef __cplusplus
}
#endif

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
