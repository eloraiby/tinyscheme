#include "scheme-private.h"


port_t *port_rep_from_string(scheme_t *sc, char *start, char *past_the_end, int prop) {
	port_t *pt;
	pt=(port_t*)sc->malloc(sizeof(port_t));
	if(pt==0) {
		return 0;
	}
	pt->kind=port_string | prop;
	pt->string.start=start;
	pt->string.curr=start;
	pt->string.past_the_end=past_the_end;
	return pt;
}

cell_ptr_t port_from_string(scheme_t *sc, char *start, char *past_the_end, int prop) {
	port_t *pt;
	pt=port_rep_from_string(sc,start,past_the_end,prop);
	if(pt==0) {
		return sc->NIL;
	}
	return mk_port(sc,pt);
}

#define BLOCK_SIZE 256

port_t *port_rep_from_scratch(scheme_t *sc) {
	port_t *pt;
	char *start;
	pt=(port_t*)sc->malloc(sizeof(port_t));
	if(pt==0) {
		return 0;
	}
	start=sc->malloc(BLOCK_SIZE);
	if(start==0) {
		return 0;
	}
	memset(start,' ',BLOCK_SIZE-1);
	start[BLOCK_SIZE-1]='\0';
	pt->kind=port_string|port_output|port_srfi6;
	pt->string.start=start;
	pt->string.curr=start;
	pt->string.past_the_end=start+BLOCK_SIZE-1;
	return pt;
}

cell_ptr_t port_from_scratch(scheme_t *sc) {
	port_t *pt;
	pt=port_rep_from_scratch(sc);
	if(pt==0) {
		return sc->NIL;
	}
	return mk_port(sc,pt);
}

void port_close(scheme_t *sc, cell_ptr_t p, int flag) {
	port_t *pt=p->_object._port;
	pt->kind&=~flag;
	if((pt->kind & (port_input|port_output))==0) {
		pt->kind=port_free;
	}
}

/* get new character from input file */
int inchar(scheme_t *sc) {
	int c;
	port_t *pt;

	pt = sc->inport->_object._port;
	if(pt->kind & port_saw_EOF) {
		return EOF;
	}
	c = basic_inchar(pt);
	if(c == EOF && sc->inport == sc->loadport) {
		/* Instead, set port_saw_EOF */
		pt->kind |= port_saw_EOF;

		/* file_pop(sc); */
		return EOF;
		/* NOTREACHED */
	}
	return c;
}

int basic_inchar(port_t *pt) {
	if(*pt->string.curr == 0 || pt->string.curr == pt->string.past_the_end) {
		return EOF;
	} else {
		return *pt->string.curr++;
	}
}

/* back character to input buffer */
void backchar(scheme_t *sc, int c) {
	port_t *pt;
	if(c==EOF) return;
	pt=sc->inport->_object._port;
	if(pt->string.curr != pt->string.start) {
		--pt->string.curr;
	}
}

int realloc_port_string(scheme_t *sc, port_t *p) {
	char *start=p->string.start;
	size_t new_size=p->string.past_the_end-start+1+BLOCK_SIZE;
	char *str=sc->malloc(new_size);
	if(str) {
		memset(str,' ',new_size-1);
		str[new_size-1]='\0';
		strcpy(str,start);
		p->string.start=str;
		p->string.past_the_end=str+new_size-1;
		p->string.curr-=start-str;
		sc->free(start);
		return 1;
	} else {
		return 0;
	}
}

void putstr(scheme_t *sc, const char *s) {
	port_t *pt=sc->outport->_object._port;
	for(; *s; s++) {
		if(pt->string.curr!=pt->string.past_the_end) {
			*pt->string.curr++=*s;
		} else if((pt->kind & port_srfi6) && realloc_port_string(sc,pt)) {
			*pt->string.curr++=*s;
		}
	}
}

void putchars(scheme_t *sc, const char *s, int len) {
	port_t *pt=sc->outport->_object._port;
	for(; len; len--) {
		if(pt->string.curr!=pt->string.past_the_end) {
			*pt->string.curr++=*s++;
		} else if((pt->kind & port_srfi6) && realloc_port_string(sc,pt)) {
			*pt->string.curr++=*s++;
		}
	}
}

void putcharacter(scheme_t *sc, int c) {
	port_t *pt=sc->outport->_object._port;
	if(pt->string.curr!=pt->string.past_the_end) {
		*pt->string.curr++=c;
	} else if((pt->kind & port_srfi6) && realloc_port_string(sc,pt)) {
		*pt->string.curr++=c;
	}
}

int is_port(cell_ptr_t p) {
	return (type(p)==T_PORT);
}

int is_inport(cell_ptr_t p) {
	return is_port(p) && p->_object._port->kind & port_input;
}

int is_outport(cell_ptr_t p) {
	return is_port(p) && p->_object._port->kind & port_output;
}
