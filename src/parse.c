#include "scheme-private.h"

#include "parser.h"

#ifndef prompt
# define prompt "ts> "
#endif

#define BACKQUOTE '`'
#define DELIMITERS  "()\";\f\t\v\n\r "

/* ========== Routines for Printing ========== */
#define   ok_abbrev(x)   (is_pair(x) && cdr(x) == sc->NIL)

/* ========== Routines for Reading ========== */
static INLINE int is_one_of(char *s, int c);
static char   *readstr_upto(scheme_t *sc, char *delim);
static cell_ptr_t readstrexp(scheme_t *sc);
static INLINE int skipspace(scheme_t *sc);


/* read characters up to delimiter, but cater to character constants */
static char *readstr_upto(scheme_t *sc, char *delim) {
	char *p = sc->strbuff;

	while( ((size_t)(p - sc->strbuff) < sizeof(sc->strbuff)) &&
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
		if(c == EOF || (size_t)(p - sc->strbuff) > sizeof(sc->strbuff)-1) {
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

cell_ptr_t op_parse(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x;

	if(sc->nesting!=0) {
		int n=sc->nesting;
		sc->nesting=0;
		sc->retcode=-1;
		error_1(sc,"unmatched parentheses:",mk_integer(sc,n));
	}

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
