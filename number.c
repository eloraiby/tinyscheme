#include "scheme-private.h"

static number_t num_add(number_t a, number_t b);
static number_t num_mul(number_t a, number_t b);
static number_t num_div(number_t a, number_t b);
static number_t num_intdiv(number_t a, number_t b);
static number_t num_sub(number_t a, number_t b);
static number_t num_rem(number_t a, number_t b);
static number_t num_mod(number_t a, number_t b);
static cell_ptr_t mk_number(scheme_t *sc, number_t n);

#if USE_MATH
static double round_per_R5RS(double x);
#endif
static int is_zero_double(double x);

static number_t num_add(number_t a, number_t b) {
	number_t ret;
	ret.is_integer=a.is_integer && b.is_integer;
	if(ret.is_integer) {
		ret.value.ivalue= a.value.ivalue+b.value.ivalue;
	} else {
		ret.value.rvalue=num_rvalue(a)+num_rvalue(b);
	}
	return ret;
}

static number_t num_mul(number_t a, number_t b) {
	number_t ret;
	ret.is_integer=a.is_integer && b.is_integer;
	if(ret.is_integer) {
		ret.value.ivalue= a.value.ivalue*b.value.ivalue;
	} else {
		ret.value.rvalue=num_rvalue(a)*num_rvalue(b);
	}
	return ret;
}

static number_t num_div(number_t a, number_t b) {
	number_t ret;
	ret.is_integer=a.is_integer && b.is_integer && a.value.ivalue%b.value.ivalue==0;
	if(ret.is_integer) {
		ret.value.ivalue= a.value.ivalue/b.value.ivalue;
	} else {
		ret.value.rvalue=num_rvalue(a)/num_rvalue(b);
	}
	return ret;
}

static number_t num_intdiv(number_t a, number_t b) {
	number_t ret;
	ret.is_integer=a.is_integer && b.is_integer;
	if(ret.is_integer) {
		ret.value.ivalue= a.value.ivalue/b.value.ivalue;
	} else {
		ret.value.rvalue=num_rvalue(a)/num_rvalue(b);
	}
	return ret;
}

static number_t num_sub(number_t a, number_t b) {
	number_t ret;
	ret.is_integer=a.is_integer && b.is_integer;
	if(ret.is_integer) {
		ret.value.ivalue= a.value.ivalue-b.value.ivalue;
	} else {
		ret.value.rvalue=num_rvalue(a)-num_rvalue(b);
	}
	return ret;
}

static number_t num_rem(number_t a, number_t b) {
	number_t ret;
	long e1, e2, res;
	ret.is_integer=a.is_integer && b.is_integer;
	e1=num_ivalue(a);
	e2=num_ivalue(b);
	res=e1%e2;
	/* remainder should have same sign as second operand */
	if (res > 0) {
		if (e1 < 0) {
			res -= labs(e2);
		}
	} else if (res < 0) {
		if (e1 > 0) {
			res += labs(e2);
		}
	}
	ret.value.ivalue=res;
	return ret;
}

static number_t num_mod(number_t a, number_t b) {
	number_t ret;
	long e1, e2, res;
	ret.is_integer=a.is_integer && b.is_integer;
	e1=num_ivalue(a);
	e2=num_ivalue(b);
	res=e1%e2;
	/* modulo should have same sign as second operand */
	if (res * e2 < 0) {
		res += e2;
	}
	ret.value.ivalue=res;
	return ret;
}

#if USE_MATH
/* Round to nearest. Round to even if midway */
static double round_per_R5RS(double x) {
	double fl=floor(x);
	double ce=ceil(x);
	double dfl=x-fl;
	double dce=ce-x;
	if(dfl>dce) {
		return ce;
	} else if(dfl<dce) {
		return fl;
	} else {
		if(fmod(fl,2.0)==0.0) {       /* I imagine this holds */
			return fl;
		} else {
			return ce;
		}
	}
}
#endif

static int is_zero_double(double x) {
	return x<DBL_MIN && x>-DBL_MIN;
}

long binary_decode(const char *s) {
	long x=0;

	while(*s!=0 && (*s=='1' || *s=='0')) {
		x<<=1;
		x+=*s-'0';
		s++;
	}

	return x;
}

static cell_ptr_t mk_number(scheme_t *sc, number_t n) {
	if(n.is_integer) {
		return mk_integer(sc,n.value.ivalue);
	} else {
		return mk_real(sc,n.value.rvalue);
	}
}

cell_ptr_t op_number(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t x;
	number_t v;
#if USE_MATH
	double dd;
#endif

	switch (op) {
#if USE_MATH
	case OP_INEX2EX:    /* inexact->exact */
		x=car(sc->args);
		if(num_is_integer(x)) {
			s_return(sc,x);
		} else if(modf(rvalue_unchecked(x),&dd)==0.0) {
			s_return(sc,mk_integer(sc,ivalue(x)));
		} else {
			error_1(sc,"inexact->exact: not integral:",x);
		}

	case OP_EXP:
		x=car(sc->args);
		s_return(sc, mk_real(sc, exp(rvalue(x))));

	case OP_LOG:
		x=car(sc->args);
		s_return(sc, mk_real(sc, log(rvalue(x))));

	case OP_SIN:
		x=car(sc->args);
		s_return(sc, mk_real(sc, sin(rvalue(x))));

	case OP_COS:
		x=car(sc->args);
		s_return(sc, mk_real(sc, cos(rvalue(x))));

	case OP_TAN:
		x=car(sc->args);
		s_return(sc, mk_real(sc, tan(rvalue(x))));

	case OP_ASIN:
		x=car(sc->args);
		s_return(sc, mk_real(sc, asin(rvalue(x))));

	case OP_ACOS:
		x=car(sc->args);
		s_return(sc, mk_real(sc, acos(rvalue(x))));

	case OP_ATAN:
		x=car(sc->args);
		if(cdr(sc->args)==sc->NIL) {
			s_return(sc, mk_real(sc, atan(rvalue(x))));
		} else {
			cell_ptr_t y=cadr(sc->args);
			s_return(sc, mk_real(sc, atan2(rvalue(x),rvalue(y))));
		}

	case OP_SQRT:
		x=car(sc->args);
		s_return(sc, mk_real(sc, sqrt(rvalue(x))));

	case OP_EXPT: {
		double result;
		int real_result=1;
		cell_ptr_t y=cadr(sc->args);
		x=car(sc->args);
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
		if (!real_result) {
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
		x=car(sc->args);
		s_return(sc, mk_real(sc, floor(rvalue(x))));

	case OP_CEILING:
		x=car(sc->args);
		s_return(sc, mk_real(sc, ceil(rvalue(x))));

	case OP_TRUNCATE : {
		double rvalue_of_x ;
		x=car(sc->args);
		rvalue_of_x = rvalue(x) ;
		if (rvalue_of_x > 0) {
			s_return(sc, mk_real(sc, floor(rvalue_of_x)));
		} else {
			s_return(sc, mk_real(sc, ceil(rvalue_of_x)));
		}
	}

	case OP_ROUND:
		x=car(sc->args);
		if (num_is_integer(x))
			s_return(sc, x);
		s_return(sc, mk_real(sc, round_per_R5RS(rvalue(x))));
#endif

	case OP_ADD:        /* + */
		v=__s_num_zero;
		for (x = sc->args; x != sc->NIL; x = cdr(x)) {
			v=num_add(v,nvalue(car(x)));
		}
		s_return(sc,mk_number(sc, v));

	case OP_MUL:        /* * */
		v=__s_num_one;
		for (x = sc->args; x != sc->NIL; x = cdr(x)) {
			v=num_mul(v,nvalue(car(x)));
		}
		s_return(sc,mk_number(sc, v));

	case OP_SUB:        /* - */
		if(cdr(sc->args)==sc->NIL) {
			x	= sc->args;
			v	= __s_num_zero;
		} else {
			x	= cdr(sc->args);
			v	= nvalue(car(sc->args));
		}
		for (; x != sc->NIL; x = cdr(x)) {
			v=num_sub(v,nvalue(car(x)));
		}
		s_return(sc,mk_number(sc, v));

	case OP_DIV:        /* / */
		if(cdr(sc->args)==sc->NIL) {
			x	= sc->args;
			v	= __s_num_one;
		} else {
			x = cdr(sc->args);
			v = nvalue(car(sc->args));
		}
		for (; x != sc->NIL; x = cdr(x)) {
			if (!is_zero_double(rvalue(car(x))))
				v=num_div(v,nvalue(car(x)));
			else {
				error_0(sc,"/: division by zero");
			}
		}
		s_return(sc,mk_number(sc, v));

	case OP_INTDIV:        /* quotient */
		if(cdr(sc->args)==sc->NIL) {
			x	= sc->args;
			v	= __s_num_one;
		} else {
			x = cdr(sc->args);
			v = nvalue(car(sc->args));
		}
		for (; x != sc->NIL; x = cdr(x)) {
			if (ivalue(car(x)) != 0)
				v=num_intdiv(v,nvalue(car(x)));
			else {
				error_0(sc,"quotient: division by zero");
			}
		}
		s_return(sc,mk_number(sc, v));

	case OP_REM:        /* remainder */
		v = nvalue(car(sc->args));
		if (ivalue(cadr(sc->args)) != 0)
			v=num_rem(v,nvalue(cadr(sc->args)));
		else {
			error_0(sc,"remainder: division by zero");
		}
		s_return(sc,mk_number(sc, v));

	case OP_MOD:        /* modulo */
		v = nvalue(car(sc->args));
		if (ivalue(cadr(sc->args)) != 0)
			v=num_mod(v,nvalue(cadr(sc->args)));
		else {
			error_0(sc,"modulo: division by zero");
		}
		s_return(sc,mk_number(sc, v));

	default:
		break;
	}

	return sc->T;
}

