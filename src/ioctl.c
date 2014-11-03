#include "scheme-private.h"

cell_ptr_t op_ioctl(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x, y;
	long v;

	switch (op) {
	case OP_GENSYM:
		s_return(sc, gensym(sc));

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
		s_return(sc,sc->F);
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

			size=p->string.curr - p->string.start+1;
			str=sc->malloc(size);
			if(str != NULL) {
				cell_ptr_t s;

				memcpy(str,p->string.start,size-1);
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
		snprintf(sc->strbuff,STRBUFFSIZE,"%d: illegal operator", sc->op);
		error_0(sc,sc->strbuff);
	}
	return sc->T; /* NOTREACHED */
}
