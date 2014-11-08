/* T I N Y S C H E M E    1 . 4 1
 *   Dimitrios Souflis (dsouflis@acm.org)
 *   Based on MiniScheme (original credits follow)
 * (MINISCM)               coded by Atsushi Moriwaki (11/5/1989)
 * (MINISCM)           E-MAIL :  moriwaki@kurims.kurims.kyoto-u.ac.jp
 * (MINISCM) This version has been modified by R.C. Secrist.
 * (MINISCM)
 * (MINISCM) Mini-Scheme is now maintained by Akira KIDA.
 * (MINISCM)
 * (MINISCM) This is a revised and modified version by Akira KIDA.
 * (MINISCM)    current version is 0.85k4 (15 May 1994)
 *
 */

#define _SCHEME_SOURCE

#include "scheme-private.h"

/*
 *  Basic memory allocation units
 */

#define banner "TinyScheme 1.41"

#ifdef __APPLE__
static int stricmp(const char *s1, const char *s2) {
	unsigned char c1, c2;
	do {
		c1 = tolower(*s1);
		c2 = tolower(*s2);
		if (c1 < c2)
			return -1;
		else if (c1 > c2)
			return 1;
		s1++, s2++;
	} while (c1 != 0);
	return 0;
}
#endif /* __APPLE__ */

#ifndef InitFile
# define InitFile "init.scm"
#endif

#ifndef FIRST_CELLSEGS
# define FIRST_CELLSEGS 3
#endif


INTERFACE INLINE bool
is_string(scheme_t* sc,
	  cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_STRING);
}

INTERFACE INLINE bool
is_vector(scheme_t* sc,
	  cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_VECTOR);
}

INTERFACE INLINE bool
is_number(scheme_t* sc,
	  cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_NUMBER);
}

INTERFACE INLINE bool
is_integer(scheme_t* sc,
	   cell_ptr_t p)
{
	if( !is_number(sc, p) )
		return false;
	if( num_is_integer(sc, p) || (double)ivalue(sc, p) == rvalue(sc, p) )
		return true;
	return false;
}

INTERFACE INLINE bool
is_real(scheme_t* sc, cell_ptr_t p) {
	return is_number(sc, p) && !is_integer(sc, p);
}

INTERFACE INLINE bool
is_character(scheme_t* sc,
	     cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_CHARACTER);
}

INTERFACE INLINE char*
string_value(scheme_t* sc,
	     cell_ptr_t p)
{
	return strvalue(sc, p);
}

INLINE number_t
nvalue(scheme_t* sc,
       cell_ptr_t p)
{
	return (cell_ptr_to_cell(sc, p)->object.number);
}

INTERFACE long
ivalue(scheme_t* sc,
       cell_ptr_t p)
{
	return (num_is_integer(sc, p) ? cell_ptr_to_cell(sc, p)->object.number.value.ivalue : (long)cell_ptr_to_cell(sc, p)->object.number.value.rvalue);
}

INTERFACE double
rvalue(scheme_t* sc,
       cell_ptr_t p)
{
	return (!num_is_integer(sc, p) ? cell_ptr_to_cell(sc, p)->object.number.value.rvalue : (double)cell_ptr_to_cell(sc, p)->object.number.value.ivalue);
}

INTERFACE long
charvalue(scheme_t* sc,
	  cell_ptr_t p)
{
	return ivalue_unchecked(sc, p);
}

INTERFACE INLINE bool
is_pair(scheme_t* sc,
	cell_ptr_t p)
{
	return (ptr_type(sc, p)==T_PAIR);
}

INTERFACE cell_ptr_t
pair_car(scheme_t* sc,
	 cell_ptr_t p)
{
	return car(sc, p);
}

INTERFACE cell_ptr_t
pair_cdr(scheme_t* sc,
	 cell_ptr_t p)
{
	return cdr(sc, p);
}

INTERFACE cell_ptr_t
set_car(scheme_t* sc,
	cell_ptr_t p,
	cell_ptr_t q)
{
	return car(sc, p) = q;
}

INTERFACE cell_ptr_t
set_cdr(scheme_t* sc,
	cell_ptr_t p,
	cell_ptr_t q)
{
	return cdr(sc, p) = q;
}

INTERFACE INLINE bool
is_symbol(scheme_t* sc,
	  cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_SYMBOL);
}

INTERFACE INLINE char*
symname(scheme_t* sc,
	cell_ptr_t p)
{
	return strvalue(sc, car(sc, p));
}

#if USE_PLIST
SCHEME_EXPORT INLINE int hasprop(scheme_t* sc, cell_ptr_t p) {
	return (ptr_typeflag(p) & T_SYMBOL);
}
#define symprop(sc, p)       cdr(sc, p)
#endif

INTERFACE INLINE bool
is_proc(scheme_t* sc,
	cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_PROC);
}

INTERFACE INLINE bool
is_foreign(scheme_t* sc,
	   cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_FOREIGN);
}

INTERFACE INLINE char*
syntaxname(scheme_t* sc,
	   cell_ptr_t p)
{
	return strvalue(sc, car(sc, p));
}

INTERFACE INLINE bool
is_closure(scheme_t* sc,
	   cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_CLOSURE);
}

INTERFACE INLINE bool
is_macro(scheme_t* sc,
	 cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_MACRO);
}

INTERFACE INLINE cell_ptr_t
closure_code(scheme_t* sc,
	     cell_ptr_t p)
{
	return car(sc, p);
}

INTERFACE INLINE cell_ptr_t
closure_env(scheme_t* sc,
	    cell_ptr_t p)
{
	return cdr(sc, p);
}

INTERFACE INLINE bool
is_continuation(scheme_t* sc,
		cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_CONTINUATION);
}

/* To do: promise should be forced ONCE only */
INTERFACE INLINE bool
is_promise(scheme_t* sc,
	   cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_PROMISE);
}

INTERFACE INLINE bool
is_environment(scheme_t* sc,
	       cell_ptr_t p)
{
	return (ptr_type(sc, p) == T_ENVIRONMENT);
}

INTERFACE INLINE bool
is_immutable(scheme_t* sc,
	     cell_ptr_t p)
{
	return (ptr_typeflag(sc, p) & T_IMMUTABLE);
}

/*#define setimmutable(p)  typeflag(p) |= T_IMMUTABLE*/
INTERFACE INLINE void
setimmutable(scheme_t* sc,
	     cell_ptr_t p)
{
	ptr_typeflag(sc, p) |= T_IMMUTABLE;
}

static cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op);
static void Eval_Cycle(scheme_t *sc, enum scheme_opcodes op);
static void assign_syntax(scheme_t *sc, char *name);
static void assign_proc(scheme_t *sc, enum scheme_opcodes, char *name);

static INLINE void ok_to_freely_gc(scheme_t *sc) {
	car(sc, sc->sink) = cell_ptr(SPCELL_NIL);
}

/* Result is:
   proper list: length
   circular list: -1
   not even a pair: -2
   dotted list: -2 minus length before dot
*/
int list_length(scheme_t *sc, cell_ptr_t a) {
	int i=0;
	cell_ptr_t slow, fast;

	slow = fast = a;
	while (1) {
		if( is_nil(fast) )
			return i;
		if( !is_pair(sc, fast) )
			return -2 - i;
		fast = cdr(sc, fast);
		++i;
		if( is_nil(fast) )
			return i;
		if( !is_pair(sc, fast) )
			return -2 - i;
		++i;
		fast = cdr(sc, fast);

		/* Safe because we would have already returned if `fast'
		encountered a non-pair. */
		slow = cdr(sc, slow);
		if (fast.index == slow.index ) {
			/* the fast cell_ptr_t has looped back around and caught up
			with the slow cell_ptr_t, hence the structure is circular,
			not of finite length, and therefore not a list */
			return -1;
		}
	}
}




typedef cell_ptr_t (*dispatch_func)(scheme_t*, enum scheme_opcodes);

typedef bool (*test_predicate)(scheme_t*, cell_ptr_t);
static bool is_any(scheme_t* sc, cell_ptr_t p) {
	return true;
}

static bool
is_nonneg(scheme_t* sc,
	  cell_ptr_t p)
{
	return ivalue(sc, p) >= 0 && is_integer(sc, p);
}

/* Correspond carefully with following defines! */
static struct {
	test_predicate fct;
	const char *kind;
} tests[]= {
	{0,0}, /* unused */
	{is_any, 0},
	{is_string, "string"},
	{is_symbol, "symbol"},
	{is_environment, "environment"},
	{is_pair, "pair"},
	{0, "pair or '()"},
	{is_character, "character"},
	{is_vector, "vector"},
	{is_number, "number"},
	{is_integer, "integer"},
	{is_nonneg, "non-negative integer"}
};

#define TST_NONE 0
#define TST_ANY "\001"
#define TST_STRING "\002"
#define TST_SYMBOL "\003"
#define TST_ENVIRONMENT "\004"
#define TST_PAIR "\005"
#define TST_LIST "\006"
#define TST_CHAR "\007"
#define TST_VECTOR "\010"
#define TST_NUMBER "\011"
#define TST_INTEGER "\012"
#define TST_NATURAL "\013"

typedef struct {
	dispatch_func func;
	char *name;
	int min_arity;
	int max_arity;
	char *arg_tests_encoding;
} op_code_info;

#define INF_ARG 0xffff

static op_code_info dispatch_table[]= {
#define _OP_DEF(A,B,C,D,E,OP) {A,B,C,D,E},
#include "opdefines.h"
	{ 0, 0, 0, 0, 0 }
};

const char*
procname(scheme_t* sc,
	 cell_ptr_t x)
{
	int n	= procnum(sc, x);
	const char *name=dispatch_table[n].name;
	if(name==0) {
		name="ILLEGAL!";
	}
	return name;
}

/* kernel of this interpreter */
static void
eval_cycle(scheme_t *sc,
	   enum scheme_opcodes op)
{
	sc->op = op;
	for( ; ; ) {
		op_code_info*	pcd	= dispatch_table + sc->op;
		if( pcd->name != 0 ) { /* if built-in function, check arguments */
			char	msg[STRBUFFSIZE];
			int	ok	= 1;
			int	n	= list_length(sc, sc->args);

			/* Check number of arguments */
			if( n < pcd->min_arity ) {
				ok	= 0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
				         pcd->name,
					 pcd->min_arity == pcd->max_arity ? "" : " at least",
				         pcd->min_arity);
			}

			if( ok && n>pcd->max_arity ) {
				ok=0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
				         pcd->name,
					 pcd->min_arity == pcd->max_arity ? "" : " at most",
				         pcd->max_arity);
			}

			if( ok ) {
				if( pcd->arg_tests_encoding != 0 ) {
					int		i = 0;
					int		j;
					const char*	t = pcd->arg_tests_encoding;
					cell_ptr_t	arglist	= sc->args;
					do {
						cell_ptr_t	arg = car(sc, arglist);
						j	= (int)t[0];
						if( j == TST_LIST[0] ) {
							if( !is_nil(arg) && !is_pair(sc, arg) ) break;
						} else {
							if( !tests[j].fct(sc, arg) ) break;
						}

						if( t[1] != 0 ) { /* last test is replicated as necessary */
							t++;
						}
						arglist	= cdr(sc, arglist);
						i++;
					} while( i < n );
					if( i < n ) {
						ok	= 0;
						snprintf(msg, STRBUFFSIZE, "%s: argument %d must be: %s",
						         pcd->name,
							 i + 1,
						         tests[j].kind);
					}
				}
			}

			if( !ok ) {
				if( is_nil(_error_1(sc, msg, cell_ptr(0))) ) {
					return;
				}
				pcd	= dispatch_table + sc->op;
			}
		}

		ok_to_freely_gc(sc);

//		fprintf(stderr, "op: %s\n", pcd->name);
		if( is_nil(pcd->func(sc, (enum scheme_opcodes)sc->op)) ) {
			return;
		}

		if( sc->no_memory ) {
			fprintf(stderr,"No memory!\n");
			return;
		}
	}
}

/* ========== Initialization of internal keywords ========== */

static void assign_syntax(scheme_t *sc, char *name) {
	cell_ptr_t x;

	x = oblist_add_by_name(sc, name);
	ptr_typeflag(sc, x) |= T_SYNTAX;
}

static void assign_proc(scheme_t *sc, enum scheme_opcodes op, char *name) {
	cell_ptr_t x, y;

	x = mk_symbol(sc, name);
	y = mk_proc(sc,op);
	new_slot_in_env(sc, x, y);
}

static cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t y;

	y = get_cell(sc, cell_ptr(SPCELL_NIL), cell_ptr(SPCELL_NIL));
	ptr_typeflag(sc, y) = (T_PROC | T_ATOM);
	ivalue_unchecked(sc, y) = (long) op;
	set_num_integer(sc, y);
	return y;
}



scheme_t *scheme_init_new() {
	scheme_t *sc=(scheme_t*)malloc(sizeof(scheme_t));
	if(!scheme_init(sc)) {
		free(sc);
		return 0;
	} else {
		return sc;
	}
}

scheme_t *scheme_init_new_custom_alloc(func_alloc malloc, func_dealloc free) {
	scheme_t *sc=(scheme_t*)malloc(sizeof(scheme_t));
	if(!scheme_init_custom_alloc(sc,malloc,free)) {
		free(sc);
		return 0;
	} else {
		return sc;
	}
}


int scheme_init(scheme_t *sc) {
	return scheme_init_custom_alloc(sc,malloc,free);
}

int scheme_init_custom_alloc(scheme_t *sc, func_alloc malloc, func_dealloc free) {
	int i, n=sizeof(dispatch_table)/sizeof(dispatch_table[0]);
	cell_ptr_t x;
	memset(sc, 0, sizeof(*sc));

	sc->num_zero.is_integer		= 1;
	sc->num_zero.value.ivalue	= 0;
	sc->num_one.is_integer		= 1;
	sc->num_one.value.ivalue	= 1;

	sc->gensym_cnt	= 0;
	sc->malloc	= malloc;
	sc->free	= free;
	sc->sink	= cell_ptr(SPCELL_SINK);
	sc->fcells	= 0;
	sc->no_memory	= false;

	if( !alloc_cellseg(sc) ) {
		sc->no_memory=1;
		return 0;
	}

	sc->gc_verbose	= false;
	dump_stack_initialize(sc);
	sc->code	= cell_ptr(SPCELL_NIL);
	sc->tracing	= false;

	/* init NIL */
	for( sint32 i = SPCELL_NIL; i <= SPCELL_EOF_OBJ; ++i ) {
		ptr_typeflag(sc, cell_ptr(i)) = (T_ATOM | MARK);
		car(sc, cell_ptr(i)) = cdr(sc, cell_ptr(i)) = cell_ptr(SPCELL_NIL);
	}

	/* init sink */
	ptr_typeflag(sc, sc->sink) = (T_PAIR | MARK);
	car(sc, sc->sink)	= cell_ptr(SPCELL_NIL);

	/* init c_nest */
	sc->c_nest		= cell_ptr(SPCELL_NIL);

	sc->oblist		= oblist_initial_value(sc);

	/* init global_env */
	new_frame_in_env(sc, cell_ptr(SPCELL_NIL));
	sc->global_env		= sc->envir;

	/* init else */
	x = mk_symbol(sc,"else");
	new_slot_in_env(sc, x, cell_ptr(SPCELL_TRUE));

	assign_syntax(sc, "lambda");
	assign_syntax(sc, "quote");
	assign_syntax(sc, "define");
	assign_syntax(sc, "if");
	assign_syntax(sc, "begin");
	assign_syntax(sc, "set!");
	assign_syntax(sc, "let");
	assign_syntax(sc, "let*");
	assign_syntax(sc, "letrec");
	assign_syntax(sc, "cond");
	assign_syntax(sc, "delay");
	assign_syntax(sc, "and");
	assign_syntax(sc, "or");
	assign_syntax(sc, "cons-stream");
	assign_syntax(sc, "macro");
	assign_syntax(sc, "case");

	for(i=0; i<n; i++) {
		if(dispatch_table[i].name!=0) {
			assign_proc(sc, (enum scheme_opcodes)i, dispatch_table[i].name);
		}
	}

	/* initialization of global pointers to special symbols */
//	sc->LAMBDA	= mk_symbol(sc, "lambda");
//	sc->QUOTE	= mk_symbol(sc, "quote");
//	sc->QQUOTE	= mk_symbol(sc, "quasiquote");
//	sc->UNQUOTE	= mk_symbol(sc, "unquote");
//	sc->UNQUOTESP	= mk_symbol(sc, "unquote-splicing");
//	sc->FEED_TO	= mk_symbol(sc, "=>");
//	sc->COLON_HOOK	= mk_symbol(sc, "*colon-hook*");
//	sc->ERROR_HOOK	= mk_symbol(sc, "*error-hook*");
//	sc->SHARP_HOOK	= mk_symbol(sc, "*sharp-hook*");
//	sc->COMPILE_HOOK = mk_symbol(sc, "*compile-hook*");

	return !sc->no_memory;
}

void scheme_set_external_data(scheme_t *sc, void *p) {
	sc->ext_data=p;
}

void scheme_deinit(scheme_t *sc) {

	sc->oblist	= cell_ptr(SPCELL_NIL);
	sc->global_env	= cell_ptr(SPCELL_NIL);
	dump_stack_free(sc);
	sc->envir	= cell_ptr(SPCELL_NIL);
	sc->code	= cell_ptr(SPCELL_NIL);
	sc->args	= cell_ptr(SPCELL_NIL);
	sc->value	= cell_ptr(SPCELL_NIL);

	sc->gc_verbose	= 0;
	gc(sc, cell_ptr(SPCELL_NIL), cell_ptr(SPCELL_NIL));
}


void scheme_load_string(scheme_t *sc, const char *cmd) {
	dump_stack_reset(sc);
	sc->envir		= sc->global_env;
	sc->loaded_file		= cmd;
	sc->file_position	= cmd;
	Eval_Cycle(sc, OP_T0LVL);
}

void scheme_define(scheme_t *sc, cell_ptr_t envir, cell_ptr_t symbol, cell_ptr_t value) {
	cell_ptr_t x;

	x	= find_slot_in_env(sc, envir, symbol, 0);
	if( !is_nil(x) ) {
		set_slot_in_env(sc, x, value);
	} else {
		new_slot_spec_in_env(sc, envir, symbol, value);
	}
}

#if !STANDALONE
void scheme_register_foreign_func(scheme_t * sc, scheme_registerable * sr) {
	scheme_define(sc,
	              sc->global_env,
	              mk_symbol(sc,sr->name),
	              mk_foreign_func(sc, sr->f));
}

void scheme_register_foreign_func_list(scheme_t * sc,
                                       scheme_registerable * list,
                                       int count) {
	int i;
	for(i = 0; i < count; i++) {
		scheme_register_foreign_func(sc, list + i);
	}
}

cell_ptr_t scheme_apply0(scheme_t *sc, const char *procname) {
	return scheme_eval(sc, cons(sc,mk_symbol(sc,procname),sc->NIL));
}

void save_from_C_call(scheme_t *sc) {
	cell_ptr_t saved_data =
	    cons(sc,
	         car(sc->sink),
	         cons(sc,
	              sc->envir,
	              sc->dump));
	/* Push */
	sc->c_nest = cons(sc, saved_data, sc->c_nest);
	/* Truncate the dump stack so TS will return here when done, not
	 directly resume pre-C-call operations. */
	dump_stack_reset(sc);
}
void restore_from_C_call(scheme_t *sc) {
	car(sc->sink) = caar(sc->c_nest);
	sc->envir = cadar(sc->c_nest);
	sc->dump = cdr(cdar(sc->c_nest));
	/* Pop */
	sc->c_nest = cdr(sc->c_nest);
}

/* "func" and "args" are assumed to be already eval'ed. */
cell_ptr_t scheme_call(scheme_t *sc, cell_ptr_t func, cell_ptr_t args) {
	int old_repl = sc->interactive_repl;
	sc->interactive_repl = 0;
	save_from_C_call(sc);
	sc->envir = sc->global_env;
	sc->args = args;
	sc->code = func;
	sc->retcode = 0;
	Eval_Cycle(sc, OP_APPLY);
	sc->interactive_repl = old_repl;
	restore_from_C_call(sc);
	return sc->value;
}

cell_ptr_t scheme_eval(scheme_t *sc, cell_ptr_t obj) {
	int old_repl = sc->interactive_repl;
	sc->interactive_repl = 0;
	save_from_C_call(sc);
	sc->args = sc->NIL;
	sc->code = obj;
	sc->retcode = 0;
	Eval_Cycle(sc, OP_EVAL);
	sc->interactive_repl = old_repl;
	restore_from_C_call(sc);
	return sc->value;
}


#endif

/* ========== Main ========== */
const char* types[16] = {
	"NONE",
	"T_STRING",
	"T_NUMBER",
	"T_SYMBOL",
	"T_PROC",
	"T_PAIR",
	"T_CLOSURE",
	"T_CONTINUATION",
	"T_FOREIGN",
	"T_CHARACTER",
	"T_PORT",
	"T_VECTOR",
	"T_MACRO",
	"T_PROMISE",
	"T_ENVIRONMENT",
	"T_LAST_SYSTEM_TYPE"
};


cell_ptr_t
print_args(scheme_t* sc, cell_ptr_t args) {
	cell_ptr_t	retval	= cell_ptr(SPCELL_TRUE);;
	cell_ptr_t	c	= args;
	int		i	= 0;
	fprintf(stderr, "arg count: %d", list_length(sc, args));
	for( ; c.index != SPCELL_NIL; c = cdr(sc, c) ) {
		fprintf(stderr, "arg %d: %s\n", i, types[ptr_type(sc, car(sc, c))]);
		++i;
	}
	return retval;
}

#if STANDALONE

#if defined(__APPLE__) && !defined (OSX)
int main() {
	extern MacTS_main(int argc, char **argv);
	char**    argv;
	int argc = ccommand(&argv);
	MacTS_main(argc,argv);
	return 0;
}
int MacTS_main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
	scheme_t* sc	= (scheme_t*)malloc(sizeof(scheme_t));

	FILE *fin = NULL;
	char *file_name=InitFile;
	int retcode;
	int isfile=1;

	if(argc==1) {
		printf(banner);
	}

	if(argc==2 && strcmp(argv[1],"-?")==0) {
		printf("Usage: tinyscheme -?\n");
		printf("or:    tinyscheme [<file1> <file2> ...]\n");
		printf("followed by\n");
		printf("          -1 <file> [<arg1> <arg2> ...]\n");
		printf("          -c <Scheme commands> [<arg1> <arg2> ...]\n");
		printf("assuming that the executable is named tinyscheme.\n");
		printf("Use - as filename for stdin.\n");
		return 1;
	}

	if(!scheme_init(sc)) {
		fprintf(stderr,"Could not initialize!\n");
		return 2;
	}

	fprintf(stderr, "cell size: %lu\n", sizeof(cell_t));

	scheme_define(sc, sc->global_env, mk_symbol(sc, "print-args"), mk_foreign_func(sc, print_args));

#if USE_DL
	scheme_define(sc, sc->global_env, mk_symbol(sc, "load-extension"), mk_foreign_func(sc, scm_load_ext));
#endif
	argv++;
	if(access(file_name,0)!=0) {
		char *p=getenv("TINYSCHEMEINIT");
		if(p!=0) {
			file_name=p;
		}
	}

	do {
		if(strcmp(file_name,"-")==0) {
			fin=stdin;
		} else if( strcmp(file_name, "-1")==0 || strcmp(file_name, "-c") == 0 ) {
			cell_ptr_t args	= cell_ptr(SPCELL_NIL);
			isfile		= file_name[1] == '1';
			file_name	= *argv++;
			if( strcmp(file_name, "-") == 0 ) {
				fin=stdin;
			} else if( isfile ) {
				fin=fopen(file_name, "r");
			}
			for( ; *argv; argv++ ) {
				cell_ptr_t value	= mk_string(sc, *argv);
				args	= cons(sc, value, args);
			}
			args	= reverse_in_place(sc, cell_ptr(SPCELL_NIL), args);
			scheme_define(sc, sc->global_env, mk_symbol(sc, "*args*"), args);

		} else {
			fin	= fopen(file_name, "r");
		}
		if( isfile && fin == 0 ) {
			fprintf(stderr, "Could not open file %s\n", file_name);
		} else {
			FILE *f = fopen(file_name, "rb");
			fseek(f, 0, SEEK_END);
			long fsize = ftell(f);
			fseek(f, 0, SEEK_SET);

			char *string = malloc(fsize + 1);
			fread(string, fsize, 1, f);
			fclose(f);

			string[fsize] = 0;
			scheme_load_string(sc, string);

			free(string);
			if( !isfile || fin!=stdin ) {
				if(sc->retcode != 0) {
					fprintf(stderr, "Errors encountered reading %s\n", file_name);
				}
				if( isfile ) {
					fclose(fin);
				}
			}
		}
		file_name	= *argv++;
	} while( file_name != 0 );

	retcode	= sc->retcode;
	scheme_deinit(sc);
	free(sc);

	return retcode;
}

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
