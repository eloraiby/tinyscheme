#include "scheme-private.h"

#define cont_dump(p)     cdr(p)

#define is_syntax(p)	(typeflag(p)&T_SYNTAX)


/* Eval should be a single function with jump table optimization, something like:
	static const void*	jt[2]	= { &&l0, &&l1 };
	srand(time(NULL));
	int r = rand();
	fprintf(stderr, "rand: %d\n", r);

	goto *jt[r % 2];

	l0:
		fprintf(stderr, "l0\n");
		goto l2;
	l1:	fprintf(stderr, "l1\n");
		goto l2;
	l2:
		fprintf(stderr, "l2\n");
*/

static int is_list(scheme_t *sc, cell_ptr_t a) {
	return list_length(sc,a) >= 0;
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


/* ========== Evaluation Cycle ========== */

cell_ptr_t op_eval(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t	x, y;
	long	v;

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
		if( is_symbol(sc->code) ) {    /* symbol */
			x=find_slot_in_env(sc,sc->envir,sc->code,1);
			if (x != sc->NIL) {
				s_return(sc,slot_value_in_env(x));
			} else {
				error_1(sc, "eval: unbound variable:", sc->code);
			}
		} else if( is_pair(sc->code) ) {
			if( is_syntax(x = car(sc->code)) ) {     /* SYNTAX */
				sc->code = cdr(sc->code);
				s_goto(sc, syntaxnum(x));
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
		if( is_immutable(car(sc->code)) )
			error_1(sc,"define: unable to alter immutable", car(sc->code));

		if( is_pair(car(sc->code)) ) {
			x = caar(sc->code);
			sc->code = cons(sc, sc->LAMBDA, cons(sc, cdar(sc->code), cdr(sc->code)));
		} else {
			x = car(sc->code);
			sc->code = cadr(sc->code);
		}

		if( !is_symbol(x) ) {
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

	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->T;
}
