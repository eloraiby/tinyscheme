#include "scheme-private.h"

#if USE_MATH
# include <math.h>
#endif

#include <limits.h>
#include <float.h>

/*******************************************************************************
 *
 * Number section
 *
 ******************************************************************************/

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

int
is_zero_double(double x)
{
	return x < DBL_MIN && x > -DBL_MIN;
}

/*******************************************************************************
 *
 * Atoms
 *
 *******************************************************************************/
cell_ptr_t
mk_port(scheme_t *sc, port_t *p) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);

	typeflag(x) = T_PORT|T_ATOM;
	x->_object._port=p;
	return (x);
}

cell_ptr_t mk_foreign_func(scheme_t *sc, foreign_func f) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);

	typeflag(x) = (T_FOREIGN | T_ATOM);
	x->_object._ff=f;
	return (x);
}

INTERFACE cell_ptr_t mk_character(scheme_t *sc, int c) {
	cell_ptr_t x = get_cell(sc,sc->NIL, sc->NIL);

	typeflag(x) = (T_CHARACTER | T_ATOM);
	ivalue_unchecked(x)= c;
	set_num_integer(x);
	return (x);
}

/* get number atom (integer) */
INTERFACE cell_ptr_t mk_integer(scheme_t *sc, long number_t) {
	cell_ptr_t x = get_cell(sc,sc->NIL, sc->NIL);

	typeflag(x) = (T_NUMBER | T_ATOM);
	ivalue_unchecked(x)= number_t;
	set_num_integer(x);
	return (x);
}

INTERFACE cell_ptr_t mk_real(scheme_t *sc, double n) {
	cell_ptr_t x = get_cell(sc,sc->NIL, sc->NIL);

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
INTERFACE cell_ptr_t mk_string(scheme_t *sc, const char *str) {
	return mk_counted_string(sc,str,strlen(str));
}

INTERFACE cell_ptr_t mk_counted_string(scheme_t *sc, const char *str, int len) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);
	typeflag(x) = (T_STRING | T_ATOM);
	strvalue(x) = store_string(sc,len,str,0);
	strlength(x) = len;
	return (x);
}

INTERFACE cell_ptr_t mk_empty_string(scheme_t *sc, int len, char fill) {
	cell_ptr_t x = get_cell(sc, sc->NIL, sc->NIL);
	typeflag(x) = (T_STRING | T_ATOM);
	strvalue(x) = store_string(sc,len,0,fill);
	strlength(x) = len;
	return (x);
}

INTERFACE cell_ptr_t mk_vector(scheme_t *sc, int len)
{ return get_vector_object(sc,len,sc->NIL); }

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
	if (x != sc->NIL) {
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

/* make constant */
cell_ptr_t
mk_sharp_const(scheme_t *sc,
	       char *name)
{
	long    x;
	char    tmp[STRBUFFSIZE];

	if (!strcmp(name, "t"))
		return (sc->T);
	else if (!strcmp(name, "f"))
		return (sc->F);
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

/* make closure. c is code. e is environment */
cell_ptr_t mk_closure(scheme_t *sc, cell_ptr_t c, cell_ptr_t e) {
	cell_ptr_t x = get_cell(sc, c, e);

	typeflag(x) = T_CLOSURE;
	car(x) = c;
	cdr(x) = e;
	return (x);
}

/* make continuation. */
cell_ptr_t mk_continuation(scheme_t *sc, cell_ptr_t d) {
	cell_ptr_t x = get_cell(sc, sc->NIL, d);

	typeflag(x) = T_CONTINUATION;
	cont_dump(x) = d;
	return (x);
}

cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t y;

	y = get_cell(sc, sc->NIL, sc->NIL);
	typeflag(y) = (T_PROC | T_ATOM);
	ivalue_unchecked(y) = (long) op;
	set_num_integer(y);
	return y;
}