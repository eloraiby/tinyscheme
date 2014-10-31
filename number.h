#ifndef __NUMBER_H__
#define __NUMBER_H__
#include "scheme-private.h"

#include <stdlib.h>

#define num_ivalue(n)       (n.is_integer?(n).value.ivalue:(long)(n).value.rvalue)
#define num_rvalue(n)       (!n.is_integer?(n).value.rvalue:(double)(n).value.ivalue)

long binary_decode(const char *s);

#ifndef NUMBER_SOURCE

static number_t num_add(number_t a, number_t b);
static number_t num_mul(number_t a, number_t b);
static number_t num_div(number_t a, number_t b);
static number_t num_intdiv(number_t a, number_t b);
static number_t num_sub(number_t a, number_t b);
static number_t num_rem(number_t a, number_t b);
static number_t num_mod(number_t a, number_t b);
static int num_eq(number_t a, number_t b);
static int num_gt(number_t a, number_t b);
static int num_ge(number_t a, number_t b);
static int num_lt(number_t a, number_t b);
static int num_le(number_t a, number_t b);


#if USE_MATH
static double round_per_R5RS(double x);
#endif
static int is_zero_double(double x);
static INLINE int num_is_integer(cell_ptr_t p) {
	return ((p)->_object._number.is_integer);
}


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

static int num_eq(number_t a, number_t b) {
	int ret;
	int is_integer=a.is_integer && b.is_integer;
	if(is_integer) {
		ret= a.value.ivalue==b.value.ivalue;
	} else {
		ret=num_rvalue(a)==num_rvalue(b);
	}
	return ret;
}


static int num_gt(number_t a, number_t b) {
	int ret;
	int is_integer=a.is_integer && b.is_integer;
	if(is_integer) {
		ret= a.value.ivalue>b.value.ivalue;
	} else {
		ret=num_rvalue(a)>num_rvalue(b);
	}
	return ret;
}

static int num_ge(number_t a, number_t b) {
	return !num_lt(a,b);
}

static int num_lt(number_t a, number_t b) {
	int ret;
	int is_integer=a.is_integer && b.is_integer;
	if(is_integer) {
		ret= a.value.ivalue<b.value.ivalue;
	} else {
		ret=num_rvalue(a)<num_rvalue(b);
	}
	return ret;
}

static int num_le(number_t a, number_t b) {
	return !num_gt(a,b);
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

#endif /* NUMBER_SOURCE */
#endif /* __NUMBER_H__ */

