#include "scheme-private.h"


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

/*******************************************************************************
 *
 * Number section
 *
 ******************************************************************************/
static long binary_decode(const char *s) {
	long x	= 0;

	while( *s != 0 && (*s=='1' || *s=='0')) {
		x	<<= 1;
		x	+= *s - '0';
		s++;
	}

	return x;
}

number_t num_add(number_t a,
		 number_t b)
{
	number_t	ret;
	ret.is_integer	= a.is_integer && b.is_integer;
	if( ret.is_integer ) {
		ret.value.ivalue	= a.value.ivalue + b.value.ivalue;
	} else {
		ret.value.rvalue	= num_rvalue(a) + num_rvalue(b);
	}
	return ret;
}

number_t num_mul(number_t a,
		 number_t b)
{
	number_t	ret;
	ret.is_integer	= a.is_integer && b.is_integer;
	if( ret.is_integer ) {
		ret.value.ivalue	= a.value.ivalue * b.value.ivalue;
	} else {
		ret.value.rvalue	= num_rvalue(a) * num_rvalue(b);
	}
	return ret;
}

number_t num_div(number_t a,
		 number_t b)
{
	number_t	ret;
	ret.is_integer	= a.is_integer && b.is_integer && (a.value.ivalue % b.value.ivalue == 0);
	if( ret.is_integer ) {
		ret.value.ivalue	= a.value.ivalue / b.value.ivalue;
	} else {
		ret.value.rvalue	= num_rvalue(a) / num_rvalue(b);
	}
	return ret;
}

number_t num_intdiv(number_t a,
		    number_t b)
{
	number_t	ret;
	ret.is_integer	= a.is_integer && b.is_integer;
	if( ret.is_integer ) {
		ret.value.ivalue	= a.value.ivalue / b.value.ivalue;
	} else {
		ret.value.rvalue	= num_rvalue(a) / num_rvalue(b);
	}
	return ret;
}

number_t num_sub(number_t a,
		 number_t b)
{
	number_t	ret;
	ret.is_integer	= a.is_integer && b.is_integer;
	if( ret.is_integer ) {
		ret.value.ivalue	= a.value.ivalue - b.value.ivalue;
	} else {
		ret.value.rvalue	= num_rvalue(a) - num_rvalue(b);
	}
	return ret;
}

number_t num_rem(number_t a,
		 number_t b)
{
	number_t	ret;
	long		e1, e2, res;
	ret.is_integer	= a.is_integer && b.is_integer;
	e1	= num_ivalue(a);
	e2	= num_ivalue(b);
	res	= e1 % e2;
	/* remainder should have same sign as second operand */
	if( res > 0 ) {
		if( e1 < 0 ) {
			res -= labs(e2);
		}
	} else if( res < 0 ) {
		if( e1 > 0 ) {
			res += labs(e2);
		}
	}
	ret.value.ivalue=res;
	return ret;
}

number_t
num_mod(number_t a,
	number_t b)
{
	number_t	ret;
	long		e1, e2, res;
	ret.is_integer	= a.is_integer && b.is_integer;
	e1	= num_ivalue(a);
	e2	= num_ivalue(b);
	res	= e1 % e2;

	/* modulo should have same sign as second operand */
	if( res * e2 < 0 ) {
		res += e2;
	}

	ret.value.ivalue = res;
	return ret;
}

int
num_eq(number_t a,
       number_t b)
{
	int	ret;
	int	is_integer	= a.is_integer && b.is_integer;
	if( is_integer ) {
		ret	= a.value.ivalue == b.value.ivalue;
	} else {
		ret	= num_rvalue(a) == num_rvalue(b);
	}
	return ret;
}


int
num_gt(number_t a,
       number_t b)
{
	int	ret;
	int	is_integer	= a.is_integer && b.is_integer;
	if( is_integer ) {
		ret	= a.value.ivalue > b.value.ivalue;
	} else {
		ret	= num_rvalue(a)>num_rvalue(b);
	}
	return ret;
}

int
num_ge(number_t a,
       number_t b)
{
	return !num_lt(a, b);
}

int
num_lt(number_t a,
       number_t b)
{
	int	ret;
	int	is_integer	= a.is_integer && b.is_integer;
	if( is_integer ) {
		ret	= a.value.ivalue < b.value.ivalue;
	} else {
		ret	= num_rvalue(a) < num_rvalue(b);
	}
	return ret;
}

int num_le(number_t a,
	   number_t b)
{
	return !num_gt(a,b);
}

#if USE_MATH
/* Round to nearest. Round to even if midway */
double
round_per_R5RS(double x)
{
	double fl	= floor(x);
	double ce	= ceil(x);
	double dfl	= x - fl;
	double dce	= ce - x;
	if( dfl > dce ) {
		return ce;
	} else if( dfl < dce ) {
		return fl;
	} else {
		if( fmod(fl, 2.0) == 0.0 ) {       /* I imagine this holds */
			return fl;
		} else {
			return ce;
		}
	}
}
#endif

/*******************************************************************************
 *
 * Atoms
 *
 *******************************************************************************/
cell_ptr_t
mk_port(scheme_t *sc,
	port_t *p)
{
	cell_ptr_t x	= get_cell(sc, sc->syms.NIL, sc->syms.NIL);

	typeflag(x)	= T_PORT | T_ATOM;
	x->object.port	= p;
	return (x);
}

cell_ptr_t
mk_foreign_func(scheme_t *sc,
		foreign_func f)
{
	cell_ptr_t x	= get_cell(sc, sc->syms.NIL, sc->syms.NIL);

	typeflag(x)	= T_FOREIGN | T_ATOM;
	x->object.ff	= f;
	return (x);
}

cell_ptr_t
mk_character(scheme_t *sc,
	     int c)
{
	cell_ptr_t x	= get_cell(sc, sc->syms.NIL, sc->syms.NIL);

	typeflag(x)	= T_CHARACTER | T_ATOM;
	ivalue_unchecked(x)	= c;
	set_num_integer(x);
	return (x);
}

/* get number atom (integer) */
INTERFACE cell_ptr_t mk_integer(scheme_t *sc, long number_t) {
	cell_ptr_t x = get_cell(sc,sc->syms.NIL, sc->syms.NIL);

	typeflag(x) = (T_NUMBER | T_ATOM);
	ivalue_unchecked(x)= number_t;
	set_num_integer(x);
	return (x);
}

INTERFACE cell_ptr_t mk_real(scheme_t *sc, double n) {
	cell_ptr_t x = get_cell(sc,sc->syms.NIL, sc->syms.NIL);

	typeflag(x) = (T_NUMBER | T_ATOM);
	rvalue_unchecked(x)= n;
	set_num_real(x);
	return (x);
}

cell_ptr_t mk_number(scheme_t *sc, number_t n) {
	if(n.is_integer) {
		return mk_integer(sc,n.value.ivalue);
	} else {
		return mk_real(sc,n.value.rvalue);
	}
}

/* allocate name to string area */
char*
store_string(scheme_t *sc,
	     int len_str,
	     const char *str,
	     char fill)
{
	char *q;

	q	= (char*)sc->malloc(len_str + 1);

	if( q == 0 ) {
		sc->memory.no_memory	= true;
		return sc->regs.strbuff;
	}

	if( str != 0 ) {
		snprintf(q, len_str + 1, "%s", str);
	} else {
		memset(q, fill, len_str);
		q[len_str]	= 0;
	}

	return (q);
}

/* get new string */
INTERFACE cell_ptr_t mk_string(scheme_t *sc, const char *str) {
	return mk_counted_string(sc,str,strlen(str));
}

INTERFACE cell_ptr_t mk_counted_string(scheme_t *sc, const char *str, int len) {
	cell_ptr_t x = get_cell(sc, sc->syms.NIL, sc->syms.NIL);
	typeflag(x) = (T_STRING | T_ATOM);
	strvalue(x) = store_string(sc,len,str,0);
	strlength(x) = len;
	return (x);
}

INTERFACE cell_ptr_t mk_empty_string(scheme_t *sc, int len, char fill) {
	cell_ptr_t x = get_cell(sc, sc->syms.NIL, sc->syms.NIL);
	typeflag(x) = (T_STRING | T_ATOM);
	strvalue(x) = store_string(sc,len,0,fill);
	strlength(x) = len;
	return (x);
}

INTERFACE cell_ptr_t
mk_vector(scheme_t *sc,
	  int len)
{
	return get_vector_object(sc,len,sc->syms.NIL);
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
INTERFACE cell_ptr_t mk_symbol(scheme_t *sc, const char *name) {
	cell_ptr_t x;

	/* first check oblist */
	x = oblist_find_by_name(sc, name);
	if (x != sc->syms.NIL) {
		return (x);
	} else {
		x = oblist_add_by_name(sc, name);
		return (x);
	}
}

INTERFACE cell_ptr_t gensym(scheme_t *sc) {
	cell_ptr_t x;
	char name[40];

	for(; sc->gensym_cnt<LONG_MAX; sc->gensym_cnt++) {
		snprintf(name,40,"gensym-%ld",sc->gensym_cnt);

		/* first check oblist */
		x = oblist_find_by_name(sc, name);

		if (x != sc->syms.NIL) {
			continue;
		} else {
			x = oblist_add_by_name(sc, name);
			return (x);
		}
	}

	return sc->syms.NIL;
}

/* make symbol or number atom from string */
cell_ptr_t
mk_atom(scheme_t *sc,
	char *q)
{
	char    c, *p;
	int has_dec_point=0;
	int has_fp_exp = 0;

#if USE_COLON_HOOK
	if((p=strstr(q,"::"))!=0) {
		*p=0;
		return cons(sc, sc->syms.COLON_HOOK,
			    cons(sc,
				 cons(sc,
				      sc->syms.QUOTE,
				      cons(sc, mk_atom(sc,p+2), sc->syms.NIL)),
				 cons(sc, mk_symbol(sc,strlwr(q)), sc->syms.NIL)));
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
			}
			else if ((c == 'e') || (c == 'E')) {
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

#if USE_ASCII_NAMES
static const char *charnames[32]={
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

/* make constant */
cell_ptr_t
mk_sharp_const(scheme_t *sc,
	       char *name)
{
	long    x;
	char    tmp[STRBUFFSIZE];

	if (!strcmp(name, "t"))
		return (sc->syms.T);
	else if (!strcmp(name, "f"))
		return (sc->syms.F);
	else if (*name == 'o') {/* #o (octal) */
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
			if( sscanf(name+2,"%x",(unsigned int *)&c1)==1 && c1 < UCHAR_MAX ) {
				c=c1;
			} else {
				return sc->syms.NIL;
			}
#if USE_ASCII_NAMES
		} else if(is_ascii_name(name+1,&c)) {
			/* nothing */
#endif
		} else if(name[2]==0) {
			c=name[1];
		} else {
			return sc->syms.NIL;
		}
		return mk_character(sc,c);
	} else
		return (sc->syms.NIL);
}

/* equivalence of atoms */
int eqv(cell_ptr_t a, cell_ptr_t b) {
	if (is_string(a)) {
		if (is_string(b))
			return (strvalue(a) == strvalue(b));
		else
			return (0);
	} else if (is_number(a)) {
		if (is_number(b)) {
			if (num_is_integer(a) == num_is_integer(b))
				return num_eq(nvalue(a),nvalue(b));
		}
		return (0);
	} else if (is_character(a)) {
		if (is_character(b))
			return charvalue(a)==charvalue(b);
		else
			return (0);
	} else if (is_port(a)) {
		if (is_port(b))
			return a==b;
		else
			return (0);
	} else if (is_proc(a)) {
		if (is_proc(b))
			return procnum(a)==procnum(b);
		else
			return (0);
	} else {
		return (a == b);
	}
}

/* ========== Routines for Printing ========== */
static void printslashstring(scheme_t *sc, char *p, int len) {
	int i;
	unsigned char *s=(unsigned char*)p;
	putcharacter(sc,'"');
	for ( i=0; i<len; i++) {
		if(*s==0xff || *s=='"' || *s<' ' || *s=='\\') {
			putcharacter(sc,'\\');
			switch(*s) {
			case '"':
				putcharacter(sc,'"');
				break;
			case '\n':
				putcharacter(sc,'n');
				break;
			case '\t':
				putcharacter(sc,'t');
				break;
			case '\r':
				putcharacter(sc,'r');
				break;
			case '\\':
				putcharacter(sc,'\\');
				break;
			default: {
				int d=*s/16;
				putcharacter(sc,'x');
				if(d<10) {
					putcharacter(sc,d+'0');
				} else {
					putcharacter(sc,d-10+'A');
				}
				d=*s%16;
				if(d<10) {
					putcharacter(sc,d+'0');
				} else {
					putcharacter(sc,d-10+'A');
				}
			}
			}
		} else {
			putcharacter(sc,*s);
		}
		s++;
	}
	putcharacter(sc,'"');
}

/* print atoms */
void printatom(scheme_t *sc, cell_ptr_t l, int f) {
	char *p;
	int len;
	atom2str(sc,l,f,&p,&len);
	putchars(sc,p,len);
}


/* Uses internal buffer unless string cell_ptr_t is already available */
void atom2str(scheme_t *sc, cell_ptr_t l, int f, char **pp, int *plen) {
	char *p;

	if (l == sc->syms.NIL) {
		p = "()";
	} else if (l == sc->syms.T) {
		p = "#t";
	} else if (l == sc->syms.F) {
		p = "#f";
	} else if (l == sc->syms.EOF_OBJ) {
		p = "#<EOF>";
	} else if (is_port(l)) {
		p = sc->regs.strbuff;
		snprintf(p, STRBUFFSIZE, "#<PORT>");
	} else if (is_number(l)) {
		p = sc->regs.strbuff;
		if (f <= 1 || f == 10) /* f is the base for numbers if > 1 */ {
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
				do { *--p = (b&1) ? '1' : '0'; b >>= 1; } while (b != 0);
				if (v < 0) *--p = '-';
			}
		}
	} else if (is_string(l)) {
		if (!f) {
			p = strvalue(l);
		} else { /* Hack, uses the fact that printing is needed */
			*pp=sc->regs.strbuff;
			*plen=0;
			printslashstring(sc, strvalue(l), strlength(l));
			return;
		}
	} else if (is_character(l)) {
		int c=charvalue(l);
		p = sc->regs.strbuff;
		if (!f) {
			p[0]=c;
			p[1]=0;
		} else {
			switch(c) {
			case ' ':
				snprintf(p,STRBUFFSIZE,"#\\space"); break;
			case '\n':
				snprintf(p,STRBUFFSIZE,"#\\newline"); break;
			case '\r':
				snprintf(p,STRBUFFSIZE,"#\\return"); break;
			case '\t':
				snprintf(p,STRBUFFSIZE,"#\\tab"); break;
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
					snprintf(p,STRBUFFSIZE,"#\\x%x",c); break;
					break;
				}
#endif
				snprintf(p,STRBUFFSIZE,"#\\%c",c); break;
				break;
			}
		}
	} else if (is_symbol(l)) {
		p = symname(l);
	} else if (is_proc(l)) {
		p = sc->regs.strbuff;
		snprintf(p,STRBUFFSIZE,"#<%s PROCEDURE %ld>", procname(l),procnum(l));
	} else if (is_macro(l)) {
		p = "#<MACRO>";
	} else if (is_closure(l)) {
		p = "#<CLOSURE>";
	} else if (is_promise(l)) {
		p = "#<PROMISE>";
	} else if (is_foreign(l)) {
		p = sc->regs.strbuff;
		snprintf(p,STRBUFFSIZE,"#<FOREIGN PROCEDURE %ld>", procnum(l));
	} else if (is_continuation(l)) {
		p = "#<CONTINUATION>";
	} else {
		p = "#<ERROR>";
	}
	*pp=p;
	*plen=strlen(p);
}

