#include "scheme-private.h"

#define NUMBER_SOURCE
#include "number.h"

long binary_decode(const char *s) {
	long x=0;

	while(*s!=0 && (*s=='1' || *s=='0')) {
		x<<=1;
		x+=*s-'0';
		s++;
	}

	return x;
}

