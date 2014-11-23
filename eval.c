#include "scheme-private.h"

static int
is_zero_double(double x)
{
	return x < DBL_MIN && x > -DBL_MIN;
}

cell_ptr_t
eval_op(scheme_t *sc,
	enum scheme_opcodes op)
{
	cell_ptr_t	x, y;
	number_t	n;
	long		v;

#if USE_MATH
	double dd;
#endif

	int (*comp_func)(number_t,number_t)=0;

	switch (op) {

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
			sc->regs.args = sc->value;
			s_goto(sc,OP_P0LIST);
		} else {
			s_return(sc,sc->value);
		}

	case OP_EVAL:       /* main part of evaluation */
#if USE_TRACING
		if(sc->tracing) {
			/*s_save(sc,OP_VALUEPRINT,sc->syms.NIL,sc->syms.NIL);*/
			s_save(sc,OP_REAL_EVAL,sc->regs.args, sc->regs.code);
			sc->regs.args = sc->regs.code;
			putstr(sc,"\nEval: ");
			s_goto(sc,OP_P0LIST);
		}
		/* fall through */
	case OP_REAL_EVAL:
#endif
		if (is_symbol(sc->regs.code)) {    /* symbol */
			x=find_slot_in_env(sc,sc->regs.envir,sc->regs.code,1);
			if (x != sc->syms.NIL) {
				s_return(sc,slot_value_in_env(x));
			} else {
				error_1(sc,"eval: unbound variable:", sc->regs.code);
			}
		} else if (is_pair(sc->regs.code)) {
			if (is_syntax(x = car(sc->regs.code))) {     /* SYNTAX */
				sc->regs.code = cdr(sc->regs.code);
				s_goto(sc,syntaxnum(x));
			} else {/* first, eval top element and eval arguments */
				s_save(sc,OP_E0ARGS, sc->syms.NIL, sc->regs.code);
				/* If no macros => s_save(sc,OP_E1ARGS, sc->syms.NIL, cdr(sc->regs.code));*/
				sc->regs.code = car(sc->regs.code);
				s_goto(sc,OP_EVAL);
			}
		} else {
			s_return(sc,sc->regs.code);
		}

	case OP_E0ARGS:     /* eval arguments */
		if (is_macro(sc->value)) {    /* macro expansion */
			s_save(sc,OP_DOMACRO, sc->syms.NIL, sc->syms.NIL);
			sc->regs.args = cons(sc,sc->regs.code, sc->syms.NIL);
			sc->regs.code = sc->value;
			s_goto(sc,OP_APPLY);
		} else {
			sc->regs.code = cdr(sc->regs.code);
			s_goto(sc,OP_E1ARGS);
		}

	case OP_E1ARGS:     /* eval arguments */
		sc->regs.args = cons(sc, sc->value, sc->regs.args);
		if (is_pair(sc->regs.code)) { /* continue */
			s_save(sc,OP_E1ARGS, sc->regs.args, cdr(sc->regs.code));
			sc->regs.code = car(sc->regs.code);
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->regs.args = reverse_in_place(sc, sc->syms.NIL, sc->regs.args);
			sc->regs.code = car(sc->regs.args);
			sc->regs.args = cdr(sc->regs.args);
			s_goto(sc,OP_APPLY);
		}

#if USE_TRACING
	case OP_TRACING: {
		int tr=sc->tracing;
		sc->tracing=ivalue(car(sc->regs.args));
		s_return(sc,mk_integer(sc,tr));
	}
#endif

	case OP_APPLY:      /* apply 'code' to 'args' */
#if USE_TRACING
		if(sc->tracing) {
			s_save(sc,OP_REAL_APPLY,sc->regs.args,sc->regs.code);
			sc->print_flag = 1;
			/*  sc->regs.args=cons(sc,sc->regs.code,sc->regs.args);*/
			putstr(sc,"\nApply to: ");
			s_goto(sc,OP_P0LIST);
		}
		/* fall through */
	case OP_REAL_APPLY:
#endif
		if( is_proc(sc->regs.code) ) {
			s_goto(sc, procnum(sc->regs.code));   /* PROCEDURE */
		} else if( is_foreign(sc->regs.code) ) {
			/* Keep nested calls from GC'ing the arglist */
			push_recent_alloc(sc, sc->regs.args, sc->syms.NIL);
			x = sc->regs.code->object.ff(sc, sc->regs.args);
			s_return(sc,x);
		} else if( is_closure(sc->regs.code) || is_macro(sc->regs.code) || is_promise(sc->regs.code) ) { /* CLOSURE */
			/* Should not accept promise */
			/* make environment */
			new_frame_in_env(sc, closure_env(sc->regs.code));
			for (x = car(closure_code(sc->regs.code)), y = sc->regs.args;
			     is_pair(x); x = cdr(x), y = cdr(y)) {
				if (y == sc->syms.NIL) {
					error_0(sc,"not enough arguments");
				} else {
					new_slot_in_env(sc, car(x), car(y));
				}
			}
			if (x == sc->syms.NIL) {
				/*--
		     * if (y != sc->syms.NIL) {
		     *   error_0(sc,"too many arguments");
		     * }
		     */
			} else if (is_symbol(x))
				new_slot_in_env(sc, x, y);
			else {
				error_1(sc,"syntax error in closure: not a symbol:", x);
			}
			sc->regs.code = cdr(closure_code(sc->regs.code));
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_BEGIN);
		} else if (is_continuation(sc->regs.code)) { /* CONTINUATION */
			sc->regs.dump = cont_dump(sc->regs.code);
			s_return(sc,sc->regs.args != sc->syms.NIL ? car(sc->regs.args) : sc->syms.NIL);
		} else {
			error_0(sc,"illegal function");
		}

	case OP_DOMACRO:    /* do macro */
		sc->regs.code = sc->value;
		s_goto(sc,OP_EVAL);

#if 1
	case OP_LAMBDA:     /* lambda */
	/* If the hook is defined, apply it to sc->regs.code, otherwise
	   set sc->value fall thru */
	{
		cell_ptr_t f=find_slot_in_env(sc,sc->regs.envir,sc->syms.COMPILE_HOOK,1);
		if(f==sc->syms.NIL) {
			sc->value = sc->regs.code;
			/* Fallthru */
		} else {
			s_save(sc,OP_LAMBDA1,sc->regs.args,sc->regs.code);
			sc->regs.args=cons(sc,sc->regs.code,sc->syms.NIL);
			sc->regs.code=slot_value_in_env(f);
			s_goto(sc,OP_APPLY);
		}
	}

	case OP_LAMBDA1:
		s_return(sc,mk_closure(sc, sc->value, sc->regs.envir));

#else
	case OP_LAMBDA:     /* lambda */
		s_return(sc,mk_closure(sc, sc->regs.code, sc->regs.envir));

#endif

	case OP_MKCLOSURE: /* make-closure */
		x=car(sc->regs.args);
		if(car(x)==sc->syms.LAMBDA) {
			x=cdr(x);
		}
		if(cdr(sc->regs.args)==sc->syms.NIL) {
			y=sc->regs.envir;
		} else {
			y=cadr(sc->regs.args);
		}
		s_return(sc,mk_closure(sc, x, y));

	case OP_QUOTE:      /* quote */
		s_return(sc,car(sc->regs.code));

	case OP_DEF0:  /* define */
		if(is_immutable(car(sc->regs.code)))
			error_1(sc,"define: unable to alter immutable", car(sc->regs.code));

		if (is_pair(car(sc->regs.code))) {
			x = caar(sc->regs.code);
			sc->regs.code = cons(sc, sc->syms.LAMBDA, cons(sc, cdar(sc->regs.code), cdr(sc->regs.code)));
		} else {
			x = car(sc->regs.code);
			sc->regs.code = cadr(sc->regs.code);
		}
		if (!is_symbol(x)) {
			error_0(sc,"variable is not a symbol");
		}
		s_save(sc,OP_DEF1, sc->syms.NIL, x);
		s_goto(sc,OP_EVAL);

	case OP_DEF1:  /* define */
		x=find_slot_in_env(sc,sc->regs.envir,sc->regs.code,0);
		if (x != sc->syms.NIL) {
			set_slot_in_env(sc, x, sc->value);
		} else {
			new_slot_in_env(sc, sc->regs.code, sc->value);
		}
		s_return(sc,sc->regs.code);


	case OP_DEFP:  /* defined? */
		x=sc->regs.envir;
		if(cdr(sc->regs.args)!=sc->syms.NIL) {
			x=cadr(sc->regs.args);
		}
		s_retbool(find_slot_in_env(sc,x,car(sc->regs.args),1)!=sc->syms.NIL);

	case OP_SET0:       /* set! */
		if(is_immutable(car(sc->regs.code)))
			error_1(sc,"set!: unable to alter immutable variable",car(sc->regs.code));
		s_save(sc,OP_SET1, sc->syms.NIL, car(sc->regs.code));
		sc->regs.code = cadr(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_SET1:       /* set! */
		y=find_slot_in_env(sc,sc->regs.envir,sc->regs.code,1);
		if (y != sc->syms.NIL) {
			set_slot_in_env(sc, y, sc->value);
			s_return(sc,sc->value);
		} else {
			error_1(sc,"set!: unbound variable:", sc->regs.code);
		}


	case OP_BEGIN:      /* begin */
		if (!is_pair(sc->regs.code)) {
			s_return(sc,sc->regs.code);
		}
		if (cdr(sc->regs.code) != sc->syms.NIL) {
			s_save(sc,OP_BEGIN, sc->syms.NIL, cdr(sc->regs.code));
		}
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_IF0:        /* if */
		s_save(sc,OP_IF1, sc->syms.NIL, cdr(sc->regs.code));
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_IF1:        /* if */
		if (is_true(sc->value))
			sc->regs.code = car(sc->regs.code);
		else
			sc->regs.code = cadr(sc->regs.code);  /* (if #f 1) ==> () because
					    * car(sc->syms.NIL) = sc->syms.NIL */
		s_goto(sc,OP_EVAL);

	case OP_LET0:       /* let */
		sc->regs.args = sc->syms.NIL;
		sc->value = sc->regs.code;
		sc->regs.code = is_symbol(car(sc->regs.code)) ? cadr(sc->regs.code) : car(sc->regs.code);
		s_goto(sc,OP_LET1);

	case OP_LET1:       /* let (calculate parameters) */
		sc->regs.args = cons(sc, sc->value, sc->regs.args);
		if (is_pair(sc->regs.code)) { /* continue */
			if (!is_pair(car(sc->regs.code)) || !is_pair(cdar(sc->regs.code))) {
				error_1(sc, "Bad syntax of binding spec in let :",
					car(sc->regs.code));
			}
			s_save(sc,OP_LET1, sc->regs.args, cdr(sc->regs.code));
			sc->regs.code = cadar(sc->regs.code);
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->regs.args = reverse_in_place(sc, sc->syms.NIL, sc->regs.args);
			sc->regs.code = car(sc->regs.args);
			sc->regs.args = cdr(sc->regs.args);
			s_goto(sc,OP_LET2);
		}

	case OP_LET2:       /* let */
		new_frame_in_env(sc, sc->regs.envir);
		for (x = is_symbol(car(sc->regs.code)) ? cadr(sc->regs.code) : car(sc->regs.code), y = sc->regs.args;
		     y != sc->syms.NIL; x = cdr(x), y = cdr(y)) {
			new_slot_in_env(sc, caar(x), car(y));
		}
		if (is_symbol(car(sc->regs.code))) {    /* named let */
			for (x = cadr(sc->regs.code), sc->regs.args = sc->syms.NIL; x != sc->syms.NIL; x = cdr(x)) {
				if (!is_pair(x))
					error_1(sc, "Bad syntax of binding in let :", x);
				if (!is_list(sc, car(x)))
					error_1(sc, "Bad syntax of binding in let :", car(x));
				sc->regs.args = cons(sc, caar(x), sc->regs.args);
			}
			x = mk_closure(sc, cons(sc, reverse_in_place(sc, sc->syms.NIL, sc->regs.args), cddr(sc->regs.code)), sc->regs.envir);
			new_slot_in_env(sc, car(sc->regs.code), x);
			sc->regs.code = cddr(sc->regs.code);
			sc->regs.args = sc->syms.NIL;
		} else {
			sc->regs.code = cdr(sc->regs.code);
			sc->regs.args = sc->syms.NIL;
		}
		s_goto(sc,OP_BEGIN);

	case OP_LET0AST:    /* let* */
		if (car(sc->regs.code) == sc->syms.NIL) {
			new_frame_in_env(sc, sc->regs.envir);
			sc->regs.code = cdr(sc->regs.code);
			s_goto(sc,OP_BEGIN);
		}
		if(!is_pair(car(sc->regs.code)) || !is_pair(caar(sc->regs.code)) || !is_pair(cdaar(sc->regs.code))) {
			error_1(sc,"Bad syntax of binding spec in let* :",car(sc->regs.code));
		}
		s_save(sc,OP_LET1AST, cdr(sc->regs.code), car(sc->regs.code));
		sc->regs.code = cadaar(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_LET1AST:    /* let* (make new frame) */
		new_frame_in_env(sc, sc->regs.envir);
		s_goto(sc,OP_LET2AST);

	case OP_LET2AST:    /* let* (calculate parameters) */
		new_slot_in_env(sc, caar(sc->regs.code), sc->value);
		sc->regs.code = cdr(sc->regs.code);
		if (is_pair(sc->regs.code)) { /* continue */
			s_save(sc,OP_LET2AST, sc->regs.args, sc->regs.code);
			sc->regs.code = cadar(sc->regs.code);
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->regs.code = sc->regs.args;
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_BEGIN);
		}

	/* =================== binding operations ==================*/
	case OP_LET0REC:    /* letrec */
		new_frame_in_env(sc, sc->regs.envir);
		sc->regs.args = sc->syms.NIL;
		sc->value = sc->regs.code;
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_LET1REC);

	case OP_LET1REC:    /* letrec (calculate parameters) */
		sc->regs.args = cons(sc, sc->value, sc->regs.args);
		if (is_pair(sc->regs.code)) { /* continue */
			if (!is_pair(car(sc->regs.code)) || !is_pair(cdar(sc->regs.code))) {
				error_1(sc, "Bad syntax of binding spec in letrec :",
					car(sc->regs.code));
			}
			s_save(sc,OP_LET1REC, sc->regs.args, cdr(sc->regs.code));
			sc->regs.code = cadar(sc->regs.code);
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_EVAL);
		} else {  /* end */
			sc->regs.args = reverse_in_place(sc, sc->syms.NIL, sc->regs.args);
			sc->regs.code = car(sc->regs.args);
			sc->regs.args = cdr(sc->regs.args);
			s_goto(sc,OP_LET2REC);
		}

	case OP_LET2REC:    /* letrec */
		for (x = car(sc->regs.code), y = sc->regs.args; y != sc->syms.NIL; x = cdr(x), y = cdr(y)) {
			new_slot_in_env(sc, caar(x), car(y));
		}
		sc->regs.code = cdr(sc->regs.code);
		sc->regs.args = sc->syms.NIL;
		s_goto(sc,OP_BEGIN);

	case OP_COND0:      /* cond */
		if (!is_pair(sc->regs.code)) {
			error_0(sc,"syntax error in cond");
		}
		s_save(sc,OP_COND1, sc->syms.NIL, sc->regs.code);
		sc->regs.code = caar(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_COND1:      /* cond */
		if (is_true(sc->value)) {
			if ((sc->regs.code = cdar(sc->regs.code)) == sc->syms.NIL) {
				s_return(sc,sc->value);
			}
			if(car(sc->regs.code)==sc->syms.FEED_TO) {
				if(!is_pair(cdr(sc->regs.code))) {
					error_0(sc,"syntax error in cond");
				}
				x=cons(sc, sc->syms.QUOTE, cons(sc, sc->value, sc->syms.NIL));
				sc->regs.code=cons(sc,cadr(sc->regs.code),cons(sc,x,sc->syms.NIL));
				s_goto(sc,OP_EVAL);
			}
			s_goto(sc,OP_BEGIN);
		} else {
			if ((sc->regs.code = cdr(sc->regs.code)) == sc->syms.NIL) {
				s_return(sc,sc->syms.NIL);
			} else {
				s_save(sc,OP_COND1, sc->syms.NIL, sc->regs.code);
				sc->regs.code = caar(sc->regs.code);
				s_goto(sc,OP_EVAL);
			}
		}

	case OP_DELAY:      /* delay */
		x = mk_closure(sc, cons(sc, sc->syms.NIL, sc->regs.code), sc->regs.envir);
		typeflag(x)=T_PROMISE;
		s_return(sc,x);

	case OP_AND0:       /* and */
		if (sc->regs.code == sc->syms.NIL) {
			s_return(sc,sc->syms.T);
		}
		s_save(sc,OP_AND1, sc->syms.NIL, cdr(sc->regs.code));
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_AND1:       /* and */
		if (is_false(sc->value)) {
			s_return(sc,sc->value);
		} else if (sc->regs.code == sc->syms.NIL) {
			s_return(sc,sc->value);
		} else {
			s_save(sc,OP_AND1, sc->syms.NIL, cdr(sc->regs.code));
			sc->regs.code = car(sc->regs.code);
			s_goto(sc,OP_EVAL);
		}

	case OP_OR0:        /* or */
		if (sc->regs.code == sc->syms.NIL) {
			s_return(sc,sc->syms.F);
		}
		s_save(sc,OP_OR1, sc->syms.NIL, cdr(sc->regs.code));
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_OR1:        /* or */
		if (is_true(sc->value)) {
			s_return(sc,sc->value);
		} else if (sc->regs.code == sc->syms.NIL) {
			s_return(sc,sc->value);
		} else {
			s_save(sc,OP_OR1, sc->syms.NIL, cdr(sc->regs.code));
			sc->regs.code = car(sc->regs.code);
			s_goto(sc,OP_EVAL);
		}

	case OP_C0STREAM:   /* cons-stream */
		s_save(sc,OP_C1STREAM, sc->syms.NIL, cdr(sc->regs.code));
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_C1STREAM:   /* cons-stream */
		sc->regs.args = sc->value;  /* save sc->value to register sc->regs.args for gc */
		x = mk_closure(sc, cons(sc, sc->syms.NIL, sc->regs.code), sc->regs.envir);
		typeflag(x)=T_PROMISE;
		s_return(sc,cons(sc, sc->regs.args, x));

	case OP_MACRO0:     /* macro */
		if (is_pair(car(sc->regs.code))) {
			x = caar(sc->regs.code);
			sc->regs.code = cons(sc, sc->syms.LAMBDA, cons(sc, cdar(sc->regs.code), cdr(sc->regs.code)));
		} else {
			x = car(sc->regs.code);
			sc->regs.code = cadr(sc->regs.code);
		}
		if (!is_symbol(x)) {
			error_0(sc,"variable is not a symbol");
		}
		s_save(sc,OP_MACRO1, sc->syms.NIL, x);
		s_goto(sc,OP_EVAL);

	case OP_MACRO1:     /* macro */
		typeflag( sc->value ) = T_MACRO;
		x = find_slot_in_env(sc, sc->regs.envir, sc->regs.code, 0);
		if( x != sc->syms.NIL ) {
			set_slot_in_env(sc, x, sc->value);
		} else {
			new_slot_in_env(sc, sc->regs.code, sc->value);
		}
		s_return(sc,sc->regs.code);

	case OP_CASE0:      /* case */
		s_save(sc,OP_CASE1, sc->syms.NIL, cdr(sc->regs.code));
		sc->regs.code = car(sc->regs.code);
		s_goto(sc,OP_EVAL);

	case OP_CASE1:      /* case */
		for( x = sc->regs.code; x != sc->syms.NIL; x = cdr(x) ) {
			if (!is_pair(y = caar(x))) {
				break;
			}

			for( ; y != sc->syms.NIL; y = cdr(y) ) {
				if (eqv(car(y), sc->value)) {
					break;
				}
			}

			if( y != sc->syms.NIL ) {
				break;
			}
		}
		if (x != sc->syms.NIL) {
			if( is_pair(caar(x)) ) {
				sc->regs.code = cdar(x);
				s_goto(sc,OP_BEGIN);
			} else {/* else */
				s_save(sc,OP_CASE2, sc->syms.NIL, cdar(x));
				sc->regs.code = caar(x);
				s_goto(sc,OP_EVAL);
			}
		} else {
			s_return(sc,sc->syms.NIL);
		}

	case OP_CASE2:      /* case */
		if (is_true(sc->value)) {
			s_goto(sc,OP_BEGIN);
		} else {
			s_return(sc,sc->syms.NIL);
		}

	case OP_PAPPLY:     /* apply */
		sc->regs.code = car(sc->regs.args);
		sc->regs.args = list_star(sc,cdr(sc->regs.args));
		/*sc->regs.args = cadr(sc->regs.args);*/
		s_goto(sc,OP_APPLY);

	case OP_PEVAL: /* eval */
		if(cdr(sc->regs.args)!=sc->syms.NIL) {
			sc->regs.envir=cadr(sc->regs.args);
		}
		sc->regs.code = car(sc->regs.args);
		s_goto(sc,OP_EVAL);

	case OP_CONTINUATION:    /* call-with-current-continuation */
		sc->regs.code = car(sc->regs.args);
		sc->regs.args = cons(sc, mk_continuation(sc, sc->regs.dump), sc->syms.NIL);
		s_goto(sc,OP_APPLY);

	/* ============== Math starts here ====================== */
#if USE_MATH
	case OP_INEX2EX:    /* inexact->exact */
		x=car(sc->regs.args);
		if(num_is_integer(x)) {
			s_return(sc,x);
		} else if(modf(rvalue_unchecked(x),&dd)==0.0) {
			s_return(sc,mk_integer(sc,ivalue(x)));
		} else {
			error_1(sc,"inexact->exact: not integral:",x);
		}

	case OP_EXP:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, exp(rvalue(x))));

	case OP_LOG:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, log(rvalue(x))));

	case OP_SIN:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, sin(rvalue(x))));

	case OP_COS:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, cos(rvalue(x))));

	case OP_TAN:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, tan(rvalue(x))));

	case OP_ASIN:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, asin(rvalue(x))));

	case OP_ACOS:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, acos(rvalue(x))));

	case OP_ATAN:
		x=car(sc->regs.args);
		if(cdr(sc->regs.args)==sc->syms.NIL) {
			s_return(sc, mk_real(sc, atan(rvalue(x))));
		} else {
			cell_ptr_t y=cadr(sc->regs.args);
			s_return(sc, mk_real(sc, atan2(rvalue(x),rvalue(y))));
		}

	case OP_SQRT:
		x=car(sc->regs.args);
		s_return(sc, mk_real(sc, sqrt(rvalue(x))));

	case OP_EXPT: {
		double result;
		int real_result=1;
		cell_ptr_t y=cadr(sc->regs.args);
		x=car(sc->regs.args);
		if (num_is_integer(x) && num_is_integer(y))
			real_result=0;
		/* This 'if' is an R5RS compatibility fix. */
		/* NOTE: Remove this 'if' fix for R6RS.    */
		if (rvalue(x) == 0 && rvalue(y) < 0) {
			result = 0.0;
		} else {
			result = pow(rvalue(x),rvalue(y));
		}
		/* Before returning integer result make sure we can. */
		/* If the test fails, result is too big for integer. */
		if (!real_result)
		{
			long result_as_long = (long)result;
			if (result != (double)result_as_long)
				real_result = 1;
		}
		if (real_result) {
			s_return(sc, mk_real(sc, result));
		} else {
			s_return(sc, mk_integer(sc, result));
		}
	}

	case OP_FLOOR:
		x = car(sc->regs.args);
		s_return(sc, mk_real(sc, floor(rvalue(x))));

	case OP_CEILING:
		x = car(sc->regs.args);
		s_return(sc, mk_real(sc, ceil(rvalue(x))));

	case OP_TRUNCATE : {
		double rvalue_of_x ;
		x = car(sc->regs.args);
		rvalue_of_x = rvalue(x) ;
		if (rvalue_of_x > 0) {
			s_return(sc, mk_real(sc, floor(rvalue_of_x)));
		} else {
			s_return(sc, mk_real(sc, ceil(rvalue_of_x)));
		}
	}

	case OP_ROUND:
		x = car(sc->regs.args);
		if (num_is_integer(x))
			s_return(sc, x);
		s_return(sc, mk_real(sc, round_per_R5RS(rvalue(x))));
#endif

	case OP_ADD:        /* + */
		n = sc->num_zero;
		for( x = sc->regs.args; x != sc->syms.NIL; x = cdr(x) ) {
			n = num_add(n, nvalue(car(x)));
		}
		s_return(sc, mk_number(sc, n));

	case OP_MUL:        /* * */
		n = sc->num_one;
		for (x = sc->regs.args; x != sc->syms.NIL; x = cdr(x)) {
			n = num_mul(n, nvalue(car(x)));
		}
		s_return(sc, mk_number(sc, n));

	case OP_SUB:        /* - */
		if( cdr(sc->regs.args) == sc->syms.NIL ) {
			x = sc->regs.args;
			n = sc->num_zero;
		} else {
			x = cdr(sc->regs.args);
			n = nvalue(car(sc->regs.args));
		}
		for (; x != sc->syms.NIL; x = cdr(x)) {
			n = num_sub(n, nvalue(car(x)));
		}
		s_return(sc, mk_number(sc, n));

	case OP_DIV:        /* / */
		if(cdr(sc->regs.args) == sc->syms.NIL) {
			x = sc->regs.args;
			n = sc->num_one;
		} else {
			x = cdr(sc->regs.args);
			n = nvalue(car(sc->regs.args));
		}
		for (; x != sc->syms.NIL; x = cdr(x)) {
			if (!is_zero_double(rvalue(car(x))))
				n = num_div(n, nvalue(car(x)));
			else {
				error_0(sc,"/: division by zero");
			}
		}
		s_return(sc, mk_number(sc, n));

	case OP_INTDIV:        /* quotient */
		if(cdr(sc->regs.args)==sc->syms.NIL) {
			x = sc->regs.args;
			n = sc->num_one;
		} else {
			x = cdr(sc->regs.args);
			n = nvalue(car(sc->regs.args));
		}
		for (; x != sc->syms.NIL; x = cdr(x)) {
			if (ivalue(car(x)) != 0)
				n = num_intdiv(n, nvalue(car(x)));
			else {
				error_0(sc,"quotient: division by zero");
			}
		}
		s_return(sc, mk_number(sc, n));

	case OP_REM:        /* remainder */
		n = nvalue(car(sc->regs.args));
		if (ivalue(cadr(sc->regs.args)) != 0)
			n = num_rem(n, nvalue(cadr(sc->regs.args)));
		else {
			error_0(sc,"remainder: division by zero");
		}
		s_return(sc, mk_number(sc, n));

	case OP_MOD:        /* modulo */
		n = nvalue(car(sc->regs.args));
		if (ivalue(cadr(sc->regs.args)) != 0)
			n = num_mod(n,nvalue(cadr(sc->regs.args)));
		else {
			error_0(sc,"modulo: division by zero");
		}
		s_return(sc, mk_number(sc, n));

	case OP_CAR:        /* car */
		s_return(sc,caar(sc->regs.args));

	case OP_CDR:        /* cdr */
		s_return(sc,cdar(sc->regs.args));

	case OP_CONS:       /* cons */
		cdr(sc->regs.args) = cadr(sc->regs.args);
		s_return(sc,sc->regs.args);

	case OP_SETCAR:     /* set-car! */
		if(!is_immutable(car(sc->regs.args))) {
			caar(sc->regs.args) = cadr(sc->regs.args);
			s_return(sc,car(sc->regs.args));
		} else {
			error_0(sc,"set-car!: unable to alter immutable pair");
		}

	case OP_SETCDR:     /* set-cdr! */
		if(!is_immutable(car(sc->regs.args))) {
			cdar(sc->regs.args) = cadr(sc->regs.args);
			s_return(sc,car(sc->regs.args));
		} else {
			error_0(sc,"set-cdr!: unable to alter immutable pair");
		}

	case OP_CHAR2INT: { /* char->integer */
		char c;
		c=(char)ivalue(car(sc->regs.args));
		s_return(sc,mk_integer(sc,(unsigned char)c));
	}

	case OP_INT2CHAR: { /* integer->char */
		unsigned char c;
		c=(unsigned char)ivalue(car(sc->regs.args));
		s_return(sc,mk_character(sc,(char)c));
	}

	case OP_CHARUPCASE: {
		unsigned char c;
		c=(unsigned char)ivalue(car(sc->regs.args));
		c=toupper(c);
		s_return(sc,mk_character(sc,(char)c));
	}

	case OP_CHARDNCASE: {
		unsigned char c;
		c=(unsigned char)ivalue(car(sc->regs.args));
		c=tolower(c);
		s_return(sc,mk_character(sc,(char)c));
	}

	case OP_STR2SYM:  /* string->symbol */
		s_return(sc,mk_symbol(sc,strvalue(car(sc->regs.args))));

	case OP_STR2ATOM: /* string->atom */ {
		char *s=strvalue(car(sc->regs.args));
		long pf = 0;
		if(cdr(sc->regs.args)!=sc->syms.NIL) {
			/* we know cadr(sc->regs.args) is a natural number */
			/* see if it is 2, 8, 10, or 16, or error */
			pf = ivalue_unchecked(cadr(sc->regs.args));
			if(pf == 16 || pf == 10 || pf == 8 || pf == 2) {
				/* base is OK */
			}
			else {
				pf = -1;
			}
		}
		if (pf < 0) {
			error_1(sc, "string->atom: bad base:", cadr(sc->regs.args));
		} else if(*s=='#') /* no use of base! */ {
			s_return(sc, mk_sharp_const(sc, s+1));
		} else {
			if (pf == 0 || pf == 10) {
				s_return(sc, mk_atom(sc, s));
			}
			else {
				char *ep;
				long iv = strtol(s,&ep,(int )pf);
				if (*ep == 0) {
					s_return(sc, mk_integer(sc, iv));
				}
				else {
					s_return(sc, sc->syms.F);
				}
			}
		}
	}

	case OP_SYM2STR: /* symbol->string */
		x=mk_string(sc,symname(car(sc->regs.args)));
		setimmutable(x);
		s_return(sc,x);

	case OP_ATOM2STR: /* atom->string */ {
		long pf = 0;
		x=car(sc->regs.args);
		if(cdr(sc->regs.args)!=sc->syms.NIL) {
			/* we know cadr(sc->regs.args) is a natural number */
			/* see if it is 2, 8, 10, or 16, or error */
			pf = ivalue_unchecked(cadr(sc->regs.args));
			if(is_number(x) && (pf == 16 || pf == 10 || pf == 8 || pf == 2)) {
				/* base is OK */
			}
			else {
				pf = -1;
			}
		}
		if (pf < 0) {
			error_1(sc, "atom->string: bad base:", cadr(sc->regs.args));
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

		len=ivalue(car(sc->regs.args));

		if(cdr(sc->regs.args)!=sc->syms.NIL) {
			fill=charvalue(cadr(sc->regs.args));
		}
		s_return(sc,mk_empty_string(sc,len,(char)fill));
	}

	case OP_STRLEN:  /* string-length */
		s_return(sc,mk_integer(sc,strlength(car(sc->regs.args))));

	case OP_STRREF: { /* string-ref */
		char*		str;
		unsigned int	index;

		str	= strvalue(car(sc->regs.args));

		index	= (unsigned int)ivalue(cadr(sc->regs.args));

		if( index >= strlength(car(sc->regs.args)) ) {
			error_1(sc,"string-ref: out of bounds:", cadr(sc->regs.args));
		}

		s_return(sc, mk_character(sc, ((unsigned char*)str)[index]));
	}

	case OP_STRSET: { /* string-set! */
		char*		str;
		unsigned int	index;
		int		c;

		if( is_immutable(car(sc->regs.args)) ) {
			error_1(sc,"string-set!: unable to alter immutable string:",car(sc->regs.args));
		}

		str	= strvalue(car(sc->regs.args));

		index	= (unsigned int)ivalue(cadr(sc->regs.args));
		if( index >= strlength(car(sc->regs.args)) ) {
			error_1(sc,"string-set!: out of bounds:",cadr(sc->regs.args));
		}

		c	= charvalue(caddr(sc->regs.args));

		str[index]	= (char)c;
		s_return(sc, car(sc->regs.args));
	}

	case OP_STRAPPEND: { /* string-append */
		/* in 1.29 string-append was in Scheme in init.scm but was too slow */
		int len = 0;
		cell_ptr_t newstr;
		char *pos;

		/* compute needed length for new string */
		for (x = sc->regs.args; x != sc->syms.NIL; x = cdr(x)) {
			len += strlength(car(x));
		}
		newstr = mk_empty_string(sc, len, ' ');
		/* store the contents of the argument strings into the new string */
		for (pos = strvalue(newstr), x = sc->regs.args; x != sc->syms.NIL;
		     pos += strlength(car(x)), x = cdr(x)) {
			memcpy(pos, strvalue(car(x)), strlength(car(x)));
		}
		s_return(sc, newstr);
	}

	case OP_SUBSTR: { /* substring */
		char*		str;
		unsigned int	index0;
		unsigned int	index1;
		unsigned int	len;

		str	= strvalue(car(sc->regs.args));

		index0	= (unsigned int)ivalue(cadr(sc->regs.args));

		if( index0 > strlength(car(sc->regs.args))) {
			error_1(sc,"substring: start out of bounds:",cadr(sc->regs.args));
		}

		if( cddr(sc->regs.args) != sc->syms.NIL) {
			index1	= (unsigned int)ivalue(caddr(sc->regs.args));
			if( index1 > strlength(car(sc->regs.args)) || index1 < index0 ) {
				error_1(sc, "substring: end out of bounds:", caddr(sc->regs.args));
			}
		} else {
			index1 = strlength(car(sc->regs.args));
		}

		len	= index1 - index0;
		x	= mk_empty_string(sc, len, ' ');
		memcpy(strvalue(x), str + index0, len);
		strvalue(x)[len]	= 0;

		s_return(sc,x);
	}

	case OP_VECTOR: {   /* vector */
		int		i;
		cell_ptr_t	vec;
		int		len	= list_length(sc, sc->regs.args);
		if( len < 0 ) {
			error_1(sc, "vector: not a proper list:", sc->regs.args);
		}
		vec	= mk_vector(sc, len);
		if( sc->memory.no_memory ) { s_return(sc, sc->syms.sink); }
		for( x = sc->regs.args, i = 0; is_pair(x); x = cdr(x), i++ ) {
			set_vector_elem(vec, i, car(x));
		}
		s_return(sc,vec);
	}

	case OP_MKVECTOR: { /* make-vector */
		cell_ptr_t	fill = sc->syms.NIL;
		int		len;
		cell_ptr_t	vec;

		len	= ivalue(car(sc->regs.args));

		if( cdr(sc->regs.args) != sc->syms.NIL ) {
			fill	= cadr(sc->regs.args);
		}

		vec	= mk_vector(sc, len);
		if( sc->memory.no_memory ) { s_return(sc, sc->syms.sink); }
		if( fill != sc->syms.NIL ) {
			fill_vector(vec, fill);
		}
		s_return(sc, vec);
	}

	case OP_VECLEN:  /* vector-length */
		s_return(sc,mk_integer(sc,ivalue(car(sc->regs.args))));

	case OP_VECREF: { /* vector-ref */
		int index;

		index=ivalue(cadr(sc->regs.args));

		if(index>=ivalue(car(sc->regs.args))) {
			error_1(sc,"vector-ref: out of bounds:",cadr(sc->regs.args));
		}

		s_return(sc,vector_elem(car(sc->regs.args),index));
	}

	case OP_VECSET: {   /* vector-set! */
		int index;

		if(is_immutable(car(sc->regs.args))) {
			error_1(sc,"vector-set!: unable to alter immutable vector:",car(sc->regs.args));
		}

		index=ivalue(cadr(sc->regs.args));
		if(index>=ivalue(car(sc->regs.args))) {
			error_1(sc,"vector-set!: out of bounds:",cadr(sc->regs.args));
		}

		set_vector_elem(car(sc->regs.args),index,caddr(sc->regs.args));
		s_return(sc,car(sc->regs.args));
	}

	/* ============= Relations start here ==================== */
	case OP_NOT:        /* not */
		s_retbool(is_false(car(sc->regs.args)));
	case OP_BOOLP:       /* boolean? */
		s_retbool(car(sc->regs.args) == sc->syms.F || car(sc->regs.args) == sc->syms.T);
	case OP_EOFOBJP:       /* boolean? */
		s_retbool(car(sc->regs.args) == sc->syms.EOF_OBJ);
	case OP_NULLP:       /* null? */
		s_retbool(car(sc->regs.args) == sc->syms.NIL);
	case OP_NUMEQ:      /* = */
	case OP_LESS:       /* < */
	case OP_GRE:        /* > */
	case OP_LEQ:        /* <= */
	case OP_GEQ:        /* >= */
		switch(op) {
		case OP_NUMEQ: comp_func=num_eq; break;
		case OP_LESS:  comp_func=num_lt; break;
		case OP_GRE:   comp_func=num_gt; break;
		case OP_LEQ:   comp_func=num_le; break;
		case OP_GEQ:   comp_func=num_ge; break;
		default:
			fprintf(stderr, "FATAL ERROR: should never have operation %u reaching this far\n", (unsigned int)op);
			exit(1);
		}
		x=sc->regs.args;
		n=nvalue(car(x));
		x=cdr(x);

		for (; x != sc->syms.NIL; x = cdr(x)) {
			if(!comp_func(n,nvalue(car(x)))) {
				s_retbool(0);
			}
			n=nvalue(car(x));
		}
		s_retbool(1);
	case OP_SYMBOLP:     /* symbol? */
		s_retbool(is_symbol(car(sc->regs.args)));
	case OP_NUMBERP:     /* number? */
		s_retbool(is_number(car(sc->regs.args)));
	case OP_STRINGP:     /* string? */
		s_retbool(is_string(car(sc->regs.args)));
	case OP_INTEGERP:     /* integer? */
		s_retbool(is_integer(car(sc->regs.args)));
	case OP_REALP:     /* real? */
		s_retbool(is_number(car(sc->regs.args))); /* All numbers are real */
	case OP_CHARP:     /* char? */
		s_retbool(is_character(car(sc->regs.args)));
#if USE_CHAR_CLASSIFIERS
	case OP_CHARAP:     /* char-alphabetic? */
		s_retbool(Cisalpha(ivalue(car(sc->regs.args))));
	case OP_CHARNP:     /* char-numeric? */
		s_retbool(Cisdigit(ivalue(car(sc->regs.args))));
	case OP_CHARWP:     /* char-whitespace? */
		s_retbool(Cisspace(ivalue(car(sc->regs.args))));
	case OP_CHARUP:     /* char-upper-case? */
		s_retbool(Cisupper(ivalue(car(sc->regs.args))));
	case OP_CHARLP:     /* char-lower-case? */
		s_retbool(Cislower(ivalue(car(sc->regs.args))));
#endif
	case OP_PORTP:     /* port? */
		s_retbool(is_port(car(sc->regs.args)));
	case OP_INPORTP:     /* input-port? */
		s_retbool(is_inport(car(sc->regs.args)));
	case OP_OUTPORTP:     /* output-port? */
		s_retbool(is_outport(car(sc->regs.args)));
	case OP_PROCP:       /* procedure? */
		/*--
	      * continuation should be procedure by the example
	      * (call-with-current-continuation procedure?) ==> #t
		 * in R^3 report sec. 6.9
	      */
		s_retbool(is_proc(car(sc->regs.args)) || is_closure(car(sc->regs.args))
			  || is_continuation(car(sc->regs.args)) || is_foreign(car(sc->regs.args)));
	case OP_PAIRP:       /* pair? */
		s_retbool(is_pair(car(sc->regs.args)));
	case OP_LISTP:       /* list? */
		s_retbool(list_length(sc,car(sc->regs.args)) >= 0);

	case OP_ENVP:        /* environment? */
		s_retbool(is_environment(car(sc->regs.args)));
	case OP_VECTORP:     /* vector? */
		s_retbool(is_vector(car(sc->regs.args)));
	case OP_EQ:         /* eq? */
		s_retbool(car(sc->regs.args) == cadr(sc->regs.args));
	case OP_EQV:        /* eqv? */
		s_retbool(eqv(car(sc->regs.args), cadr(sc->regs.args)));

	case OP_FORCE:      /* force */
		sc->regs.code = car(sc->regs.args);
		if (is_promise(sc->regs.code)) {
			/* Should change type to closure here */
			s_save(sc, OP_SAVE_FORCED, sc->syms.NIL, sc->regs.code);
			sc->regs.args = sc->syms.NIL;
			s_goto(sc,OP_APPLY);
		} else {
			s_return(sc,sc->regs.code);
		}

	case OP_SAVE_FORCED:     /* Save forced value replacing promise */
		memcpy(sc->regs.code,sc->value,sizeof(cell_t));
		s_return(sc,sc->value);

	case OP_WRITE:      /* write */
	case OP_DISPLAY:    /* display */
	case OP_WRITE_CHAR: /* write-char */
		if(is_pair(cdr(sc->regs.args))) {
			if(cadr(sc->regs.args)!=sc->outport) {
				x=cons(sc,sc->outport,sc->syms.NIL);
				s_save(sc,OP_SET_OUTPORT, x, sc->syms.NIL);
				sc->outport=cadr(sc->regs.args);
			}
		}
		sc->regs.args = car(sc->regs.args);
		if(op==OP_WRITE) {
			sc->print_flag = 1;
		} else {
			sc->print_flag = 0;
		}
		s_goto(sc,OP_P0LIST);

	case OP_NEWLINE:    /* newline */
		if(is_pair(sc->regs.args)) {
			if(car(sc->regs.args)!=sc->outport) {
				x=cons(sc,sc->outport,sc->syms.NIL);
				s_save(sc,OP_SET_OUTPORT, x, sc->syms.NIL);
				sc->outport=car(sc->regs.args);
			}
		}
		putstr(sc, "\n");
		s_return(sc,sc->syms.T);

	case OP_ERR0:  /* error */
		sc->retcode=-1;
		if (!is_string(car(sc->regs.args))) {
			sc->regs.args=cons(sc,mk_string(sc," -- "),sc->regs.args);
			setimmutable(car(sc->regs.args));
		}
		putstr(sc, "Error: ");
		putstr(sc, strvalue(car(sc->regs.args)));
		sc->regs.args = cdr(sc->regs.args);
		s_goto(sc,OP_ERR1);

	case OP_ERR1:  /* error */
		putstr(sc, " ");
		if (sc->regs.args != sc->syms.NIL) {
			s_save(sc,OP_ERR1, cdr(sc->regs.args), sc->syms.NIL);
			sc->regs.args = car(sc->regs.args);
			sc->print_flag = 1;
			s_goto(sc,OP_P0LIST);
		} else {
			putstr(sc, "\n");
			if(sc->interactive_repl) {
				s_goto(sc,OP_T0LVL);
			} else {
				return sc->syms.NIL;
			}
		}

	case OP_REVERSE:   /* reverse */
		s_return(sc,reverse(sc, car(sc->regs.args)));

	case OP_LIST_STAR: /* list* */
		s_return(sc,list_star(sc,sc->regs.args));

	case OP_APPEND:    /* append */
		x = sc->syms.NIL;
		y = sc->regs.args;
		if (y == x) {
			s_return(sc, x);
		}

		/* cdr() in the while condition is not a typo. If car() */
		/* is used (append '() 'a) will return the wrong result.*/
		while (cdr(y) != sc->syms.NIL) {
			x = revappend(sc, x, car(y));
			y = cdr(y);
			if (x == sc->syms.F) {
				error_0(sc, "non-list argument to append");
			}
		}

		s_return(sc, reverse_in_place(sc, car(y), x));

#if USE_PLIST
	case OP_PUT:        /* put */
		if (!hasprop(car(sc->regs.args)) || !hasprop(cadr(sc->regs.args))) {
			error_0(sc,"illegal use of put");
		}
		for (x = symprop(car(sc->regs.args)), y = cadr(sc->regs.args); x != sc->syms.NIL; x = cdr(x)) {
			if (caar(x) == y) {
				break;
			}
		}
		if (x != sc->syms.NIL)
			cdar(x) = caddr(sc->regs.args);
		else
			symprop(car(sc->regs.args)) = cons(sc, cons(sc, y, caddr(sc->regs.args)),
						      symprop(car(sc->regs.args)));
		s_return(sc,sc->syms.T);

	case OP_GET:        /* get */
		if (!hasprop(car(sc->regs.args)) || !hasprop(cadr(sc->regs.args))) {
			error_0(sc,"illegal use of get");
		}
		for (x = symprop(car(sc->regs.args)), y = cadr(sc->regs.args); x != sc->syms.NIL; x = cdr(x)) {
			if (caar(x) == y) {
				break;
			}
		}
		if (x != sc->syms.NIL) {
			s_return(sc,cdar(x));
		} else {
			s_return(sc,sc->syms.NIL);
		}
#endif /* USE_PLIST */
	case OP_QUIT:       /* quit */
		if(is_pair(sc->regs.args)) {
			sc->retcode=ivalue(car(sc->regs.args));
		}
		return (sc->syms.NIL);

	case OP_GC:         /* gc */
		gc(sc, sc->syms.NIL, sc->syms.NIL);
		s_return(sc,sc->syms.T);

	case OP_GCVERB:          /* gc-verbose */
	{    int  was = sc->memory.gc_verbose;

		sc->memory.gc_verbose = (car(sc->regs.args) != sc->syms.F);
		s_retbool(was);
	}

	case OP_NEWSEGMENT: /* new-segment */
		if (!is_pair(sc->regs.args) || !is_number(car(sc->regs.args))) {
			error_0(sc,"new-segment: argument must be a number");
		}
		alloc_cellseg(sc, (int) ivalue(car(sc->regs.args)));
		s_return(sc,sc->syms.T);

	case OP_OBLIST: /* oblist */
		s_return(sc, oblist_all_symbols(sc));

	case OP_CURR_INPORT: /* current-input-port */
		s_return(sc,sc->inport);

	case OP_CURR_OUTPORT: /* current-output-port */
		s_return(sc,sc->outport);

	case OP_OPEN_INFILE: /* open-input-file */
	case OP_OPEN_OUTFILE: /* open-output-file */
	case OP_OPEN_INOUTFILE: /* open-input-output-file */ {
		int prop=0;
		cell_ptr_t p;
		switch(op) {
		case OP_OPEN_INFILE:     prop=port_input; break;
		case OP_OPEN_OUTFILE:    prop=port_output; break;
		case OP_OPEN_INOUTFILE: prop=port_input|port_output; break;
		default:
			fprintf(stderr, "FATAL ERROR: should never have operation %u reaching this far\n", (unsigned int)op);
			exit(1);
		}
		p=port_from_filename(sc,strvalue(car(sc->regs.args)),prop);
		if(p==sc->syms.NIL) {
			s_return(sc,sc->syms.F);
		}
		s_return(sc,p);
	}

#if USE_STRING_PORTS
	case OP_OPEN_INSTRING: /* open-input-string */
	case OP_OPEN_INOUTSTRING: /* open-input-output-string */ {
		int prop=0;
		cell_ptr_t p;
		switch(op) {
		case OP_OPEN_INSTRING:     prop=port_input; break;
		case OP_OPEN_INOUTSTRING:  prop=port_input|port_output; break;
		default:
			fprintf(stderr, "FATAL ERROR: should never have operation %u reaching this far\n", (unsigned int)op);
			exit(1);
		}
		p=port_from_string(sc, strvalue(car(sc->regs.args)),
				   strvalue(car(sc->regs.args))+strlength(car(sc->regs.args)), prop);
		if(p==sc->syms.NIL) {
			s_return(sc,sc->syms.F);
		}
		s_return(sc,p);
	}
	case OP_OPEN_OUTSTRING: /* open-output-string */ {
		cell_ptr_t p;
		if(car(sc->regs.args)==sc->syms.NIL) {
			p=port_from_scratch(sc);
			if(p==sc->syms.NIL) {
				s_return(sc,sc->syms.F);
			}
		} else {
			p=port_from_string(sc, strvalue(car(sc->regs.args)),
					   strvalue(car(sc->regs.args))+strlength(car(sc->regs.args)),
					   port_output);
			if(p==sc->syms.NIL) {
				s_return(sc,sc->syms.F);
			}
		}
		s_return(sc,p);
	}
	case OP_GET_OUTSTRING: /* get-output-string */ {
		port_t *p;

		if( (p = car(sc->regs.args)->object.port)->kind.value & port_string ) {
			size_t	size;
			char*	str;

			size	= p->rep.string.curr - p->rep.string.start + 1;
			str	= sc->malloc(size);
			if( str != NULL ) {
				cell_ptr_t s;

				memcpy(str, p->rep.string.start, size - 1);
				str[size - 1]	= '\0';
				s=mk_string(sc, str);
				sc->free(str);
				s_return(sc, s);
			}
		}
		s_return(sc,sc->syms.F);
	}
#endif

	case OP_CLOSE_INPORT: /* close-input-port */
		port_close(sc,car(sc->regs.args),port_input);
		s_return(sc,sc->syms.T);

	case OP_CLOSE_OUTPORT: /* close-output-port */
		port_close(sc,car(sc->regs.args),port_output);
		s_return(sc,sc->syms.T);

	case OP_INT_ENV: /* interaction-environment */
		s_return(sc,sc->global_env);

	case OP_CURR_ENV: /* current-environment */
		s_return(sc,sc->regs.envir);

	/* ======= Extensions (assq, macro, closures) parts ======= */
	case OP_LIST_LENGTH:     /* length */   /* a.k */
		v=list_length(sc,car(sc->regs.args));
		if(v<0) {
			error_1(sc,"length: not a list:",car(sc->regs.args));
		}
		s_return(sc,mk_integer(sc, v));

	case OP_ASSQ:       /* assq */     /* a.k */
		x = car(sc->regs.args);
		for (y = cadr(sc->regs.args); is_pair(y); y = cdr(y)) {
			if (!is_pair(car(y))) {
				error_0(sc,"unable to handle non pair element");
			}
			if (x == caar(y))
				break;
		}
		if (is_pair(y)) {
			s_return(sc,car(y));
		} else {
			s_return(sc,sc->syms.F);
		}


	case OP_GET_CLOSURE:     /* get-closure-code */   /* a.k */
		sc->regs.args = car(sc->regs.args);
		if (sc->regs.args == sc->syms.NIL) {
			s_return(sc,sc->syms.F);
		} else if (is_closure(sc->regs.args)) {
			s_return(sc,cons(sc, sc->syms.LAMBDA, closure_code(sc->value)));
		} else if (is_macro(sc->regs.args)) {
			s_return(sc,cons(sc, sc->syms.LAMBDA, closure_code(sc->value)));
		} else {
			s_return(sc,sc->syms.F);
		}
	case OP_CLOSUREP:        /* closure? */
		/*
		* Note, macro object is also a closure.
		* Therefore, (closure? <#MACRO>) ==> #t
		*/
		s_retbool(is_closure(car(sc->regs.args)));
	case OP_MACROP:          /* macro? */
		s_retbool(is_macro(car(sc->regs.args)));

	/* ========== reading part ========== */
	case OP_READ:
		if(!is_pair(sc->regs.args)) {
			s_goto(sc,OP_READ_INTERNAL);
		}
		if(!is_inport(car(sc->regs.args))) {
			error_1(sc,"read: not an input port:",car(sc->regs.args));
		}
		if(car(sc->regs.args)==sc->inport) {
			s_goto(sc,OP_READ_INTERNAL);
		}
		x=sc->inport;
		sc->inport=car(sc->regs.args);
		x=cons(sc,x,sc->syms.NIL);
		s_save(sc,OP_SET_INPORT, x, sc->syms.NIL);
		s_goto(sc,OP_READ_INTERNAL);

	case OP_READ_CHAR: /* read-char */
	case OP_PEEK_CHAR: /* peek-char */ {
		int c;
		if(is_pair(sc->regs.args)) {
			if(car(sc->regs.args)!=sc->inport) {
				x=sc->inport;
				x=cons(sc,x,sc->syms.NIL);
				s_save(sc,OP_SET_INPORT, x, sc->syms.NIL);
				sc->inport=car(sc->regs.args);
			}
		}
		c=inchar(sc);
		if(c==EOF) {
			s_return(sc,sc->syms.EOF_OBJ);
		}
		if(sc->op==OP_PEEK_CHAR) {
			backchar(sc,c);
		}
		s_return(sc,mk_character(sc,c));
	}

	case OP_CHAR_READY: /* char-ready? */ {
		cell_ptr_t p	= sc->inport;
		int res;
		if( is_pair(sc->regs.args) ) {
			p	= car(sc->regs.args);
		}
		res = p->object.port->kind.value & port_string;
		s_retbool(res);
	}

	case OP_SET_INPORT: /* set-input-port */
		sc->inport=car(sc->regs.args);
		s_return(sc,sc->value);

	case OP_SET_OUTPORT: /* set-output-port */
		sc->outport=car(sc->regs.args);
		s_return(sc,sc->value);
	default:
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->syms.T;
}
