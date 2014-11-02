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


INTERFACE INLINE int is_string(cell_ptr_t p) {
	return (type(p)==T_STRING);
}

INTERFACE INLINE int is_vector(cell_ptr_t p) {
	return (type(p)==T_VECTOR);
}
INTERFACE INLINE int is_number(cell_ptr_t p) {
	return (type(p)==T_NUMBER);
}
INTERFACE INLINE int is_integer(cell_ptr_t p) {
	if (!is_number(p))
		return 0;
	if (num_is_integer(p) || (double)ivalue(p) == rvalue(p))
		return 1;
	return 0;
}

INTERFACE INLINE int is_real(cell_ptr_t p) {
	return is_number(p) && (!(p)->_object._number.is_integer);
}

INTERFACE INLINE int is_character(cell_ptr_t p) {
	return (type(p)==T_CHARACTER);
}
INTERFACE INLINE char *string_value(cell_ptr_t p) {
	return strvalue(p);
}
INLINE number_t nvalue(cell_ptr_t p) {
	return ((p)->_object._number);
}
INTERFACE long ivalue(cell_ptr_t p) {
	return (num_is_integer(p)?(p)->_object._number.value.ivalue:(long)(p)->_object._number.value.rvalue);
}
INTERFACE double rvalue(cell_ptr_t p) {
	return (!num_is_integer(p)?(p)->_object._number.value.rvalue:(double)(p)->_object._number.value.ivalue);
}
INTERFACE  long charvalue(cell_ptr_t p) {
	return ivalue_unchecked(p);
}

INTERFACE INLINE int is_pair(cell_ptr_t p) {
	return (type(p)==T_PAIR);
}
INTERFACE cell_ptr_t pair_car(cell_ptr_t p) {
	return car(p);
}
INTERFACE cell_ptr_t pair_cdr(cell_ptr_t p) {
	return cdr(p);
}
INTERFACE cell_ptr_t set_car(cell_ptr_t p, cell_ptr_t q) {
	return car(p)=q;
}
INTERFACE cell_ptr_t set_cdr(cell_ptr_t p, cell_ptr_t q) {
	return cdr(p)=q;
}

INTERFACE INLINE int is_symbol(cell_ptr_t p) {
	return (type(p)==T_SYMBOL);
}
INTERFACE INLINE char *symname(cell_ptr_t p) {
	return strvalue(car(p));
}
#if USE_PLIST
SCHEME_EXPORT INLINE int hasprop(cell_ptr_t p) {
	return (typeflag(p)&T_SYMBOL);
}
#define symprop(p)       cdr(p)
#endif

INTERFACE INLINE int is_syntax(cell_ptr_t p) {
	return (typeflag(p)&T_SYNTAX);
}
INTERFACE INLINE int is_proc(cell_ptr_t p) {
	return (type(p)==T_PROC);
}
INTERFACE INLINE int is_foreign(cell_ptr_t p) {
	return (type(p)==T_FOREIGN);
}
INTERFACE INLINE char *syntaxname(cell_ptr_t p) {
	return strvalue(car(p));
}

INTERFACE INLINE int is_closure(cell_ptr_t p) {
	return (type(p)==T_CLOSURE);
}
INTERFACE INLINE int is_macro(cell_ptr_t p) {
	return (type(p)==T_MACRO);
}
INTERFACE INLINE cell_ptr_t closure_code(cell_ptr_t p) {
	return car(p);
}
INTERFACE INLINE cell_ptr_t closure_env(cell_ptr_t p) {
	return cdr(p);
}

INTERFACE INLINE int is_continuation(cell_ptr_t p) {
	return (type(p)==T_CONTINUATION);
}

/* To do: promise should be forced ONCE only */
INTERFACE INLINE int is_promise(cell_ptr_t p) {
	return (type(p)==T_PROMISE);
}

INTERFACE INLINE int is_environment(cell_ptr_t p) {
	return (type(p)==T_ENVIRONMENT);
}

INTERFACE INLINE int is_immutable(cell_ptr_t p) {
	return (typeflag(p)&T_IMMUTABLE);
}
/*#define setimmutable(p)  typeflag(p) |= T_IMMUTABLE*/
INTERFACE INLINE void setimmutable(cell_ptr_t p) {
	typeflag(p) |= T_IMMUTABLE;
}

static cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op);
static void Eval_Cycle(scheme_t *sc, enum scheme_opcodes op);
static void assign_syntax(scheme_t *sc, char *name);
static void assign_proc(scheme_t *sc, enum scheme_opcodes, char *name);

static INLINE void ok_to_freely_gc(scheme_t *sc) {
	car(sc->sink) = sc->NIL;
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
		if (fast == sc->NIL)
			return i;
		if (!is_pair(fast))
			return -2 - i;
		fast = cdr(fast);
		++i;
		if (fast == sc->NIL)
			return i;
		if (!is_pair(fast))
			return -2 - i;
		++i;
		fast = cdr(fast);

		/* Safe because we would have already returned if `fast'
		encountered a non-pair. */
		slow = cdr(slow);
		if (fast == slow) {
			/* the fast cell_ptr_t has looped back around and caught up
			with the slow cell_ptr_t, hence the structure is circular,
			not of finite length, and therefore not a list */
			return -1;
		}
	}
}




typedef cell_ptr_t (*dispatch_func)(scheme_t *, enum scheme_opcodes);

typedef int (*test_predicate)(cell_ptr_t);
static int is_any(cell_ptr_t p) {
	return 1;
}

static int is_nonneg(cell_ptr_t p) {
	return ivalue(p)>=0 && is_integer(p);
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
	{is_port, "port"},
	{is_inport,"input port"},
	{is_outport,"output port"},
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
#define TST_PORT "\004"
#define TST_INPORT "\005"
#define TST_OUTPORT "\006"
#define TST_ENVIRONMENT "\007"
#define TST_PAIR "\010"
#define TST_LIST "\011"
#define TST_CHAR "\012"
#define TST_VECTOR "\013"
#define TST_NUMBER "\014"
#define TST_INTEGER "\015"
#define TST_NATURAL "\016"

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

const char *procname(cell_ptr_t x) {
	int n=procnum(x);
	const char *name=dispatch_table[n].name;
	if(name==0) {
		name="ILLEGAL!";
	}
	return name;
}

/* kernel of this interpreter */
static void Eval_Cycle(scheme_t *sc, enum scheme_opcodes op) {
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
				         pcd->min_arity==pcd->max_arity?"":" at least",
				         pcd->min_arity);
			}

			if( ok && n>pcd->max_arity ) {
				ok=0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
				         pcd->name,
				         pcd->min_arity==pcd->max_arity?"":" at most",
				         pcd->max_arity);
			}

			if( ok ) {
				if( pcd->arg_tests_encoding != 0 ) {
					int		i = 0;
					int		j;
					const char*	t = pcd->arg_tests_encoding;
					cell_ptr_t	arglist	= sc->args;
					do {
						cell_ptr_t	arg = car(arglist);
						j	= (int)t[0];
						if( j == TST_LIST[0] ) {
							if( arg != sc->NIL && !is_pair(arg) ) break;
						} else {
							if( !tests[j].fct(arg) ) break;
						}

						if( t[1] != 0 ) { /* last test is replicated as necessary */
							t++;
						}
						arglist	= cdr(arglist);
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
				if( _error_1(sc,msg,0) == sc->NIL ) {
					return;
				}
				pcd	= dispatch_table + sc->op;
			}
		}

		ok_to_freely_gc(sc);

		if( pcd->func(sc, (enum scheme_opcodes)sc->op) == sc->NIL ) {
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
	typeflag(x) |= T_SYNTAX;
}

static void assign_proc(scheme_t *sc, enum scheme_opcodes op, char *name) {
	cell_ptr_t x, y;

	x = mk_symbol(sc, name);
	y = mk_proc(sc,op);
	new_slot_in_env(sc, x, y);
}

static cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t y;

	y = get_cell(sc, sc->NIL, sc->NIL);
	typeflag(y) = (T_PROC | T_ATOM);
	ivalue_unchecked(y) = (long) op;
	set_num_integer(y);
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

	sc->num_zero.is_integer		= 1;
	sc->num_zero.value.ivalue	= 0;
	sc->num_one.is_integer		= 1;
	sc->num_one.value.ivalue	= 1;

	sc->gensym_cnt=0;
	sc->malloc=malloc;
	sc->free=free;
	sc->sink = &sc->_sink;
	sc->NIL = &sc->_NIL;
	sc->T = &sc->_HASHT;
	sc->F = &sc->_HASHF;
	sc->EOF_OBJ=&sc->_EOF_OBJ;
	sc->free_cell = &sc->_NIL;
	sc->fcells = 0;
	sc->no_memory=0;
	sc->inport=sc->NIL;
	sc->outport=sc->NIL;
	sc->save_inport=sc->NIL;
	sc->loadport=sc->NIL;
	sc->nesting=0;
	sc->interactive_repl=0;

	if( !alloc_cellseg(sc) ) {
		sc->no_memory=1;
		return 0;
	}
	sc->gc_verbose = 0;
	dump_stack_initialize(sc);
	sc->code = sc->NIL;
	sc->tracing=0;

	/* init sc->NIL */
	typeflag(sc->NIL) = (T_ATOM | MARK);
	car(sc->NIL) = cdr(sc->NIL) = sc->NIL;
	/* init T */
	typeflag(sc->T) = (T_ATOM | MARK);
	car(sc->T) = cdr(sc->T) = sc->T;
	/* init F */
	typeflag(sc->F) = (T_ATOM | MARK);
	car(sc->F) = cdr(sc->F) = sc->F;
	/* init sink */
	typeflag(sc->sink) = (T_PAIR | MARK);
	car(sc->sink) = sc->NIL;
	/* init c_nest */
	sc->c_nest = sc->NIL;

	sc->oblist = oblist_initial_value(sc);
	/* init global_env */
	new_frame_in_env(sc, sc->NIL);
	sc->global_env = sc->envir;
	/* init else */
	x = mk_symbol(sc,"else");
	new_slot_in_env(sc, x, sc->T);

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
	sc->LAMBDA	= mk_symbol(sc, "lambda");
	sc->QUOTE	= mk_symbol(sc, "quote");
	sc->QQUOTE	= mk_symbol(sc, "quasiquote");
	sc->UNQUOTE	= mk_symbol(sc, "unquote");
	sc->UNQUOTESP	= mk_symbol(sc, "unquote-splicing");
	sc->FEED_TO	= mk_symbol(sc, "=>");
	sc->COLON_HOOK	= mk_symbol(sc, "*colon-hook*");
	sc->ERROR_HOOK	= mk_symbol(sc, "*error-hook*");
	sc->SHARP_HOOK	= mk_symbol(sc, "*sharp-hook*");
	sc->COMPILE_HOOK = mk_symbol(sc, "*compile-hook*");

	return !sc->no_memory;
}

void scheme_set_input_port_file(scheme_t *sc, FILE *fin) {
	sc->inport=port_from_file(sc,fin,port_input);
}

void scheme_set_input_port_string(scheme_t *sc, char *start, char *past_the_end) {
	sc->inport=port_from_string(sc,start,past_the_end,port_input);
}

void scheme_set_output_port_file(scheme_t *sc, FILE *fout) {
	sc->outport=port_from_file(sc,fout,port_output);
}

void scheme_set_output_port_string(scheme_t *sc, char *start, char *past_the_end) {
	sc->outport=port_from_string(sc,start,past_the_end,port_output);
}

void scheme_set_external_data(scheme_t *sc, void *p) {
	sc->ext_data=p;
}

void scheme_deinit(scheme_t *sc) {
	int i;

#if SHOW_ERROR_LINE
	char *fname;
#endif

	sc->oblist=sc->NIL;
	sc->global_env=sc->NIL;
	dump_stack_free(sc);
	sc->envir=sc->NIL;
	sc->code=sc->NIL;
	sc->args=sc->NIL;
	sc->value=sc->NIL;
	if(is_port(sc->inport)) {
		typeflag(sc->inport) = T_ATOM;
	}
	sc->inport=sc->NIL;
	sc->outport=sc->NIL;
	if(is_port(sc->save_inport)) {
		typeflag(sc->save_inport) = T_ATOM;
	}
	sc->save_inport=sc->NIL;
	if(is_port(sc->loadport)) {
		typeflag(sc->loadport) = T_ATOM;
	}
	sc->loadport=sc->NIL;
	sc->gc_verbose=0;
	gc(sc,sc->NIL,sc->NIL);

#if SHOW_ERROR_LINE
	for(i=0; i<=sc->file_i; i++) {
		if (sc->load_stack[i].kind & port_file) {
			fname = sc->load_stack[i].rep.stdio.filename;
			if(fname)
				sc->free(fname);
		}
	}
#endif
}

void scheme_load_file(scheme_t *sc, FILE *fin) {
	scheme_load_named_file(sc,fin,0);
}

void scheme_load_named_file(scheme_t *sc, FILE *fin, const char *filename) {
	dump_stack_reset(sc);
	sc->envir = sc->global_env;
	sc->file_i=0;
	sc->load_stack[0].kind=port_input|port_file;
	sc->load_stack[0].rep.stdio.file=fin;
	sc->loadport=mk_port(sc,sc->load_stack);
	sc->retcode=0;
	if(fin==stdin) {
		sc->interactive_repl=1;
	}

#if SHOW_ERROR_LINE
	sc->load_stack[0].rep.stdio.curr_line = 0;
	if(fin!=stdin && filename)
		sc->load_stack[0].rep.stdio.filename = store_string(sc, strlen(filename), filename, 0);
#endif

	sc->inport=sc->loadport;
	sc->args = mk_integer(sc,sc->file_i);
	Eval_Cycle(sc, OP_T0LVL);
	typeflag(sc->loadport)=T_ATOM;
	if(sc->retcode==0) {
		sc->retcode=sc->nesting!=0;
	}
}

void scheme_load_string(scheme_t *sc, const char *cmd) {
	dump_stack_reset(sc);
	sc->envir	= sc->global_env;
	sc->file_i	= 0;
	sc->load_stack[0].kind	= port_input | port_string;
	sc->load_stack[0].rep.string.start		= (char*)cmd; /* This func respects const */
	sc->load_stack[0].rep.string.past_the_end	= (char*)cmd + strlen(cmd);
	sc->load_stack[0].rep.string.curr		= (char*)cmd;
	sc->loadport	= mk_port(sc, sc->load_stack);
	sc->retcode	= 0;
	sc->interactive_repl	= 0;
	sc->inport	= sc->loadport;
	sc->args	= mk_integer(sc, sc->file_i);
	Eval_Cycle(sc, OP_T0LVL);
	typeflag(sc->loadport)	= T_ATOM;
	if( sc->retcode == 0 ) {
		sc->retcode=sc->nesting!=0;
	}
}

void scheme_define(scheme_t *sc, cell_ptr_t envir, cell_ptr_t symbol, cell_ptr_t value) {
	cell_ptr_t x;

	x=find_slot_in_env(sc,envir,symbol,0);
	if (x != sc->NIL) {
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
	scheme_set_input_port_file(sc, stdin);
	scheme_set_output_port_file(sc, stdout);

	fprintf(stderr, "cell size: %lu\n", sizeof(cell_t));

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
			cell_ptr_t args	= sc->NIL;
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
			args	= reverse_in_place(sc, sc->NIL, args);
			scheme_define(sc, sc->global_env, mk_symbol(sc, "*args*"), args);

		} else {
			fin	= fopen(file_name, "r");
		}
		if( isfile && fin == 0 ) {
			fprintf(stderr, "Could not open file %s\n", file_name);
		} else {
			if( isfile ) {
				scheme_load_named_file(sc, fin, file_name);
			} else {
				scheme_load_string(sc, file_name);
			}
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
	if( argc == 1 ) {
		scheme_load_named_file(sc, stdin,0);
	}
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
