#include "scheme-private.h"


port_t *port_rep_from_filename(scheme_t *sc, const char *fn, int prop) {
	FILE *f;
	char *rw;
	port_t *pt;
	if(prop==(port_input|port_output)) {
		rw="a+";
	} else if(prop==port_output) {
		rw="w";
	} else {
		rw="r";
	}
	f=fopen(fn,rw);
	if(f==0) {
		return 0;
	}
	pt=port_rep_from_file(sc,f,prop);
	pt->rep.stdio.closeit=1;

#if SHOW_ERROR_LINE
	if(fn)
		pt->rep.stdio.filename = store_string(sc, strlen(fn), fn, 0);

	pt->rep.stdio.curr_line = 0;
#endif
	return pt;
}

cell_ptr_t port_from_filename(scheme_t *sc, const char *fn, int prop) {
	port_t *pt;
	pt=port_rep_from_filename(sc,fn,prop);
	if(pt==0) {
		return sc->syms.NIL;
	}
	return mk_port(sc,pt);
}

port_t*
port_rep_from_file(scheme_t *sc,
		   FILE *f,
		   int prop)
{
	port_t* pt	= (port_t*)sc->malloc(sizeof *pt);
	if (pt == NULL) {
		return NULL;
	}

	pt->kind.value		= port_file | prop;
	pt->rep.stdio.file	= f;
	pt->rep.stdio.closeit	= 0;
	return pt;
}

cell_ptr_t
port_from_file(scheme_t *sc,
	       FILE *f,
	       int prop)
{
	port_t* pt = port_rep_from_file(sc, f, prop);

	if( pt == 0 ) {
		return sc->syms.NIL;
	}

	return mk_port(sc, pt);
}

port_t*
port_rep_from_string(scheme_t *sc,
		     char *start,
		     char *past_the_end,
		     int prop)
{
	port_t*	pt =(port_t*)sc->malloc(sizeof(port_t));
	if( pt == 0 ) {
		return 0;
	}
	pt->kind.value	= port_string | prop;
	pt->rep.string.start	= start;
	pt->rep.string.curr	= start;
	pt->rep.string.past_the_end	= past_the_end;
	return pt;
}

cell_ptr_t
port_from_string(scheme_t *sc,
		 char *start,
		 char *past_the_end,
		 int prop)
{
	port_t* pt	= port_rep_from_string(sc, start, past_the_end, prop);
	if( pt == 0 ) {
		return sc->syms.NIL;
	}
	return mk_port(sc, pt);
}

#define BLOCK_SIZE 256

port_t*
port_rep_from_scratch(scheme_t *sc)
{
	port_t*	pt	= (port_t*)sc->malloc(sizeof(port_t));

	char *start;

	if( pt == 0 ) {
		return 0;
	}

	start	= sc->malloc(BLOCK_SIZE);

	if( start == 0 ) {
		return 0;
	}

	memset(start, ' ', BLOCK_SIZE - 1);
	start[BLOCK_SIZE-1]	= '\0';
	pt->kind.value	= port_string | port_output | port_srfi6;
	pt->rep.string.start	= start;
	pt->rep.string.curr	= start;
	pt->rep.string.past_the_end	= start + BLOCK_SIZE - 1;
	return pt;
}

cell_ptr_t
port_from_scratch(scheme_t *sc)
{
	port_t*	pt = port_rep_from_scratch(sc);
	if( pt == 0 ) {
		return sc->syms.NIL;
	}
	return mk_port(sc, pt);
}

void
port_close(scheme_t *sc,
	   cell_ptr_t p,
	   int flag)
{
	port_t*	pt	= p->object.port;
	pt->kind.value	&= ~flag;
	if( (pt->kind.value & (port_input | port_output)) == 0 ) {
		if( pt->kind.value & port_file ) {

#if SHOW_ERROR_LINE
			/* Cleanup is here so (close-*-port) functions could work too */
			pt->rep.stdio.curr_line = 0;

			if(pt->rep.stdio.filename) {
				sc->free(pt->rep.stdio.filename);
			}
#endif

			fclose(pt->rep.stdio.file);
		}

		pt->kind.value = port_free;
	}
}

/* get new character from input file */
int
inchar(scheme_t *sc)
{
	int c;
	port_t *pt;

	pt = sc->inport->object.port;

	if( pt->kind.value & port_saw_EOF ) {
		return EOF;
	}

	c = basic_inchar(pt);
	if( c == EOF && sc->inport == sc->loadport ) {
		/* Instead, set port_saw_EOF */
		pt->kind.value |= port_saw_EOF;

		/* file_pop(sc); */
		return EOF;
		/* NOTREACHED */
	}

	return c;
}

int basic_inchar(port_t *pt) {
	if(pt->kind.value & port_file) {
		return fgetc(pt->rep.stdio.file);
	} else {
		if(*pt->rep.string.curr == 0 ||
				pt->rep.string.curr == pt->rep.string.past_the_end) {
			return EOF;
		} else {
			return *pt->rep.string.curr++;
		}
	}
}

/* back character to input buffer */
void
backchar(scheme_t *sc,
	 int c)
{
	port_t *pt;
	if( c == EOF ) {
		return;
	}

	pt	= sc->inport->object.port;

	if( pt->kind.value & port_file ) {
		ungetc(c, pt->rep.stdio.file);
	} else {
		if( pt->rep.string.curr != pt->rep.string.start ) {
			--pt->rep.string.curr;
		}
	}
}

int
realloc_port_string(scheme_t *sc,
		    port_t *p)
{
	char *start	= p->rep.string.start;
	size_t new_size	= p->rep.string.past_the_end - start + 1 + BLOCK_SIZE;
	char *str	= sc->malloc(new_size);
	if(str) {
		memset(str, ' ', new_size - 1);
		str[new_size - 1]	= '\0';
		strcpy(str, start);
		p->rep.string.start	= str;
		p->rep.string.past_the_end	= str + new_size - 1;
		p->rep.string.curr	-= start - str;
		sc->free(start);
		return 1;
	} else {
		return 0;
	}
}

void
putstr(scheme_t *sc,
       const char *s)
{
	port_t *pt	= sc->outport->object.port;
	if( pt->kind.value & port_file ) {
		fputs(s, pt->rep.stdio.file);
	} else {
		for(; *s; s++) {
			if( pt->rep.string.curr!=pt->rep.string.past_the_end ) {
				*pt->rep.string.curr++	= *s;
			} else if( pt->kind.value & port_srfi6 && realloc_port_string(sc, pt) ) {
				*pt->rep.string.curr++	= *s;
			}
		}
	}
}

void
putchars(scheme_t *sc,
	 const char *s,
	 int len)
{
	port_t *pt	= sc->outport->object.port;
	if( pt->kind.value & port_file ) {
		fwrite(s, 1, len, pt->rep.stdio.file);
	} else {
		for(; len; len--) {
			if( pt->rep.string.curr != pt->rep.string.past_the_end ) {
				*pt->rep.string.curr++	= *s++;
			} else if( pt->kind.value & port_srfi6 && realloc_port_string(sc, pt) ) {
				*pt->rep.string.curr++	= *s++;
			}
		}
	}
}

void
putcharacter(scheme_t *sc,
	     int c)
{
	port_t *pt	= sc->outport->object.port;
	if( pt->kind.value & port_file ) {
		fputc(c, pt->rep.stdio.file);
	} else {
		if( pt->rep.string.curr != pt->rep.string.past_the_end ) {
			*pt->rep.string.curr++	= c;
		} else if( pt->kind.value & port_srfi6 && realloc_port_string(sc, pt) ) {
			*pt->rep.string.curr++	= c;
		}
	}
}

