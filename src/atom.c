#include "scheme-private.h"

#if USE_ASCII_NAMES
static const char *charnames[32]= {
	"nul",
	"soh",
	"stx",
	"etx",
	"eot",
	"enq",
	"ack",
	"bel",
	"bs",
	"ht",
	"lf",
	"vt",
	"ff",
	"cr",
	"so",
	"si",
	"dle",
	"dc1",
	"dc2",
	"dc3",
	"dc4",
	"nak",
	"syn",
	"etb",
	"can",
	"em",
	"sub",
	"esc",
	"fs",
	"gs",
	"rs",
	"us"
};

static int is_ascii_name(const char *name, int *pc) {
	int i;
	for(i=0; i<32; i++) {
		if(stricmp(name,charnames[i])==0) {
			*pc=i;
			return 1;
		}
	}
	if(stricmp(name,"del")==0) {
		*pc=127;
		return 1;
	}
	return 0;
}

#endif

#if USE_STRLWR
static const char *strlwr(char *s) {
	const char *p=s;
	while(*s) {
		*s=tolower(*s);
		s++;
	}
	return p;
}
#endif

/* ========== oblist implementation  ========== */

#ifndef USE_OBJECT_LIST

cell_ptr_t oblist_initial_value(scheme_t *sc) {
	return mk_vector(sc, 461); /* probably should be bigger */
}

/* returns the new symbol */
cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name) {
	cell_ptr_t x;
	int location;

	x = immutable_cons(sc, mk_string(sc, name), sc->NIL);
	typeflag(x) = T_SYMBOL;
	setimmutable(car(x));

	location = hash_fn(name, ivalue_unchecked(sc->oblist));
	set_vector_elem(sc->oblist, location,
			immutable_cons(sc, x, vector_elem(sc->oblist, location)));
	return x;
}

cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name) {
	int location;
	cell_ptr_t x;
	char *s;

	location = hash_fn(name, ivalue_unchecked(sc->oblist));
	for (x = vector_elem(sc->oblist, location); x != sc->NIL; x = cdr(x)) {
		s = symname(car(x));
		/* case-insensitive, per R5RS section 2. */
		if(stricmp(name, s) == 0) {
			return car(x);
		}
	}
	return sc->NIL;
}

cell_ptr_t oblist_all_symbols(scheme_t *sc) {
	int i;
	cell_ptr_t x;
	cell_ptr_t ob_list = sc->NIL;

	for (i = 0; i < ivalue_unchecked(sc->oblist); i++) {
		for (x  = vector_elem(sc->oblist, i); x != sc->NIL; x = cdr(x)) {
			ob_list = cons(sc, x, ob_list);
		}
	}
	return ob_list;
}

#else

cell_ptr_t oblist_initial_value(scheme_t *sc) {
	return sc->NIL;
}

cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name) {
	cell_ptr_t x;
	char    *s;

	for (x = sc->oblist; x != sc->NIL; x = cdr(x)) {
		s = symname(car(x));
		/* case-insensitive, per R5RS section 2. */
		if(stricmp(name, s) == 0) {
			return car(x);
		}
	}
	return sc->NIL;
}

/* returns the new symbol */
cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name) {
	cell_ptr_t x;

	x = immutable_cons(sc, mk_string(sc, name), sc->NIL);
	typeflag(x) = T_SYMBOL;
	setimmutable(car(x));
	sc->oblist = immutable_cons(sc, x, sc->oblist);
	return x;
}

cell_ptr_t oblist_all_symbols(scheme_t *sc) {
	return sc->oblist;
}

#endif

cell_ptr_t mk_foreign_func(scheme_t *sc, foreign_func f) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);

	typeflag(x) = (T_FOREIGN | T_ATOM);
	x->object.ff=f;
	return (x);
}

cell_ptr_t mk_character(scheme_t *sc, int c) {
	cell_ptr_t x = get_cell(sc,sc->NIL, sc->NIL);

	typeflag(x) = (T_CHARACTER | T_ATOM);
	ivalue_unchecked(x)= c;
	set_num_integer(x);
	return (x);
}

/* get number atom (integer) */
cell_ptr_t mk_integer(scheme_t *sc, long number_t) {
	cell_ptr_t x = get_cell(sc,sc->NIL, sc->NIL);

	typeflag(x) = (T_NUMBER | T_ATOM);
	ivalue_unchecked(x)= number_t;
	set_num_integer(x);
	return (x);
}

cell_ptr_t mk_real(scheme_t *sc, double n) {
	cell_ptr_t x = get_cell(sc,sc->NIL, sc->NIL);

	typeflag(x) = (T_NUMBER | T_ATOM);
	rvalue_unchecked(x)= n;
	set_num_real(x);
	return (x);
}


/* allocate name to string area */
char *store_string(scheme_t *sc, int len_str, const char *str, char fill) {
	char *q;

	q=(char*)sc->malloc(len_str+1);
	if(q==0) {
		sc->no_memory=1;
		return sc->strbuff;
	}
	if(str!=0) {
		snprintf(q, len_str+1, "%s", str);
	} else {
		memset(q, fill, len_str);
		q[len_str]=0;
	}
	return (q);
}

/* get new string */
cell_ptr_t mk_string(scheme_t *sc, const char *str) {
	return mk_counted_string(sc,str,strlen(str));
}

cell_ptr_t mk_counted_string(scheme_t *sc, const char *str, int len) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);
	typeflag(x) = (T_STRING | T_ATOM);
	strvalue(x) = store_string(sc,len,str,0);
	strlength(x) = len;
	return (x);
}

cell_ptr_t mk_empty_string(scheme_t *sc, int len, char fill) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);
	typeflag(x) = (T_STRING | T_ATOM);
	strvalue(x) = store_string(sc,len,0,fill);
	strlength(x) = len;
	return (x);
}

cell_ptr_t mk_vector(scheme_t *sc, int len) {
	return get_vector_object(sc,len,sc->NIL);
}

void fill_vector(cell_ptr_t vec, cell_ptr_t obj) {
	int i;
	int number_t=ivalue(vec)/2+ivalue(vec)%2;
	for(i=0; i<number_t; i++) {
		typeflag(vec+1+i) = T_PAIR;
		setimmutable(vec+1+i);
		car(vec+1+i)=obj;
		cdr(vec+1+i)=obj;
	}
}

cell_ptr_t vector_elem(cell_ptr_t vec, int ielem) {
	int n=ielem/2;
	if(ielem%2==0) {
		return car(vec+1+n);
	} else {
		return cdr(vec+1+n);
	}
}

cell_ptr_t set_vector_elem(cell_ptr_t vec, int ielem, cell_ptr_t a) {
	int n=ielem/2;
	if(ielem%2==0) {
		return car(vec+1+n)=a;
	} else {
		return cdr(vec+1+n)=a;
	}
}

/* get new symbol */
cell_ptr_t mk_symbol(scheme_t *sc, const char *name) {
	cell_ptr_t x;

	/* first check oblist */
	x = oblist_find_by_name(sc, name);
	if (x != sc->NIL) {
		return (x);
	} else {
		x = oblist_add_by_name(sc, name);
		return (x);
	}
}

cell_ptr_t gensym(scheme_t *sc) {
	cell_ptr_t x;
	char name[40];

	for(; sc->gensym_cnt<LONG_MAX; sc->gensym_cnt++) {
		snprintf(name,40,"gensym-%ld",sc->gensym_cnt);

		/* first check oblist */
		x = oblist_find_by_name(sc, name);

		if (x != sc->NIL) {
			continue;
		} else {
			x = oblist_add_by_name(sc, name);
			return (x);
		}
	}

	return sc->NIL;
}

/* make symbol or number atom from string */
cell_ptr_t mk_atom(scheme_t *sc, char *q) {
	char    c, *p;
	int has_dec_point=0;
	int has_fp_exp = 0;

#if USE_COLON_HOOK
	if((p=strstr(q,"::"))!=0) {
		*p=0;
		return cons(sc, sc->COLON_HOOK,
			    cons(sc,
				 cons(sc,
				      sc->QUOTE,
				      cons(sc, mk_atom(sc,p+2), sc->NIL)),
				 cons(sc, mk_symbol(sc,strlwr(q)), sc->NIL)));
	}
#endif

	p = q;
	c = *p++;
	if ((c == '+') || (c == '-')) {
		c = *p++;
		if (c == '.') {
			has_dec_point=1;
			c = *p++;
		}
		if (!isdigit(c)) {
			return (mk_symbol(sc, strlwr(q)));
		}
	} else if (c == '.') {
		has_dec_point=1;
		c = *p++;
		if (!isdigit(c)) {
			return (mk_symbol(sc, strlwr(q)));
		}
	} else if (!isdigit(c)) {
		return (mk_symbol(sc, strlwr(q)));
	}

	for ( ; (c = *p) != 0; ++p) {
		if (!isdigit(c)) {
			if(c=='.') {
				if(!has_dec_point) {
					has_dec_point=1;
					continue;
				}
			} else if ((c == 'e') || (c == 'E')) {
				if(!has_fp_exp) {
					has_dec_point = 1; /* decimal point illegal
						from now on */
					p++;
					if ((*p == '-') || (*p == '+') || isdigit(*p)) {
						continue;
					}
				}
			}
			return (mk_symbol(sc, strlwr(q)));
		}
	}
	if(has_dec_point) {
		return mk_real(sc,atof(q));
	}
	return (mk_integer(sc, atol(q)));
}

/* make constant */
cell_ptr_t mk_sharp_const(scheme_t *sc, char *name) {
	long    x;
	char    tmp[STRBUFFSIZE];

	if (!strcmp(name, "t"))
		return (sc->T);
	else if (!strcmp(name, "f"))
		return (sc->F);
	else if (*name == 'o') { /* #o (octal) */
		snprintf(tmp, STRBUFFSIZE, "0%s", name+1);
		sscanf(tmp, "%lo", (long unsigned *)&x);
		return (mk_integer(sc, x));
	} else if (*name == 'd') {    /* #d (decimal) */
		sscanf(name+1, "%ld", (long int *)&x);
		return (mk_integer(sc, x));
	} else if (*name == 'x') {    /* #x (hex) */
		snprintf(tmp, STRBUFFSIZE, "0x%s", name+1);
		sscanf(tmp, "%lx", (long unsigned *)&x);
		return (mk_integer(sc, x));
	} else if (*name == 'b') {    /* #b (binary) */
		x = binary_decode(name+1);
		return (mk_integer(sc, x));
	} else if (*name == '\\') { /* #\w (character) */
		int c=0;
		if(stricmp(name+1,"space")==0) {
			c=' ';
		} else if(stricmp(name+1,"newline")==0) {
			c='\n';
		} else if(stricmp(name+1,"return")==0) {
			c='\r';
		} else if(stricmp(name+1,"tab")==0) {
			c='\t';
		} else if(name[1]=='x' && name[2]!=0) {
			int c1=0;
			if(sscanf(name+2,"%x",(unsigned int *)&c1)==1 && c1 < UCHAR_MAX) {
				c=c1;
			} else {
				return sc->NIL;
			}
#if USE_ASCII_NAMES
		} else if(is_ascii_name(name+1,&c)) {
			/* nothing */
#endif
		} else if(name[2]==0) {
			c=name[1];
		} else {
			return sc->NIL;
		}
		return mk_character(sc,c);
	} else
		return (sc->NIL);
}

static void
putcharacter(char ch) {
	fprintf(stdout, "%c", ch);
}

static void printslashstring(scheme_t *sc, char *p, int len) {
	int i;
	unsigned char *s=(unsigned char*)p;
	putcharacter('"');
	for ( i=0; i<len; i++) {
		if(*s==0xff || *s=='"' || *s<' ' || *s=='\\') {
			putcharacter('\\');
			switch(*s) {
			case '"':
				putcharacter('"');
				break;
			case '\n':
				putcharacter('n');
				break;
			case '\t':
				putcharacter('t');
				break;
			case '\r':
				putcharacter('r');
				break;
			case '\\':
				putcharacter('\\');
				break;
			default: {
				int d=*s/16;
				putcharacter('x');
				if(d<10) {
					putcharacter(d+'0');
				} else {
					putcharacter(d-10+'A');
				}
				d=*s%16;
				if(d<10) {
					putcharacter(d+'0');
				} else {
					putcharacter(d-10+'A');
				}
			}
			}
		} else {
			putcharacter(*s);
		}
		s++;
	}
	putcharacter('"');
}


/* print atoms */
void printatom(scheme_t *sc, cell_ptr_t l, int f) {
	char *p;
	int len;
	atom2str(sc,l,f,&p,&len);
	for( int i = 0; i < len; ++i ) {
		fprintf(stdout, "%c", p[i]);
	}
}


/* Uses internal buffer unless string cell_ptr_t is already available */
void atom2str(scheme_t *sc, cell_ptr_t l, int f, char **pp, int *plen) {
	char *p;

	if (l == sc->NIL) {
		p = "()";
	} else if (l == sc->T) {
		p = "#t";
	} else if (l == sc->F) {
		p = "#f";
	} else if (l == sc->EOF_OBJ) {
		p = "#<EOF>";
	} else if (is_number(l)) {
		p = sc->strbuff;
		if (f <= 1 || f == 10) { /* f is the base for numbers if > 1 */
			if(num_is_integer(l)) {
				snprintf(p, STRBUFFSIZE, "%ld", ivalue_unchecked(l));
			} else {
				snprintf(p, STRBUFFSIZE, "%.10g", rvalue_unchecked(l));
				/* r5rs says there must be a '.' (unless 'e'?) */
				f = strcspn(p, ".e");
				if (p[f] == 0) {
					p[f] = '.'; /* not found, so add '.0' at the end */
					p[f+1] = '0';
					p[f+2] = 0;
				}
			}
		} else {
			long v = ivalue(l);
			if (f == 16) {
				if (v >= 0)
					snprintf(p, STRBUFFSIZE, "%lx", v);
				else
					snprintf(p, STRBUFFSIZE, "-%lx", -v);
			} else if (f == 8) {
				if (v >= 0)
					snprintf(p, STRBUFFSIZE, "%lo", v);
				else
					snprintf(p, STRBUFFSIZE, "-%lo", -v);
			} else if (f == 2) {
				unsigned long b = (v < 0) ? -v : v;
				p = &p[STRBUFFSIZE-1];
				*p = 0;
				do {
					*--p = (b&1) ? '1' : '0';
					b >>= 1;
				} while (b != 0);
				if (v < 0) *--p = '-';
			}
		}
	} else if (is_string(l)) {
		if (!f) {
			p = strvalue(l);
		} else { /* Hack, uses the fact that printing is needed */
			*pp=sc->strbuff;
			*plen=0;
			printslashstring(sc, strvalue(l), strlength(l));
			return;
		}
	} else if (is_character(l)) {
		int c=charvalue(l);
		p = sc->strbuff;
		if (!f) {
			p[0]=c;
			p[1]=0;
		} else {
			switch(c) {
			case ' ':
				snprintf(p,STRBUFFSIZE,"#\\space");
				break;
			case '\n':
				snprintf(p,STRBUFFSIZE,"#\\newline");
				break;
			case '\r':
				snprintf(p,STRBUFFSIZE,"#\\return");
				break;
			case '\t':
				snprintf(p,STRBUFFSIZE,"#\\tab");
				break;
			default:
#if USE_ASCII_NAMES
				if(c==127) {
					snprintf(p,STRBUFFSIZE, "#\\del");
					break;
				} else if(c<32) {
					snprintf(p, STRBUFFSIZE, "#\\%s", charnames[c]);
					break;
				}
#else
				if(c<32) {
					snprintf(p,STRBUFFSIZE,"#\\x%x",c);
					break;
					break;
				}
#endif
				snprintf(p,STRBUFFSIZE,"#\\%c",c);
				break;
				break;
			}
		}
	} else if (is_symbol(l)) {
		p = symname(l);
	} else if (is_proc(l)) {
		p = sc->strbuff;
		snprintf(p,STRBUFFSIZE,"#<%s PROCEDURE %ld>", procname(l),procnum(l));
	} else if (is_macro(l)) {
		p = "#<MACRO>";
	} else if (is_closure(l)) {
		p = "#<CLOSURE>";
	} else if (is_promise(l)) {
		p = "#<PROMISE>";
	} else if (is_foreign(l)) {
		p = sc->strbuff;
		snprintf(p,STRBUFFSIZE,"#<FOREIGN PROCEDURE %ld>", procnum(l));
	} else if (is_continuation(l)) {
		p = "#<CONTINUATION>";
	} else {
		p = "#<ERROR>";
	}
	*pp=p;
	*plen=strlen(p);
}

cell_ptr_t op_atom(scheme_t *sc, enum scheme_opcodes op) {
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
