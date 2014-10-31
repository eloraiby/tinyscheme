/* scheme_t-private.h */

#ifndef _SCHEME_PRIVATE_H
#define _SCHEME_PRIVATE_H

#include "scheme.h"
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

int is_port(cell_ptr_t p);

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
#ifdef USE_MACRO
int is_macro(cell_ptr_t p);
#endif
cell_ptr_t closure_code(cell_ptr_t p);
cell_ptr_t closure_env(cell_ptr_t p);

int is_continuation(cell_ptr_t p);
int is_promise(cell_ptr_t p);
int is_environment(cell_ptr_t p);
int is_immutable(cell_ptr_t p);
void setimmutable(cell_ptr_t p);

#ifdef __cplusplus
}
#endif

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
