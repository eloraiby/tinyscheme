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

#include "parser.h"

#define BACKQUOTE '`'
#define DELIMITERS  "()\";\f\t\v\n\r "

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

#ifndef prompt
# define prompt "ts> "
#endif

#ifndef InitFile
# define InitFile "init.scm"
#endif

#ifndef FIRST_CELLSEGS
# define FIRST_CELLSEGS 3
#endif

number_t	__s_num_zero;
number_t	__s_num_one;


INTERFACE INLINE int is_string(cell_ptr_t p) {
	return (type(p)==T_STRING);
}

INTERFACE static int is_list(scheme_t *sc, cell_ptr_t p);
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
#define cont_dump(p)     cdr(p)

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

static INLINE int is_one_of(char *s, int c);
static char   *readstr_upto(scheme_t *sc, char *delim);
static cell_ptr_t readstrexp(scheme_t *sc);
static INLINE int skipspace(scheme_t *sc);
static int token(scheme_t *sc);
static cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op);
static cell_ptr_t mk_closure(scheme_t *sc, cell_ptr_t c, cell_ptr_t e);
static cell_ptr_t mk_continuation(scheme_t *sc, cell_ptr_t d);
static cell_ptr_t reverse(scheme_t *sc, cell_ptr_t a);
static cell_ptr_t reverse_in_place(scheme_t *sc, cell_ptr_t term, cell_ptr_t list);
static cell_ptr_t revappend(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
static cell_ptr_t opexe_0(scheme_t *sc, enum scheme_opcodes op);
static cell_ptr_t opexe_1(scheme_t *sc, enum scheme_opcodes op);
static cell_ptr_t opexe_2(scheme_t *sc, enum scheme_opcodes op);
static cell_ptr_t opexe_4(scheme_t *sc, enum scheme_opcodes op);
static cell_ptr_t opexe_5(scheme_t *sc, enum scheme_opcodes op);
static cell_ptr_t opexe_6(scheme_t *sc, enum scheme_opcodes op);
static void Eval_Cycle(scheme_t *sc, enum scheme_opcodes op);
static void assign_syntax(scheme_t *sc, char *name);
static int syntaxnum(cell_ptr_t p);
static void assign_proc(scheme_t *sc, enum scheme_opcodes, char *name);

static INLINE void ok_to_freely_gc(scheme_t *sc) {
	car(sc->sink) = sc->NIL;
}


/* ========== Routines for Reading ========== */

/* read characters up to delimiter, but cater to character constants */
static char *readstr_upto(scheme_t *sc, char *delim) {
	char *p = sc->strbuff;

	while ((p - sc->strbuff < sizeof(sc->strbuff)) &&
	        !is_one_of(delim, (*p++ = inchar(sc))));

	if(p == sc->strbuff+2 && p[-2] == '\\') {
		*p=0;
	} else {
		backchar(sc,p[-1]);
		*--p = '\0';
	}
	return sc->strbuff;
}

/* read string expression "xxx...xxx" */
static cell_ptr_t readstrexp(scheme_t *sc) {
	char *p = sc->strbuff;
	int c;
	int c1=0;
	enum { st_ok, st_bsl, st_x1, st_x2, st_oct1, st_oct2 } state=st_ok;

	for (;;) {
		c=inchar(sc);
		if(c == EOF || p-sc->strbuff > sizeof(sc->strbuff)-1) {
			return sc->F;
		}
		switch(state) {
		case st_ok:
			switch(c) {
			case '\\':
				state=st_bsl;
				break;
			case '"':
				*p=0;
				return mk_counted_string(sc,sc->strbuff,p-sc->strbuff);
			default:
				*p++=c;
				break;
			}
			break;
		case st_bsl:
			switch(c) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				state=st_oct1;
				c1=c-'0';
				break;
			case 'x':
			case 'X':
				state=st_x1;
				c1=0;
				break;
			case 'n':
				*p++='\n';
				state=st_ok;
				break;
			case 't':
				*p++='\t';
				state=st_ok;
				break;
			case 'r':
				*p++='\r';
				state=st_ok;
				break;
			case '"':
				*p++='"';
				state=st_ok;
				break;
			default:
				*p++=c;
				state=st_ok;
				break;
			}
			break;
		case st_x1:
		case st_x2:
			c=toupper(c);
			if(c>='0' && c<='F') {
				if(c<='9') {
					c1=(c1<<4)+c-'0';
				} else {
					c1=(c1<<4)+c-'A'+10;
				}
				if(state==st_x1) {
					state=st_x2;
				} else {
					*p++=c1;
					state=st_ok;
				}
			} else {
				return sc->F;
			}
			break;
		case st_oct1:
		case st_oct2:
			if (c < '0' || c > '7') {
				*p++=c1;
				backchar(sc, c);
				state=st_ok;
			} else {
				if (state==st_oct2 && c1 >= 32)
					return sc->F;

				c1=(c1<<3)+(c-'0');

				if (state == st_oct1)
					state=st_oct2;
				else {
					*p++=c1;
					state=st_ok;
				}
			}
			break;

		}
	}
}

/* check c is in chars */
static INLINE int is_one_of(char *s, int c) {
	if(c==EOF) return 1;
	while (*s)
		if (*s++ == c)
			return (1);
	return (0);
}

/* skip white characters */
static INLINE int skipspace(scheme_t *sc) {
	int c = 0, curr_line = 0;

	do {
		c=inchar(sc);
#if SHOW_ERROR_LINE
		if(c=='\n')
			curr_line++;
#endif
	} while (isspace(c));

	/* record it */
#if SHOW_ERROR_LINE
	if (sc->load_stack[sc->file_i].kind & port_file)
		sc->load_stack[sc->file_i].rep.stdio.curr_line += curr_line;
#endif

	if(c!=EOF) {
		backchar(sc,c);
		return 1;
	} else {
		return EOF;
	}
}

/* get token */
static int token(scheme_t *sc) {
	int c;
	c = skipspace(sc);
	if(c == EOF) {
		return (TOK_EOF);
	}
	switch (c=inchar(sc)) {
	case EOF:
		return (TOK_EOF);
	case '(':
		return (TOK_LPAREN);
	case ')':
		return (TOK_RPAREN);
	case '.':
		c=inchar(sc);
		if(is_one_of(" \n\t",c)) {
			return (TOK_DOT);
		} else {
			backchar(sc,c);
			backchar(sc,'.');
			return TOK_ATOM;
		}
	case '\'':
		return (TOK_QUOTE);
	case ';':
		while ((c=inchar(sc)) != '\n' && c!=EOF)
			;

#if SHOW_ERROR_LINE
		if(c == '\n' && sc->load_stack[sc->file_i].kind & port_file)
			sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif

		if(c == EOF) {
			return (TOK_EOF);
		} else {
			return (token(sc));
		}
	case '"':
		return (TOK_DQUOTE);
	case BACKQUOTE:
		return (TOK_BQUOTE);
	case ',':
		if ((c=inchar(sc)) == '@') {
			return (TOK_ATMARK);
		} else {
			backchar(sc,c);
			return (TOK_COMMA);
		}
	case '#':
		c=inchar(sc);
		if (c == '(') {
			return (TOK_VEC);
		} else if(c == '!') {
			while ((c=inchar(sc)) != '\n' && c!=EOF)
				;

#if SHOW_ERROR_LINE
			if(c == '\n' && sc->load_stack[sc->file_i].kind & port_file)
				sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif

			if(c == EOF) {
				return (TOK_EOF);
			} else {
				return (token(sc));
			}
		} else {
			backchar(sc,c);
			if(is_one_of(" tfodxb\\",c)) {
				return TOK_SHARP_CONST;
			} else {
				return (TOK_SHARP);
			}
		}
	default:
		backchar(sc,c);
		return (TOK_ATOM);
	}
}

/* ========== Routines for Printing ========== */
#define   ok_abbrev(x)   (is_pair(x) && cdr(x) == sc->NIL)

/* ========== Routines for Evaluation Cycle ========== */

/* make closure. c is code. e is environment */
static cell_ptr_t mk_closure(scheme_t *sc, cell_ptr_t c, cell_ptr_t e) {
	cell_ptr_t x = get_cell(sc, c, e);

	typeflag(x) = T_CLOSURE;
	car(x) = c;
	cdr(x) = e;
	return (x);
}

/* make continuation. */
static cell_ptr_t mk_continuation(scheme_t *sc, cell_ptr_t d) {
	cell_ptr_t x = get_cell(sc, sc->NIL, d);

	typeflag(x) = T_CONTINUATION;
	cont_dump(x) = d;
	return (x);
}

static cell_ptr_t list_star(scheme_t *sc, cell_ptr_t d) {
	cell_ptr_t p, q;
	if(cdr(d)==sc->NIL) {
		return car(d);
	}
	p=cons(sc,car(d),cdr(d));
	q=p;
	while(cdr(cdr(p))!=sc->NIL) {
		d=cons(sc,car(p),cdr(p));
		if(cdr(cdr(p))!=sc->NIL) {
			p=cdr(d);
		}
	}
	cdr(p)=car(cdr(p));
	return q;
}

/* reverse list -- produce new list */
static cell_ptr_t reverse(scheme_t *sc, cell_ptr_t a) {
	/* a must be checked by gc */
	cell_ptr_t p = sc->NIL;

	for ( ; is_pair(a); a = cdr(a)) {
		p = cons(sc, car(a), p);
	}
	return (p);
}

/* reverse list --- in-place */
static cell_ptr_t reverse_in_place(scheme_t *sc, cell_ptr_t term, cell_ptr_t list) {
	cell_ptr_t p = list, result = term, q;

	while (p != sc->NIL) {
		q = cdr(p);
		cdr(p) = result;
		result = p;
		p = q;
	}
	return (result);
}

/* append list -- produce new list (in reverse order) */
static cell_ptr_t revappend(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	cell_ptr_t result = a;
	cell_ptr_t p = b;

	while (is_pair(p)) {
		result = cons(sc, car(p), result);
		p = cdr(p);
	}

	if (p == sc->NIL) {
		return result;
	}

	return sc->F;   /* signal an error */
}


/* ========== Evaluation Cycle ========== */

static cell_ptr_t opexe_0(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x, y;

	switch (op) {
	case OP_LOAD:       /* load */
		if(file_interactive(sc)) {
			fprintf(sc->outport->_object._port->rep.stdio.file,
			        "Loading %s\n", strvalue(car(sc->args)));
		}
		if (!file_push(sc,strvalue(car(sc->args)))) {
			error_1(sc,"unable to open", car(sc->args));
		} else {
			sc->args = mk_integer(sc,sc->file_i);
			s_goto(sc,OP_T0LVL);
		}

	case OP_T0LVL: /* top level */
		/* If we reached the end of file, this loop is done. */
		if(sc->loadport->_object._port->kind & port_saw_EOF) {
			if(sc->file_i == 0) {
				sc->args=sc->NIL;
				s_goto(sc,OP_QUIT);
			} else {
				file_pop(sc);
				s_return(sc,sc->value);
			}
			/* NOTREACHED */
		}

		/* If interactive, be nice to user. */
		if(file_interactive(sc)) {
			sc->envir = sc->global_env;
			dump_stack_reset(sc);
			putstr(sc,"\n");
			putstr(sc,prompt);
		}

		/* Set up another iteration of REPL */
		sc->nesting=0;
		sc->save_inport=sc->inport;
		sc->inport = sc->loadport;
		s_save(sc,OP_T0LVL, sc->NIL, sc->NIL);
		s_save(sc,OP_VALUEPRINT, sc->NIL, sc->NIL);
		s_save(sc,OP_T1LVL, sc->NIL, sc->NIL);
		s_goto(sc,OP_READ_INTERNAL);

	case OP_T1LVL: /* top level */
		sc->code = sc->value;
		sc->inport=sc->save_inport;
		s_goto(sc,OP_EVAL);

	case OP_READ_INTERNAL:       /* internal read */
		sc->tok = token(sc);
		if(sc->tok==TOK_EOF) {
			s_return(sc,sc->EOF_OBJ);
		}
		s_goto(sc,OP_RDSEXPR);

	case OP_GENSYM:
		s_return(sc, gensym(sc));

	case OP_VALUEPRINT: /* print evaluation result */
		/* OP_VALUEPRINT is always pushed, because when changing from
		 non-interactive to interactive mode, it needs to be
		 already on the stack */
		if(sc->tracing) {
			putstr(sc,"\nGives: ");
		}
		if(file_interactive(sc)) {
			sc->print_flag = 1;
			sc->args = sc->value;
			s_goto(sc,OP_P0LIST);
		} else {
			s_return(sc,sc->value);
		}

	case OP_EVAL:       /* main part of evaluation */
#if USE_TRACING
		if(sc->tracing) {
			/*s_save(sc,OP_VALUEPRINT,sc->NIL,sc->NIL);*/
			s_save(sc,OP_REAL_EVAL,sc->args,sc->code);
			sc->args=sc->code;
			putstr(sc,"\nEval: ");
			s_goto(sc,OP_P0LIST);
		}
		/* fall through */
	case OP_REAL_EVAL:
#endif
		if (is_symbol(sc->code)) {    /* symbol */
			x=find_slot_in_env(sc,sc->envir,sc->code,1);
			if (x != sc->NIL) {
				s_return(sc,slot_value_in_env(x));
			} else {
				error_1(sc,"eval: unbound variable:", sc->code);
			}
		} else if (is_pair(sc->code)) {
			if (is_syntax(x = car(sc->code))) {     /* SYNTAX */
				sc->code = cdr(sc->code);
				s_goto(sc,syntaxnum(x));
			} else { /* first, eval top element and eval arguments */
				s_save(sc,OP_E0ARGS, sc->NIL, sc->code);
				/* If no macros => s_save(sc,OP_E1ARGS, sc->NIL, cdr(sc->code));*/
				sc->code = car(sc->code);
				s_goto(sc,OP_EVAL);
			}
		} else {
			s_return(sc,sc->code);
		}

	case OP_E0ARGS:     /* eval arguments */
		if (is_macro(sc->value)) {    /* macro expansion */
			s_save(sc,OP_DOMACRO, sc->NIL, sc->NIL);
			sc->args = cons(sc,sc->code, sc->NIL);
			sc->code = sc->value;
			s_goto(sc,OP_APPLY);
		} else {
			sc->code = cdr(sc->code);
			s_goto(sc,OP_E1ARGS);
		}

	case OP_E1ARGS:     /* eval arguments */
		sc->args = cons(sc, sc->value, sc->args);
		if (is_pair(sc->code)) { /* continue */
			s_save(sc,OP_E1ARGS, sc->args, cdr(sc->code));
			sc->code = car(sc->code);
			sc->args = sc->NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->args = reverse_in_place(sc, sc->NIL, sc->args);
			sc->code = car(sc->args);
			sc->args = cdr(sc->args);
			s_goto(sc,OP_APPLY);
		}

#if USE_TRACING
	case OP_TRACING: {
		int tr=sc->tracing;
		sc->tracing=ivalue(car(sc->args));
		s_return(sc,mk_integer(sc,tr));
	}
#endif

	case OP_APPLY:      /* apply 'code' to 'args' */
#if USE_TRACING
		if(sc->tracing) {
			s_save(sc,OP_REAL_APPLY,sc->args,sc->code);
			sc->print_flag = 1;
			/*  sc->args=cons(sc,sc->code,sc->args);*/
			putstr(sc,"\nApply to: ");
			s_goto(sc,OP_P0LIST);
		}
		/* fall through */
	case OP_REAL_APPLY:
#endif
		if (is_proc(sc->code)) {
			s_goto(sc,procnum(sc->code));   /* PROCEDURE */
		} else if (is_foreign(sc->code)) {
			/* Keep nested calls from GC'ing the arglist */
			push_recent_alloc(sc,sc->args,sc->NIL);
			x=sc->code->_object._ff(sc,sc->args);
			s_return(sc,x);
		} else if (is_closure(sc->code) || is_macro(sc->code)
		           || is_promise(sc->code)) { /* CLOSURE */
			/* Should not accept promise */
			/* make environment */
			new_frame_in_env(sc, closure_env(sc->code));
			for (x = car(closure_code(sc->code)), y = sc->args;
			        is_pair(x); x = cdr(x), y = cdr(y)) {
				if (y == sc->NIL) {
					error_0(sc,"not enough arguments");
				} else {
					new_slot_in_env(sc, car(x), car(y));
				}
			}
			if (x == sc->NIL) {
				/*--
				* if (y != sc->NIL) {
				*   error_0(sc,"too many arguments");
				* }
				*/
			} else if (is_symbol(x))
				new_slot_in_env(sc, x, y);
			else {
				error_1(sc,"syntax error in closure: not a symbol:", x);
			}
			sc->code = cdr(closure_code(sc->code));
			sc->args = sc->NIL;
			s_goto(sc,OP_BEGIN);
		} else if (is_continuation(sc->code)) { /* CONTINUATION */
			sc->dump = cont_dump(sc->code);
			s_return(sc,sc->args != sc->NIL ? car(sc->args) : sc->NIL);
		} else {
			error_0(sc,"illegal function");
		}

	case OP_DOMACRO:    /* do macro */
		sc->code = sc->value;
		s_goto(sc,OP_EVAL);

#if 1
	case OP_LAMBDA:     /* lambda */
		/* If the hook is defined, apply it to sc->code, otherwise
		 set sc->value fall thru */
	{
		cell_ptr_t f=find_slot_in_env(sc,sc->envir,sc->COMPILE_HOOK,1);
		if(f==sc->NIL) {
			sc->value = sc->code;
			/* Fallthru */
		} else {
			s_save(sc,OP_LAMBDA1,sc->args,sc->code);
			sc->args=cons(sc,sc->code,sc->NIL);
			sc->code=slot_value_in_env(f);
			s_goto(sc,OP_APPLY);
		}
	}

	case OP_LAMBDA1:
		s_return(sc,mk_closure(sc, sc->value, sc->envir));

#else
	case OP_LAMBDA:     /* lambda */
		s_return(sc,mk_closure(sc, sc->code, sc->envir));

#endif

	case OP_MKCLOSURE: /* make-closure */
		x=car(sc->args);
		if(car(x)==sc->LAMBDA) {
			x=cdr(x);
		}
		if(cdr(sc->args)==sc->NIL) {
			y=sc->envir;
		} else {
			y=cadr(sc->args);
		}
		s_return(sc,mk_closure(sc, x, y));

	case OP_QUOTE:      /* quote */
		s_return(sc,car(sc->code));

	case OP_DEF0:  /* define */
		if(is_immutable(car(sc->code)))
			error_1(sc,"define: unable to alter immutable", car(sc->code));

		if (is_pair(car(sc->code))) {
			x = caar(sc->code);
			sc->code = cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), cdr(sc->code)));
		} else {
			x = car(sc->code);
			sc->code = cadr(sc->code);
		}
		if (!is_symbol(x)) {
			error_0(sc,"variable is not a symbol");
		}
		s_save(sc,OP_DEF1, sc->NIL, x);
		s_goto(sc,OP_EVAL);

	case OP_DEF1:  /* define */
		x=find_slot_in_env(sc,sc->envir,sc->code,0);
		if (x != sc->NIL) {
			set_slot_in_env(sc, x, sc->value);
		} else {
			new_slot_in_env(sc, sc->code, sc->value);
		}
		s_return(sc,sc->code);


	case OP_DEFP:  /* defined? */
		x=sc->envir;
		if(cdr(sc->args)!=sc->NIL) {
			x=cadr(sc->args);
		}
		s_retbool(find_slot_in_env(sc,x,car(sc->args),1)!=sc->NIL);

	case OP_SET0:       /* set! */
		if(is_immutable(car(sc->code)))
			error_1(sc,"set!: unable to alter immutable variable",car(sc->code));
		s_save(sc,OP_SET1, sc->NIL, car(sc->code));
		sc->code = cadr(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_SET1:       /* set! */
		y=find_slot_in_env(sc,sc->envir,sc->code,1);
		if (y != sc->NIL) {
			set_slot_in_env(sc, y, sc->value);
			s_return(sc,sc->value);
		} else {
			error_1(sc,"set!: unbound variable:", sc->code);
		}


	case OP_BEGIN:      /* begin */
		if (!is_pair(sc->code)) {
			s_return(sc,sc->code);
		}
		if (cdr(sc->code) != sc->NIL) {
			s_save(sc,OP_BEGIN, sc->NIL, cdr(sc->code));
		}
		sc->code = car(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_IF0:        /* if */
		s_save(sc,OP_IF1, sc->NIL, cdr(sc->code));
		sc->code = car(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_IF1:        /* if */
		if (is_true(sc->value))
			sc->code = car(sc->code);
		else
			sc->code = cadr(sc->code);  /* (if #f 1) ==> () because
					    * car(sc->NIL) = sc->NIL */
		s_goto(sc,OP_EVAL);

	case OP_LET0:       /* let */
		sc->args = sc->NIL;
		sc->value = sc->code;
		sc->code = is_symbol(car(sc->code)) ? cadr(sc->code) : car(sc->code);
		s_goto(sc,OP_LET1);

	case OP_LET1:       /* let (calculate parameters) */
		sc->args = cons(sc, sc->value, sc->args);
		if (is_pair(sc->code)) { /* continue */
			if (!is_pair(car(sc->code)) || !is_pair(cdar(sc->code))) {
				error_1(sc, "Bad syntax of binding spec in let :",
				        car(sc->code));
			}
			s_save(sc,OP_LET1, sc->args, cdr(sc->code));
			sc->code = cadar(sc->code);
			sc->args = sc->NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->args = reverse_in_place(sc, sc->NIL, sc->args);
			sc->code = car(sc->args);
			sc->args = cdr(sc->args);
			s_goto(sc,OP_LET2);
		}

	case OP_LET2:       /* let */
		new_frame_in_env(sc, sc->envir);
		for (x = is_symbol(car(sc->code)) ? cadr(sc->code) : car(sc->code), y = sc->args;
		        y != sc->NIL; x = cdr(x), y = cdr(y)) {
			new_slot_in_env(sc, caar(x), car(y));
		}
		if (is_symbol(car(sc->code))) {    /* named let */
			for (x = cadr(sc->code), sc->args = sc->NIL; x != sc->NIL; x = cdr(x)) {
				if (!is_pair(x))
					error_1(sc, "Bad syntax of binding in let :", x);
				if (!is_list(sc, car(x)))
					error_1(sc, "Bad syntax of binding in let :", car(x));
				sc->args = cons(sc, caar(x), sc->args);
			}
			x = mk_closure(sc, cons(sc, reverse_in_place(sc, sc->NIL, sc->args), cddr(sc->code)), sc->envir);
			new_slot_in_env(sc, car(sc->code), x);
			sc->code = cddr(sc->code);
			sc->args = sc->NIL;
		} else {
			sc->code = cdr(sc->code);
			sc->args = sc->NIL;
		}
		s_goto(sc,OP_BEGIN);

	case OP_LET0AST:    /* let* */
		if (car(sc->code) == sc->NIL) {
			new_frame_in_env(sc, sc->envir);
			sc->code = cdr(sc->code);
			s_goto(sc,OP_BEGIN);
		}
		if(!is_pair(car(sc->code)) || !is_pair(caar(sc->code)) || !is_pair(cdaar(sc->code))) {
			error_1(sc,"Bad syntax of binding spec in let* :",car(sc->code));
		}
		s_save(sc,OP_LET1AST, cdr(sc->code), car(sc->code));
		sc->code = cadaar(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_LET1AST:    /* let* (make new frame) */
		new_frame_in_env(sc, sc->envir);
		s_goto(sc,OP_LET2AST);

	case OP_LET2AST:    /* let* (calculate parameters) */
		new_slot_in_env(sc, caar(sc->code), sc->value);
		sc->code = cdr(sc->code);
		if (is_pair(sc->code)) { /* continue */
			s_save(sc,OP_LET2AST, sc->args, sc->code);
			sc->code = cadar(sc->code);
			sc->args = sc->NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->code = sc->args;
			sc->args = sc->NIL;
			s_goto(sc,OP_BEGIN);
		}
	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->T;
}

static cell_ptr_t opexe_1(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x, y;

	switch (op) {
	case OP_LET0REC:    /* letrec */
		new_frame_in_env(sc, sc->envir);
		sc->args = sc->NIL;
		sc->value = sc->code;
		sc->code = car(sc->code);
		s_goto(sc,OP_LET1REC);

	case OP_LET1REC:    /* letrec (calculate parameters) */
		sc->args = cons(sc, sc->value, sc->args);
		if (is_pair(sc->code)) { /* continue */
			if (!is_pair(car(sc->code)) || !is_pair(cdar(sc->code))) {
				error_1(sc, "Bad syntax of binding spec in letrec :",
				        car(sc->code));
			}
			s_save(sc,OP_LET1REC, sc->args, cdr(sc->code));
			sc->code = cadar(sc->code);
			sc->args = sc->NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->args = reverse_in_place(sc, sc->NIL, sc->args);
			sc->code = car(sc->args);
			sc->args = cdr(sc->args);
			s_goto(sc,OP_LET2REC);
		}

	case OP_LET2REC:    /* letrec */
		for (x = car(sc->code), y = sc->args; y != sc->NIL; x = cdr(x), y = cdr(y)) {
			new_slot_in_env(sc, caar(x), car(y));
		}
		sc->code = cdr(sc->code);
		sc->args = sc->NIL;
		s_goto(sc,OP_BEGIN);

	case OP_COND0:      /* cond */
		if (!is_pair(sc->code)) {
			error_0(sc,"syntax error in cond");
		}
		s_save(sc,OP_COND1, sc->NIL, sc->code);
		sc->code = caar(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_COND1:      /* cond */
		if (is_true(sc->value)) {
			if ((sc->code = cdar(sc->code)) == sc->NIL) {
				s_return(sc,sc->value);
			}
			if(car(sc->code)==sc->FEED_TO) {
				if(!is_pair(cdr(sc->code))) {
					error_0(sc,"syntax error in cond");
				}
				x=cons(sc, sc->QUOTE, cons(sc, sc->value, sc->NIL));
				sc->code=cons(sc,cadr(sc->code),cons(sc,x,sc->NIL));
				s_goto(sc,OP_EVAL);
			}
			s_goto(sc,OP_BEGIN);
		} else {
			if ((sc->code = cdr(sc->code)) == sc->NIL) {
				s_return(sc,sc->NIL);
			} else {
				s_save(sc,OP_COND1, sc->NIL, sc->code);
				sc->code = caar(sc->code);
				s_goto(sc,OP_EVAL);
			}
		}

	case OP_DELAY:      /* delay */
		x = mk_closure(sc, cons(sc, sc->NIL, sc->code), sc->envir);
		typeflag(x)=T_PROMISE;
		s_return(sc,x);

	case OP_AND0:       /* and */
		if (sc->code == sc->NIL) {
			s_return(sc,sc->T);
		}
		s_save(sc,OP_AND1, sc->NIL, cdr(sc->code));
		sc->code = car(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_AND1:       /* and */
		if (is_false(sc->value)) {
			s_return(sc,sc->value);
		} else if (sc->code == sc->NIL) {
			s_return(sc,sc->value);
		} else {
			s_save(sc,OP_AND1, sc->NIL, cdr(sc->code));
			sc->code = car(sc->code);
			s_goto(sc,OP_EVAL);
		}

	case OP_OR0:        /* or */
		if (sc->code == sc->NIL) {
			s_return(sc,sc->F);
		}
		s_save(sc,OP_OR1, sc->NIL, cdr(sc->code));
		sc->code = car(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_OR1:        /* or */
		if (is_true(sc->value)) {
			s_return(sc,sc->value);
		} else if (sc->code == sc->NIL) {
			s_return(sc,sc->value);
		} else {
			s_save(sc,OP_OR1, sc->NIL, cdr(sc->code));
			sc->code = car(sc->code);
			s_goto(sc,OP_EVAL);
		}

	case OP_C0STREAM:   /* cons-stream */
		s_save(sc,OP_C1STREAM, sc->NIL, cdr(sc->code));
		sc->code = car(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_C1STREAM:   /* cons-stream */
		sc->args = sc->value;  /* save sc->value to register sc->args for gc */
		x = mk_closure(sc, cons(sc, sc->NIL, sc->code), sc->envir);
		typeflag(x)=T_PROMISE;
		s_return(sc,cons(sc, sc->args, x));

	case OP_MACRO0:     /* macro */
		if (is_pair(car(sc->code))) {
			x = caar(sc->code);
			sc->code = cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), cdr(sc->code)));
		} else {
			x = car(sc->code);
			sc->code = cadr(sc->code);
		}
		if (!is_symbol(x)) {
			error_0(sc,"variable is not a symbol");
		}
		s_save(sc,OP_MACRO1, sc->NIL, x);
		s_goto(sc,OP_EVAL);

	case OP_MACRO1:     /* macro */
		typeflag(sc->value) = T_MACRO;
		x = find_slot_in_env(sc, sc->envir, sc->code, 0);
		if (x != sc->NIL) {
			set_slot_in_env(sc, x, sc->value);
		} else {
			new_slot_in_env(sc, sc->code, sc->value);
		}
		s_return(sc,sc->code);

	case OP_CASE0:      /* case */
		s_save(sc,OP_CASE1, sc->NIL, cdr(sc->code));
		sc->code = car(sc->code);
		s_goto(sc,OP_EVAL);

	case OP_CASE1:      /* case */
		for (x = sc->code; x != sc->NIL; x = cdr(x)) {
			if (!is_pair(y = caar(x))) {
				break;
			}
			for ( ; y != sc->NIL; y = cdr(y)) {
				if (eqv(car(y), sc->value)) {
					break;
				}
			}
			if (y != sc->NIL) {
				break;
			}
		}
		if (x != sc->NIL) {
			if (is_pair(caar(x))) {
				sc->code = cdar(x);
				s_goto(sc,OP_BEGIN);
			} else { /* else */
				s_save(sc,OP_CASE2, sc->NIL, cdar(x));
				sc->code = caar(x);
				s_goto(sc,OP_EVAL);
			}
		} else {
			s_return(sc,sc->NIL);
		}

	case OP_CASE2:      /* case */
		if (is_true(sc->value)) {
			s_goto(sc,OP_BEGIN);
		} else {
			s_return(sc,sc->NIL);
		}

	case OP_PAPPLY:     /* apply */
		sc->code = car(sc->args);
		sc->args = list_star(sc,cdr(sc->args));
		/*sc->args = cadr(sc->args);*/
		s_goto(sc,OP_APPLY);

	case OP_PEVAL: /* eval */
		if(cdr(sc->args)!=sc->NIL) {
			sc->envir=cadr(sc->args);
		}
		sc->code = car(sc->args);
		s_goto(sc,OP_EVAL);

	case OP_CONTINUATION:    /* call-with-current-continuation */
		sc->code = car(sc->args);
		sc->args = cons(sc, mk_continuation(sc, sc->dump), sc->NIL);
		s_goto(sc,OP_APPLY);

	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->T;
}

static cell_ptr_t opexe_2(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x;

	switch( op ) {
	case OP_CAR:        /* car */
		s_return(sc,caar(sc->args));

	case OP_CDR:        /* cdr */
		s_return(sc,cdar(sc->args));

	case OP_CONS:       /* cons */
		cdr(sc->args) = cadr(sc->args);
		s_return(sc,sc->args);

	case OP_SETCAR:     /* set-car! */
		if(!is_immutable(car(sc->args))) {
			caar(sc->args) = cadr(sc->args);
			s_return(sc,car(sc->args));
		} else {
			error_0(sc,"set-car!: unable to alter immutable pair");
		}

	case OP_SETCDR:     /* set-cdr! */
		if(!is_immutable(car(sc->args))) {
			cdar(sc->args) = cadr(sc->args);
			s_return(sc,car(sc->args));
		} else {
			error_0(sc,"set-cdr!: unable to alter immutable pair");
		}

	case OP_CHAR2INT: { /* char->integer */
		char c;
		c=(char)ivalue(car(sc->args));
		s_return(sc,mk_integer(sc,(unsigned char)c));
	}

	case OP_INT2CHAR: { /* integer->char */
		unsigned char c;
		c=(unsigned char)ivalue(car(sc->args));
		s_return(sc,mk_character(sc,(char)c));
	}

	case OP_CHARUPCASE: {
		unsigned char c;
		c=(unsigned char)ivalue(car(sc->args));
		c=toupper(c);
		s_return(sc,mk_character(sc,(char)c));
	}

	case OP_CHARDNCASE: {
		unsigned char c;
		c=(unsigned char)ivalue(car(sc->args));
		c=tolower(c);
		s_return(sc,mk_character(sc,(char)c));
	}

	case OP_STR2SYM:  /* string->symbol */
		s_return(sc,mk_symbol(sc,strvalue(car(sc->args))));

	case OP_STR2ATOM: { /* string->atom */
		char *s=strvalue(car(sc->args));
		long pf = 0;
		if(cdr(sc->args)!=sc->NIL) {
			/* we know cadr(sc->args) is a natural number */
			/* see if it is 2, 8, 10, or 16, or error */
			pf = ivalue_unchecked(cadr(sc->args));
			if(pf == 16 || pf == 10 || pf == 8 || pf == 2) {
				/* base is OK */
			} else {
				pf = -1;
			}
		}
		if (pf < 0) {
			error_1(sc, "string->atom: bad base:", cadr(sc->args));
		} else if(*s=='#') { /* no use of base! */
			s_return(sc, mk_sharp_const(sc, s+1));
		} else {
			if (pf == 0 || pf == 10) {
				s_return(sc, mk_atom(sc, s));
			} else {
				char *ep;
				long iv = strtol(s,&ep,(int )pf);
				if (*ep == 0) {
					s_return(sc, mk_integer(sc, iv));
				} else {
					s_return(sc, sc->F);
				}
			}
		}
	}

	case OP_SYM2STR: /* symbol->string */
		x=mk_string(sc,symname(car(sc->args)));
		setimmutable(x);
		s_return(sc,x);

	case OP_ATOM2STR: { /* atom->string */
		long pf = 0;
		x=car(sc->args);
		if(cdr(sc->args)!=sc->NIL) {
			/* we know cadr(sc->args) is a natural number */
			/* see if it is 2, 8, 10, or 16, or error */
			pf = ivalue_unchecked(cadr(sc->args));
			if(is_number(x) && (pf == 16 || pf == 10 || pf == 8 || pf == 2)) {
				/* base is OK */
			} else {
				pf = -1;
			}
		}
		if (pf < 0) {
			error_1(sc, "atom->string: bad base:", cadr(sc->args));
		} else if(is_number(x) || is_character(x) || is_string(x) || is_symbol(x)) {
			char *p;
			int len;
			atom2str(sc,x,(int )pf,&p,&len);
			s_return(sc,mk_counted_string(sc,p,len));
		} else {
			error_1(sc, "atom->string: not an atom:", x);
		}
	}

	case OP_MKSTRING: { /* make-string */
		int fill=' ';
		int len;

		len=ivalue(car(sc->args));

		if(cdr(sc->args)!=sc->NIL) {
			fill=charvalue(cadr(sc->args));
		}
		s_return(sc,mk_empty_string(sc,len,(char)fill));
	}

	case OP_STRLEN:  /* string-length */
		s_return(sc,mk_integer(sc,strlength(car(sc->args))));

	case OP_STRREF: { /* string-ref */
		char *str;
		int index;

		str=strvalue(car(sc->args));

		index=ivalue(cadr(sc->args));

		if(index>=strlength(car(sc->args))) {
			error_1(sc,"string-ref: out of bounds:",cadr(sc->args));
		}

		s_return(sc,mk_character(sc,((unsigned char*)str)[index]));
	}

	case OP_STRSET: { /* string-set! */
		char *str;
		int index;
		int c;

		if(is_immutable(car(sc->args))) {
			error_1(sc,"string-set!: unable to alter immutable string:",car(sc->args));
		}
		str=strvalue(car(sc->args));

		index=ivalue(cadr(sc->args));
		if(index>=strlength(car(sc->args))) {
			error_1(sc,"string-set!: out of bounds:",cadr(sc->args));
		}

		c=charvalue(caddr(sc->args));

		str[index]=(char)c;
		s_return(sc,car(sc->args));
	}

	case OP_STRAPPEND: { /* string-append */
		/* in 1.29 string-append was in Scheme in init.scm but was too slow */
		int len = 0;
		cell_ptr_t newstr;
		char *pos;

		/* compute needed length for new string */
		for (x = sc->args; x != sc->NIL; x = cdr(x)) {
			len += strlength(car(x));
		}
		newstr = mk_empty_string(sc, len, ' ');
		/* store the contents of the argument strings into the new string */
		for (pos = strvalue(newstr), x = sc->args; x != sc->NIL;
		        pos += strlength(car(x)), x = cdr(x)) {
			memcpy(pos, strvalue(car(x)), strlength(car(x)));
		}
		s_return(sc, newstr);
	}

	case OP_SUBSTR: { /* substring */
		char *str;
		int index0;
		int index1;
		int len;

		str=strvalue(car(sc->args));

		index0=ivalue(cadr(sc->args));

		if(index0>strlength(car(sc->args))) {
			error_1(sc,"substring: start out of bounds:",cadr(sc->args));
		}

		if(cddr(sc->args)!=sc->NIL) {
			index1=ivalue(caddr(sc->args));
			if(index1>strlength(car(sc->args)) || index1<index0) {
				error_1(sc,"substring: end out of bounds:",caddr(sc->args));
			}
		} else {
			index1=strlength(car(sc->args));
		}

		len=index1-index0;
		x=mk_empty_string(sc,len,' ');
		memcpy(strvalue(x),str+index0,len);
		strvalue(x)[len]=0;

		s_return(sc,x);
	}

	case OP_VECTOR: {   /* vector */
		int i;
		cell_ptr_t vec;
		int len=list_length(sc,sc->args);
		if(len<0) {
			error_1(sc,"vector: not a proper list:",sc->args);
		}
		vec=mk_vector(sc,len);
		if(sc->no_memory) {
			s_return(sc, sc->sink);
		}
		for (x = sc->args, i = 0; is_pair(x); x = cdr(x), i++) {
			set_vector_elem(vec,i,car(x));
		}
		s_return(sc,vec);
	}

	case OP_MKVECTOR: { /* make-vector */
		cell_ptr_t fill=sc->NIL;
		int len;
		cell_ptr_t vec;

		len=ivalue(car(sc->args));

		if(cdr(sc->args)!=sc->NIL) {
			fill=cadr(sc->args);
		}
		vec=mk_vector(sc,len);
		if(sc->no_memory) {
			s_return(sc, sc->sink);
		}
		if(fill!=sc->NIL) {
			fill_vector(vec,fill);
		}
		s_return(sc,vec);
	}

	case OP_VECLEN:  /* vector-length */
		s_return(sc,mk_integer(sc,ivalue(car(sc->args))));

	case OP_VECREF: { /* vector-ref */
		int index;

		index=ivalue(cadr(sc->args));

		if(index>=ivalue(car(sc->args))) {
			error_1(sc,"vector-ref: out of bounds:",cadr(sc->args));
		}

		s_return(sc,vector_elem(car(sc->args),index));
	}

	case OP_VECSET: {   /* vector-set! */
		int index;

		if(is_immutable(car(sc->args))) {
			error_1(sc,"vector-set!: unable to alter immutable vector:",car(sc->args));
		}

		index=ivalue(cadr(sc->args));
		if(index>=ivalue(car(sc->args))) {
			error_1(sc,"vector-set!: out of bounds:",cadr(sc->args));
		}

		set_vector_elem(car(sc->args),index,caddr(sc->args));
		s_return(sc,car(sc->args));
	}

	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->T;
}

static int is_list(scheme_t *sc, cell_ptr_t a) {
	return list_length(sc,a) >= 0;
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


static cell_ptr_t opexe_4(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x, y;

	switch (op) {
	case OP_FORCE:      /* force */
		sc->code = car(sc->args);
		if (is_promise(sc->code)) {
			/* Should change type to closure here */
			s_save(sc, OP_SAVE_FORCED, sc->NIL, sc->code);
			sc->args = sc->NIL;
			s_goto(sc,OP_APPLY);
		} else {
			s_return(sc,sc->code);
		}

	case OP_SAVE_FORCED:     /* Save forced value replacing promise */
		memcpy(sc->code,sc->value,sizeof(cell_t));
		s_return(sc,sc->value);

	case OP_WRITE:      /* write */
	case OP_DISPLAY:    /* display */
	case OP_WRITE_CHAR: /* write-char */
		if(is_pair(cdr(sc->args))) {
			if(cadr(sc->args)!=sc->outport) {
				x=cons(sc,sc->outport,sc->NIL);
				s_save(sc,OP_SET_OUTPORT, x, sc->NIL);
				sc->outport=cadr(sc->args);
			}
		}
		sc->args = car(sc->args);
		if(op==OP_WRITE) {
			sc->print_flag = 1;
		} else {
			sc->print_flag = 0;
		}
		s_goto(sc,OP_P0LIST);

	case OP_NEWLINE:    /* newline */
		if(is_pair(sc->args)) {
			if(car(sc->args)!=sc->outport) {
				x=cons(sc,sc->outport,sc->NIL);
				s_save(sc,OP_SET_OUTPORT, x, sc->NIL);
				sc->outport=car(sc->args);
			}
		}
		putstr(sc, "\n");
		s_return(sc,sc->T);

	case OP_ERR0:  /* error */
		sc->retcode=-1;
		if (!is_string(car(sc->args))) {
			sc->args=cons(sc,mk_string(sc," -- "),sc->args);
			setimmutable(car(sc->args));
		}
		putstr(sc, "Error: ");
		putstr(sc, strvalue(car(sc->args)));
		sc->args = cdr(sc->args);
		s_goto(sc,OP_ERR1);

	case OP_ERR1:  /* error */
		putstr(sc, " ");
		if (sc->args != sc->NIL) {
			s_save(sc,OP_ERR1, cdr(sc->args), sc->NIL);
			sc->args = car(sc->args);
			sc->print_flag = 1;
			s_goto(sc,OP_P0LIST);
		} else {
			putstr(sc, "\n");
			if(sc->interactive_repl) {
				s_goto(sc,OP_T0LVL);
			} else {
				return sc->NIL;
			}
		}

	case OP_REVERSE:   /* reverse */
		s_return(sc,reverse(sc, car(sc->args)));

	case OP_LIST_STAR: /* list* */
		s_return(sc,list_star(sc,sc->args));

	case OP_APPEND:    /* append */
		x = sc->NIL;
		y = sc->args;
		if (y == x) {
			s_return(sc, x);
		}

		/* cdr() in the while condition is not a typo. If car() */
		/* is used (append '() 'a) will return the wrong result.*/
		while (cdr(y) != sc->NIL) {
			x = revappend(sc, x, car(y));
			y = cdr(y);
			if (x == sc->F) {
				error_0(sc, "non-list argument to append");
			}
		}

		s_return(sc, reverse_in_place(sc, car(y), x));

#if USE_PLIST
	case OP_PUT:        /* put */
		if (!hasprop(car(sc->args)) || !hasprop(cadr(sc->args))) {
			error_0(sc,"illegal use of put");
		}
		for (x = symprop(car(sc->args)), y = cadr(sc->args); x != sc->NIL; x = cdr(x)) {
			if (caar(x) == y) {
				break;
			}
		}
		if (x != sc->NIL)
			cdar(x) = caddr(sc->args);
		else
			symprop(car(sc->args)) = cons(sc, cons(sc, y, caddr(sc->args)),
			                              symprop(car(sc->args)));
		s_return(sc,sc->T);

	case OP_GET:        /* get */
		if (!hasprop(car(sc->args)) || !hasprop(cadr(sc->args))) {
			error_0(sc,"illegal use of get");
		}
		for (x = symprop(car(sc->args)), y = cadr(sc->args); x != sc->NIL; x = cdr(x)) {
			if (caar(x) == y) {
				break;
			}
		}
		if (x != sc->NIL) {
			s_return(sc,cdar(x));
		} else {
			s_return(sc,sc->NIL);
		}
#endif /* USE_PLIST */
	case OP_QUIT:       /* quit */
		if(is_pair(sc->args)) {
			sc->retcode=ivalue(car(sc->args));
		}
		return (sc->NIL);

	case OP_GC:         /* gc */
		gc(sc, sc->NIL, sc->NIL);
		s_return(sc,sc->T);

	case OP_GCVERB: {        /* gc-verbose */
		int  was = sc->gc_verbose;

		sc->gc_verbose = (car(sc->args) != sc->F);
		s_retbool(was);
	}

	case OP_OBLIST: /* oblist */
		s_return(sc, oblist_all_symbols(sc));

	case OP_CURR_INPORT: /* current-input-port */
		s_return(sc,sc->inport);

	case OP_CURR_OUTPORT: /* current-output-port */
		s_return(sc,sc->outport);

	case OP_OPEN_INFILE: /* open-input-file */
	case OP_OPEN_OUTFILE: /* open-output-file */
	case OP_OPEN_INOUTFILE: { /* open-input-output-file */
		int prop=0;
		cell_ptr_t p;
		switch(op) {
		case OP_OPEN_INFILE:
			prop=port_input;
			break;
		case OP_OPEN_OUTFILE:
			prop=port_output;
			break;
		case OP_OPEN_INOUTFILE:
			prop=port_input|port_output;
			break;
		default:
			break;
		}
		p=port_from_filename(sc,strvalue(car(sc->args)),prop);
		if(p==sc->NIL) {
			s_return(sc,sc->F);
		}
		s_return(sc,p);
	}

#if USE_STRING_PORTS
	case OP_OPEN_INSTRING: /* open-input-string */
	case OP_OPEN_INOUTSTRING: { /* open-input-output-string */
		int prop=0;
		cell_ptr_t p;
		switch(op) {
		case OP_OPEN_INSTRING:
			prop=port_input;
			break;
		case OP_OPEN_INOUTSTRING:
			prop=port_input|port_output;
			break;
		default:
			break;
		}
		p=port_from_string(sc, strvalue(car(sc->args)),
		                   strvalue(car(sc->args))+strlength(car(sc->args)), prop);
		if(p==sc->NIL) {
			s_return(sc,sc->F);
		}
		s_return(sc,p);
	}
	case OP_OPEN_OUTSTRING: { /* open-output-string */
		cell_ptr_t p;
		if(car(sc->args)==sc->NIL) {
			p=port_from_scratch(sc);
			if(p==sc->NIL) {
				s_return(sc,sc->F);
			}
		} else {
			p=port_from_string(sc, strvalue(car(sc->args)),
			                   strvalue(car(sc->args))+strlength(car(sc->args)),
			                   port_output);
			if(p==sc->NIL) {
				s_return(sc,sc->F);
			}
		}
		s_return(sc,p);
	}
	case OP_GET_OUTSTRING: { /* get-output-string */
		port_t *p;

		if ((p=car(sc->args)->_object._port)->kind & port_string) {
			size_t size;
			char *str;

			size=p->rep.string.curr-p->rep.string.start+1;
			str=sc->malloc(size);
			if(str != NULL) {
				cell_ptr_t s;

				memcpy(str,p->rep.string.start,size-1);
				str[size-1]='\0';
				s=mk_string(sc,str);
				sc->free(str);
				s_return(sc,s);
			}
		}
		s_return(sc,sc->F);
	}
#endif

	case OP_CLOSE_INPORT: /* close-input-port */
		port_close(sc,car(sc->args),port_input);
		s_return(sc,sc->T);

	case OP_CLOSE_OUTPORT: /* close-output-port */
		port_close(sc,car(sc->args),port_output);
		s_return(sc,sc->T);

	case OP_INT_ENV: /* interaction-environment */
		s_return(sc,sc->global_env);

	case OP_CURR_ENV: /* current-environment */
		s_return(sc,sc->envir);

	default:
		break;

	}
	return sc->T;
}

static cell_ptr_t opexe_5(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x;

	if(sc->nesting!=0) {
		int n=sc->nesting;
		sc->nesting=0;
		sc->retcode=-1;
		error_1(sc,"unmatched parentheses:",mk_integer(sc,n));
	}

	switch (op) {
		/* ========== reading part ========== */
	case OP_READ:
		if(!is_pair(sc->args)) {
			s_goto(sc,OP_READ_INTERNAL);
		}
		if(!is_inport(car(sc->args))) {
			error_1(sc,"read: not an input port:",car(sc->args));
		}
		if(car(sc->args)==sc->inport) {
			s_goto(sc,OP_READ_INTERNAL);
		}
		x=sc->inport;
		sc->inport=car(sc->args);
		x=cons(sc,x,sc->NIL);
		s_save(sc,OP_SET_INPORT, x, sc->NIL);
		s_goto(sc,OP_READ_INTERNAL);

	case OP_READ_CHAR: /* read-char */
	case OP_PEEK_CHAR: { /* peek-char */
		int c;
		if(is_pair(sc->args)) {
			if(car(sc->args)!=sc->inport) {
				x=sc->inport;
				x=cons(sc,x,sc->NIL);
				s_save(sc,OP_SET_INPORT, x, sc->NIL);
				sc->inport=car(sc->args);
			}
		}
		c=inchar(sc);
		if(c==EOF) {
			s_return(sc,sc->EOF_OBJ);
		}
		if(sc->op==OP_PEEK_CHAR) {
			backchar(sc,c);
		}
		s_return(sc,mk_character(sc,c));
	}

	case OP_CHAR_READY: { /* char-ready? */
		cell_ptr_t p=sc->inport;
		int res;
		if(is_pair(sc->args)) {
			p=car(sc->args);
		}
		res=p->_object._port->kind&port_string;
		s_retbool(res);
	}

	case OP_SET_INPORT: /* set-input-port */
		sc->inport=car(sc->args);
		s_return(sc,sc->value);

	case OP_SET_OUTPORT: /* set-output-port */
		sc->outport=car(sc->args);
		s_return(sc,sc->value);

	case OP_RDSEXPR:
		switch (sc->tok) {
		case TOK_EOF:
			s_return(sc,sc->EOF_OBJ);
			/* NOTREACHED */
			/*
			* Commented out because we now skip comments in the scanner
			*
			case TOK_COMMENT: {
			int c;
			while ((c=inchar(sc)) != '\n' && c!=EOF)
			;
			sc->tok = token(sc);
			s_goto(sc,OP_RDSEXPR);
			}
			*/
		case TOK_VEC:
			s_save(sc,OP_RDVEC,sc->NIL,sc->NIL);
			/* fall through */
		case TOK_LPAREN:
			sc->tok = token(sc);
			if (sc->tok == TOK_RPAREN) {
				s_return(sc,sc->NIL);
			} else if (sc->tok == TOK_DOT) {
				error_0(sc,"syntax error: illegal dot expression");
			} else {
				sc->nesting_stack[sc->file_i]++;
				s_save(sc,OP_RDLIST, sc->NIL, sc->NIL);
				s_goto(sc,OP_RDSEXPR);
			}
		case TOK_QUOTE:
			s_save(sc,OP_RDQUOTE, sc->NIL, sc->NIL);
			sc->tok = token(sc);
			s_goto(sc,OP_RDSEXPR);
		case TOK_BQUOTE:
			sc->tok = token(sc);
			if(sc->tok==TOK_VEC) {
				s_save(sc,OP_RDQQUOTEVEC, sc->NIL, sc->NIL);
				sc->tok=TOK_LPAREN;
				s_goto(sc,OP_RDSEXPR);
			} else {
				s_save(sc,OP_RDQQUOTE, sc->NIL, sc->NIL);
			}
			s_goto(sc,OP_RDSEXPR);
		case TOK_COMMA:
			s_save(sc,OP_RDUNQUOTE, sc->NIL, sc->NIL);
			sc->tok = token(sc);
			s_goto(sc,OP_RDSEXPR);
		case TOK_ATMARK:
			s_save(sc,OP_RDUQTSP, sc->NIL, sc->NIL);
			sc->tok = token(sc);
			s_goto(sc,OP_RDSEXPR);
		case TOK_ATOM:
			s_return(sc,mk_atom(sc, readstr_upto(sc, DELIMITERS)));
		case TOK_DQUOTE:
			x=readstrexp(sc);
			if(x==sc->F) {
				error_0(sc,"Error reading string");
			}
			setimmutable(x);
			s_return(sc,x);
		case TOK_SHARP: {
			cell_ptr_t f=find_slot_in_env(sc,sc->envir,sc->SHARP_HOOK,1);
			if(f==sc->NIL) {
				error_0(sc,"undefined sharp expression");
			} else {
				sc->code=cons(sc,slot_value_in_env(f),sc->NIL);
				s_goto(sc,OP_EVAL);
			}
		}
		case TOK_SHARP_CONST:
			if ((x = mk_sharp_const(sc, readstr_upto(sc, DELIMITERS))) == sc->NIL) {
				error_0(sc,"undefined sharp expression");
			} else {
				s_return(sc,x);
			}
		default:
			error_0(sc,"syntax error: illegal token");
		}
		break;

	case OP_RDLIST: {
		sc->args = cons(sc, sc->value, sc->args);
		sc->tok = token(sc);
		/* We now skip comments in the scanner
		while (sc->tok == TOK_COMMENT) {
		   int c;
		   while ((c=inchar(sc)) != '\n' && c!=EOF)
		    ;
		   sc->tok = token(sc);
		}
		*/
		if (sc->tok == TOK_EOF) {
			s_return(sc,sc->EOF_OBJ);
		} else if (sc->tok == TOK_RPAREN) {
			int c = inchar(sc);
			if (c != '\n')
				backchar(sc,c);
#if SHOW_ERROR_LINE
			else if (sc->load_stack[sc->file_i].kind & port_file)
				sc->load_stack[sc->file_i].rep.stdio.curr_line++;
#endif
			sc->nesting_stack[sc->file_i]--;
			s_return(sc,reverse_in_place(sc, sc->NIL, sc->args));
		} else if (sc->tok == TOK_DOT) {
			s_save(sc,OP_RDDOT, sc->args, sc->NIL);
			sc->tok = token(sc);
			s_goto(sc,OP_RDSEXPR);
		} else {
			s_save(sc,OP_RDLIST, sc->args, sc->NIL);;
			s_goto(sc,OP_RDSEXPR);
		}
	}

	case OP_RDDOT:
		if (token(sc) != TOK_RPAREN) {
			error_0(sc,"syntax error: illegal dot expression");
		} else {
			sc->nesting_stack[sc->file_i]--;
			s_return(sc,reverse_in_place(sc, sc->value, sc->args));
		}

	case OP_RDQUOTE:
		s_return(sc,cons(sc, sc->QUOTE, cons(sc, sc->value, sc->NIL)));

	case OP_RDQQUOTE:
		s_return(sc,cons(sc, sc->QQUOTE, cons(sc, sc->value, sc->NIL)));

	case OP_RDQQUOTEVEC:
		s_return(sc,cons(sc, mk_symbol(sc,"apply"),
		                 cons(sc, mk_symbol(sc,"vector"),
		                      cons(sc,cons(sc, sc->QQUOTE,
		                                   cons(sc,sc->value,sc->NIL)),
		                           sc->NIL))));

	case OP_RDUNQUOTE:
		s_return(sc,cons(sc, sc->UNQUOTE, cons(sc, sc->value, sc->NIL)));

	case OP_RDUQTSP:
		s_return(sc,cons(sc, sc->UNQUOTESP, cons(sc, sc->value, sc->NIL)));

	case OP_RDVEC:
		/*sc->code=cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
		s_goto(sc,OP_EVAL); Cannot be quoted*/
		/*x=cons(sc,mk_proc(sc,OP_VECTOR),sc->value);
		s_return(sc,x); Cannot be part of pairs*/
		/*sc->code=mk_proc(sc,OP_VECTOR);
		sc->args=sc->value;
		s_goto(sc,OP_APPLY);*/
		sc->args=sc->value;
		s_goto(sc,OP_VECTOR);

		/* ========== printing part ========== */
	case OP_P0LIST:
		if(is_vector(sc->args)) {
			putstr(sc,"#(");
			sc->args=cons(sc,sc->args,mk_integer(sc,0));
			s_goto(sc,OP_PVECFROM);
		} else if(is_environment(sc->args)) {
			putstr(sc,"#<ENVIRONMENT>");
			s_return(sc,sc->T);
		} else if (!is_pair(sc->args)) {
			printatom(sc, sc->args, sc->print_flag);
			s_return(sc,sc->T);
		} else if (car(sc->args) == sc->QUOTE && ok_abbrev(cdr(sc->args))) {
			putstr(sc, "'");
			sc->args = cadr(sc->args);
			s_goto(sc,OP_P0LIST);
		} else if (car(sc->args) == sc->QQUOTE && ok_abbrev(cdr(sc->args))) {
			putstr(sc, "`");
			sc->args = cadr(sc->args);
			s_goto(sc,OP_P0LIST);
		} else if (car(sc->args) == sc->UNQUOTE && ok_abbrev(cdr(sc->args))) {
			putstr(sc, ",");
			sc->args = cadr(sc->args);
			s_goto(sc,OP_P0LIST);
		} else if (car(sc->args) == sc->UNQUOTESP && ok_abbrev(cdr(sc->args))) {
			putstr(sc, ",@");
			sc->args = cadr(sc->args);
			s_goto(sc,OP_P0LIST);
		} else {
			putstr(sc, "(");
			s_save(sc,OP_P1LIST, cdr(sc->args), sc->NIL);
			sc->args = car(sc->args);
			s_goto(sc,OP_P0LIST);
		}

	case OP_P1LIST:
		if (is_pair(sc->args)) {
			s_save(sc,OP_P1LIST, cdr(sc->args), sc->NIL);
			putstr(sc, " ");
			sc->args = car(sc->args);
			s_goto(sc,OP_P0LIST);
		} else if(is_vector(sc->args)) {
			s_save(sc,OP_P1LIST,sc->NIL,sc->NIL);
			putstr(sc, " . ");
			s_goto(sc,OP_P0LIST);
		} else {
			if (sc->args != sc->NIL) {
				putstr(sc, " . ");
				printatom(sc, sc->args, sc->print_flag);
			}
			putstr(sc, ")");
			s_return(sc,sc->T);
		}
	case OP_PVECFROM: {
		int i=ivalue_unchecked(cdr(sc->args));
		cell_ptr_t vec=car(sc->args);
		int len=ivalue_unchecked(vec);
		if(i==len) {
			putstr(sc,")");
			s_return(sc,sc->T);
		} else {
			cell_ptr_t elem=vector_elem(vec,i);
			ivalue_unchecked(cdr(sc->args))=i+1;
			s_save(sc,OP_PVECFROM, sc->args, sc->NIL);
			sc->args=elem;
			if (i > 0)
				putstr(sc," ");
			s_goto(sc,OP_P0LIST);
		}
	}

	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);

	}
	return sc->T;
}

static cell_ptr_t opexe_6(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x, y;
	long v;

	switch (op) {
	case OP_LIST_LENGTH:     /* length */   /* a.k */
		v=list_length(sc,car(sc->args));
		if(v<0) {
			error_1(sc,"length: not a list:",car(sc->args));
		}
		s_return(sc,mk_integer(sc, v));

	case OP_ASSQ:       /* assq */     /* a.k */
		x = car(sc->args);
		for (y = cadr(sc->args); is_pair(y); y = cdr(y)) {
			if (!is_pair(car(y))) {
				error_0(sc,"unable to handle non pair element");
			}
			if (x == caar(y))
				break;
		}
		if (is_pair(y)) {
			s_return(sc,car(y));
		} else {
			s_return(sc,sc->F);
		}


	case OP_GET_CLOSURE:     /* get-closure-code */   /* a.k */
		sc->args = car(sc->args);
		if (sc->args == sc->NIL) {
			s_return(sc,sc->F);
		} else if (is_closure(sc->args)) {
			s_return(sc,cons(sc, sc->LAMBDA, closure_code(sc->value)));
		} else if (is_macro(sc->args)) {
			s_return(sc,cons(sc, sc->LAMBDA, closure_code(sc->value)));
		} else {
			s_return(sc,sc->F);
		}
	case OP_CLOSUREP:        /* closure? */
		/*
		* Note, macro object is also a closure.
		* Therefore, (closure? <#MACRO>) ==> #t
		*/
		s_retbool(is_closure(car(sc->args)));
	case OP_MACROP:          /* macro? */
		s_retbool(is_macro(car(sc->args)));
	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->T; /* NOTREACHED */
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
	for (;;) {
		op_code_info *pcd=dispatch_table+sc->op;
		if (pcd->name!=0) { /* if built-in function, check arguments */
			char msg[STRBUFFSIZE];
			int ok=1;
			int n=list_length(sc,sc->args);

			/* Check number of arguments */
			if(n<pcd->min_arity) {
				ok=0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
				         pcd->name,
				         pcd->min_arity==pcd->max_arity?"":" at least",
				         pcd->min_arity);
			}
			if(ok && n>pcd->max_arity) {
				ok=0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
				         pcd->name,
				         pcd->min_arity==pcd->max_arity?"":" at most",
				         pcd->max_arity);
			}
			if(ok) {
				if(pcd->arg_tests_encoding!=0) {
					int i=0;
					int j;
					const char *t=pcd->arg_tests_encoding;
					cell_ptr_t arglist=sc->args;
					do {
						cell_ptr_t arg=car(arglist);
						j=(int)t[0];
						if(j==TST_LIST[0]) {
							if(arg!=sc->NIL && !is_pair(arg)) break;
						} else {
							if(!tests[j].fct(arg)) break;
						}

						if(t[1]!=0) { /* last test is replicated as necessary */
							t++;
						}
						arglist=cdr(arglist);
						i++;
					} while(i<n);
					if(i<n) {
						ok=0;
						snprintf(msg, STRBUFFSIZE, "%s: argument %d must be: %s",
						         pcd->name,
						         i+1,
						         tests[j].kind);
					}
				}
			}
			if(!ok) {
				if(_error_1(sc,msg,0)==sc->NIL) {
					return;
				}
				pcd=dispatch_table+sc->op;
			}
		}
		ok_to_freely_gc(sc);
		if (pcd->func(sc, (enum scheme_opcodes)sc->op) == sc->NIL) {
			return;
		}
		if(sc->no_memory) {
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

/* Hard-coded for the given keywords. Remember to rewrite if more are added! */
static int syntaxnum(cell_ptr_t p) {
	const char *s=strvalue(car(p));
	switch(strlength(car(p))) {
	case 2:
		if(s[0]=='i') return OP_IF0;        /* if */
		else return OP_OR0;                 /* or */
	case 3:
		if(s[0]=='a') return OP_AND0;      /* and */
		else return OP_LET0;               /* let */
	case 4:
		switch(s[3]) {
		case 'e':
			return OP_CASE0;         /* case */
		case 'd':
			return OP_COND0;         /* cond */
		case '*':
			return OP_LET0AST;       /* let* */
		default:
			return OP_SET0;           /* set! */
		}
	case 5:
		switch(s[2]) {
		case 'g':
			return OP_BEGIN;         /* begin */
		case 'l':
			return OP_DELAY;         /* delay */
		case 'c':
			return OP_MACRO0;        /* macro */
		default:
			return OP_QUOTE;          /* quote */
		}
	case 6:
		switch(s[2]) {
		case 'm':
			return OP_LAMBDA;        /* lambda */
		case 'f':
			return OP_DEF0;          /* define */
		default:
			return OP_LET0REC;        /* letrec */
		}
	default:
		return OP_C0STREAM;                /* cons-stream */
	}
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

	__s_num_zero.is_integer=1;
	__s_num_zero.value.ivalue=0;
	__s_num_one.is_integer=1;
	__s_num_one.value.ivalue=1;

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
	sc->LAMBDA = mk_symbol(sc, "lambda");
	sc->QUOTE = mk_symbol(sc, "quote");
	sc->QQUOTE = mk_symbol(sc, "quasiquote");
	sc->UNQUOTE = mk_symbol(sc, "unquote");
	sc->UNQUOTESP = mk_symbol(sc, "unquote-splicing");
	sc->FEED_TO = mk_symbol(sc, "=>");
	sc->COLON_HOOK = mk_symbol(sc,"*colon-hook*");
	sc->ERROR_HOOK = mk_symbol(sc, "*error-hook*");
	sc->SHARP_HOOK = mk_symbol(sc, "*sharp-hook*");
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
	sc->envir = sc->global_env;
	sc->file_i=0;
	sc->load_stack[0].kind=port_input|port_string;
	sc->load_stack[0].rep.string.start=(char*)cmd; /* This func respects const */
	sc->load_stack[0].rep.string.past_the_end=(char*)cmd+strlen(cmd);
	sc->load_stack[0].rep.string.curr=(char*)cmd;
	sc->loadport=mk_port(sc,sc->load_stack);
	sc->retcode=0;
	sc->interactive_repl=0;
	sc->inport=sc->loadport;
	sc->args = mk_integer(sc,sc->file_i);
	Eval_Cycle(sc, OP_T0LVL);
	typeflag(sc->loadport)=T_ATOM;
	if(sc->retcode==0) {
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
	scheme_t sc;
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
	if(!scheme_init(&sc)) {
		fprintf(stderr,"Could not initialize!\n");
		return 2;
	}
	scheme_set_input_port_file(&sc, stdin);
	scheme_set_output_port_file(&sc, stdout);

	fprintf(stderr, "cell size: %lu\n", sizeof(cell_t));

#if USE_DL
	scheme_define(&sc,sc.global_env,mk_symbol(&sc,"load-extension"),mk_foreign_func(&sc, scm_load_ext));
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
		} else if(strcmp(file_name,"-1")==0 || strcmp(file_name,"-c")==0) {
			cell_ptr_t args=sc.NIL;
			isfile=file_name[1]=='1';
			file_name=*argv++;
			if(strcmp(file_name,"-")==0) {
				fin=stdin;
			} else if(isfile) {
				fin=fopen(file_name,"r");
			}
			for(; *argv; argv++) {
				cell_ptr_t value=mk_string(&sc,*argv);
				args=cons(&sc,value,args);
			}
			args=reverse_in_place(&sc,sc.NIL,args);
			scheme_define(&sc,sc.global_env,mk_symbol(&sc,"*args*"),args);

		} else {
			fin=fopen(file_name,"r");
		}
		if(isfile && fin==0) {
			fprintf(stderr,"Could not open file %s\n",file_name);
		} else {
			if(isfile) {
				scheme_load_named_file(&sc,fin,file_name);
			} else {
				scheme_load_string(&sc,file_name);
			}
			if(!isfile || fin!=stdin) {
				if(sc.retcode!=0) {
					fprintf(stderr,"Errors encountered reading %s\n",file_name);
				}
				if(isfile) {
					fclose(fin);
				}
			}
		}
		file_name=*argv++;
	} while(file_name!=0);
	if(argc==1) {
		scheme_load_named_file(&sc,stdin,0);
	}
	retcode=sc.retcode;
	scheme_deinit(&sc);

	return retcode;
}

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
