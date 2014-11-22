/* T I N Y S C H E M E    1 . 4 1
 *   Dimitrios Souflis (dsouflis@acm.org)
 *   Based on MiniScheme (original credits follow)
 * (MINISCM)               coded by Atsushi Moriwaki (11/5/1989)
 * (MINISCM)           E-MAIL :  moriwaki@kurims.kurims.kyoto-u.ac.jp
 * (MINISCM) This version has been modified by R.C. Secrist.
 * (MINISCM)
 * (MINISCM) Mini-Scheme is now maintained by Akira KIDA.
 * (MINISCM)
 * (MINISCM) This is a revised and modified version by Akira KIDA.
 * (MINISCM)    current version is 0.85k4 (15 May 1994)
 *
 */

#define _SCHEME_SOURCE
#include "scheme-private.h"
#ifndef WIN32
# include <unistd.h>
#endif
#ifdef WIN32
#define snprintf _snprintf
#endif
#if USE_DL
# include "dynload.h"
#endif
#include <ctype.h>

#if USE_STRCASECMP
#include <strings.h>
# ifndef __APPLE__
#  define stricmp strcasecmp
# endif
#endif

#define TOK_EOF     (-1)
#define TOK_LPAREN  0
#define TOK_RPAREN  1
#define TOK_DOT     2
#define TOK_ATOM    3
#define TOK_QUOTE   4
#define TOK_COMMENT 5
#define TOK_DQUOTE  6
#define TOK_BQUOTE  7
#define TOK_COMMA   8
#define TOK_ATMARK  9
#define TOK_SHARP   10
#define TOK_SHARP_CONST 11
#define TOK_VEC     12

#define BACKQUOTE '`'
#define DELIMITERS  "()\";\f\t\v\n\r "

/*
 *  Basic memory allocation units
 */

#define banner "TinyScheme 1.41"

#include <string.h>
#include <stdlib.h>

#ifdef __APPLE__
static int stricmp(const char *s1, const char *s2)
{
	unsigned char c1, c2;
	do {
		c1 = tolower(*s1);
		c2 = tolower(*s2);
		if (c1 < c2)
			return -1;
		else if (c1 > c2)
			return 1;
		s1++, s2++;
	} while (c1 != 0);
	return 0;
}
#endif /* __APPLE__ */

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

#ifndef prompt
# define prompt "ts> "
#endif

#ifndef InitFile
# define InitFile "init.scm"
#endif

#ifndef FIRST_CELLSEGS
# define FIRST_CELLSEGS 3
#endif


#if USE_CHAR_CLASSIFIERS
INLINE int Cisalpha(int c) { return isascii(c) && isalpha(c); }
INLINE int Cisdigit(int c) { return isascii(c) && isdigit(c); }
INLINE int Cisspace(int c) { return isascii(c) && isspace(c); }
INLINE int Cisupper(int c) { return isascii(c) && isupper(c); }
INLINE int Cislower(int c) { return isascii(c) && islower(c); }
#endif

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

static int file_push(scheme_t *sc, const char *fname);
static void file_pop(scheme_t *sc);
static int file_interactive(scheme_t *sc);
INLINE int is_one_of(char *s, int c);
static int alloc_cellseg(scheme_t *sc, int n);
static long binary_decode(const char *s);
INLINE cell_ptr_t get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
static cell_ptr_t _get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
static cell_ptr_t reserve_cells(scheme_t *sc, int n);
static cell_ptr_t get_consecutive_cells(scheme_t *sc, int n);
static cell_ptr_t find_consecutive_cells(scheme_t *sc, int n);
static void finalize_cell(scheme_t *sc, cell_ptr_t a);
static int count_consecutive_cells(cell_ptr_t x, int needed);
static cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t sym, int all);
static void mark(cell_ptr_t a);
static void gc(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
static int basic_inchar(port_t *pt);
static int inchar(scheme_t *sc);
static void backchar(scheme_t *sc, int c);
static char *readstr_upto(scheme_t *sc, char *delim);
static cell_ptr_t readstrexp(scheme_t *sc);
INLINE int skipspace(scheme_t *sc);
static int token(scheme_t *sc);
static void printslashstring(scheme_t *sc, char *s, int len);
static void atom2str(scheme_t *sc, cell_ptr_t l, int f, char **pp, int *plen);
static void printatom(scheme_t *sc, cell_ptr_t l, int f);
static void dump_stack_mark(scheme_t *);

static cell_ptr_t opexe_5(scheme_t *sc, enum scheme_opcodes op);
static void Eval_Cycle(scheme_t *sc, enum scheme_opcodes op);
static void assign_syntax(scheme_t *sc, char *name);
static void assign_proc(scheme_t *sc, enum scheme_opcodes, char *name);


static long binary_decode(const char *s) {
	long x=0;

	while(*s!=0 && (*s=='1' || *s=='0')) {
		x<<=1;
		x+=*s-'0';
		s++;
	}

	return x;
}

/* allocate new cell segment */
static int alloc_cellseg(scheme_t *sc, int n) {
	cell_ptr_t newp;
	cell_ptr_t last;
	cell_ptr_t p;
	char *cp;
	long i;
	int k;
	int adj=ADJ;

	if(adj<sizeof(cell_t)) {
		adj=sizeof(cell_t);
	}

	for (k = 0; k < n; k++) {
		if (sc->last_cell_seg >= CELL_NSEGMENT - 1)
			return k;
		cp = (char*) sc->malloc(CELL_SEGSIZE * sizeof(cell_t)+adj);
		if (cp == 0)
			return k;
		i = ++sc->last_cell_seg ;
		sc->alloc_seg[i] = cp;
		/* adjust in TYPE_BITS-bit boundary */
		if(((unsigned long)cp)%adj!=0) {
			cp=(char*)(adj*((unsigned long)cp/adj+1));
		}
		/* insert new segment in address order */
		newp=(cell_ptr_t)cp;
		sc->cell_seg[i] = newp;
		while (i > 0 && sc->cell_seg[i - 1] > sc->cell_seg[i]) {
			p = sc->cell_seg[i];
			sc->cell_seg[i] = sc->cell_seg[i - 1];
			sc->cell_seg[--i] = p;
		}
		sc->fcells += CELL_SEGSIZE;
		last = newp + CELL_SEGSIZE - 1;
		for (p = newp; p <= last; p++) {
			typeflag(p) = 0;
			cdr(p) = p + 1;
			car(p) = sc->NIL;
		}
		/* insert new cells in address order on free list */
		if (sc->free_cell == sc->NIL || p < sc->free_cell) {
			cdr(last) = sc->free_cell;
			sc->free_cell = newp;
		} else {
			p = sc->free_cell;
			while (cdr(p) != sc->NIL && newp > cdr(p))
				p = cdr(p);
			cdr(last) = cdr(p);
			cdr(p) = newp;
		}
	}
	return n;
}

INLINE cell_ptr_t get_cell_x(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	if (sc->free_cell != sc->NIL) {
		cell_ptr_t x = sc->free_cell;
		sc->free_cell = cdr(x);
		--sc->fcells;
		return (x);
	}
	return _get_cell (sc, a, b);
}


/* get new cell.  parameter a, b is marked by gc. */
static cell_ptr_t _get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	cell_ptr_t x;

	if(sc->no_memory) {
		return sc->sink;
	}

	if (sc->free_cell == sc->NIL) {
		const int min_to_be_recovered = sc->last_cell_seg*8;
		gc(sc,a, b);
		if (sc->fcells < min_to_be_recovered
				|| sc->free_cell == sc->NIL) {
			/* if only a few recovered, get more to avoid fruitless gc's */
			if (!alloc_cellseg(sc,1) && sc->free_cell == sc->NIL) {
				sc->no_memory=1;
				return sc->sink;
			}
		}
	}
	x = sc->free_cell;
	sc->free_cell = cdr(x);
	--sc->fcells;
	return (x);
}

/* make sure that there is a given number of cells free */
static cell_ptr_t reserve_cells(scheme_t *sc, int n) {
	if(sc->no_memory) {
		return sc->NIL;
	}

	/* Are there enough cells available? */
	if (sc->fcells < n) {
		/* If not, try gc'ing some */
		gc(sc, sc->NIL, sc->NIL);
		if (sc->fcells < n) {
			/* If there still aren't, try getting more heap */
			if (!alloc_cellseg(sc,1)) {
				sc->no_memory=1;
				return sc->NIL;
			}
		}
		if (sc->fcells < n) {
			/* If all fail, report failure */
			sc->no_memory=1;
			return sc->NIL;
		}
	}
	return (sc->T);
}

static cell_ptr_t get_consecutive_cells(scheme_t *sc, int n) {
	cell_ptr_t x;

	if(sc->no_memory) { return sc->sink; }

	/* Are there any cells available? */
	x=find_consecutive_cells(sc,n);
	if (x != sc->NIL) { return x; }

	/* If not, try gc'ing some */
	gc(sc, sc->NIL, sc->NIL);
	x=find_consecutive_cells(sc,n);
	if (x != sc->NIL) { return x; }

	/* If there still aren't, try getting more heap */
	if (!alloc_cellseg(sc,1))
	{
		sc->no_memory=1;
		return sc->sink;
	}

	x=find_consecutive_cells(sc,n);
	if (x != sc->NIL) { return x; }

	/* If all fail, report failure */
	sc->no_memory=1;
	return sc->sink;
}

static int count_consecutive_cells(cell_ptr_t x, int needed) {
	int n=1;
	while(cdr(x)==x+1) {
		x=cdr(x);
		n++;
		if(n>needed) return n;
	}
	return n;
}

static cell_ptr_t find_consecutive_cells(scheme_t *sc, int n) {
	cell_ptr_t *pp;
	int cnt;

	pp=&sc->free_cell;
	while(*pp!=sc->NIL) {
		cnt=count_consecutive_cells(*pp,n);
		if(cnt>=n) {
			cell_ptr_t x=*pp;
			*pp=cdr(*pp+n-1);
			sc->fcells -= n;
			return x;
		}
		pp=&cdr(*pp+cnt-1);
	}
	return sc->NIL;
}

/* To retain recent allocs before interpreter knows about them -
   Tehom */

static void push_recent_alloc(scheme_t *sc, cell_ptr_t recent, cell_ptr_t extra)
{
	cell_ptr_t holder = get_cell_x(sc, recent, extra);
	typeflag(holder) = T_PAIR | T_IMMUTABLE;
	car(holder) = recent;
	cdr(holder) = car(sc->sink);
	car(sc->sink) = holder;
}


static cell_ptr_t get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b)
{
	cell_ptr_t cell   = get_cell_x(sc, a, b);
	/* For right now, include "a" and "b" in "cell" so that gc doesn't
     think they are garbage. */
	/* Tentatively record it as a pair so gc understands it. */
	typeflag(cell) = T_PAIR;
	car(cell) = a;
	cdr(cell) = b;
	push_recent_alloc(sc, cell, sc->NIL);
	return cell;
}

static cell_ptr_t get_vector_object(scheme_t *sc, int len, cell_ptr_t init)
{
	cell_ptr_t cells = get_consecutive_cells(sc,len/2+len%2+1);
	if(sc->no_memory) { return sc->sink; }
	/* Record it as a vector so that gc understands it. */
	typeflag(cells) = (T_VECTOR | T_ATOM);
	ivalue_unchecked(cells)=len;
	set_num_integer(cells);
	fill_vector(cells,init);
	push_recent_alloc(sc, cells, sc->NIL);
	return cells;
}

INLINE void ok_to_freely_gc(scheme_t *sc)
{
	car(sc->sink) = sc->NIL;
}


#if defined TSGRIND
static void check_cell_alloced(cell_ptr_t p, int expect_alloced)
{
	/* Can't use putstr(sc,str) because callers have no access to
     sc.  */
	if(typeflag(p) & !expect_alloced)
	{
		fprintf(stderr,"Cell is already allocated!\n");
	}
	if(!(typeflag(p)) & expect_alloced)
	{
		fprintf(stderr,"Cell is not allocated!\n");
	}

}
static void check_range_alloced(cell_ptr_t p, int n, int expect_alloced)
{
	int i;
	for(i = 0;i<n;i++)
	{ (void)check_cell_alloced(p+i,expect_alloced); }
}

#endif

/* Medium level cell allocation */

/* get new cons cell */
cell_ptr_t _cons(scheme_t *sc, cell_ptr_t a, cell_ptr_t b, int immutable) {
	cell_ptr_t x = get_cell(sc,a, b);

	typeflag(x) = T_PAIR;
	if(immutable) {
		setimmutable(x);
	}
	car(x) = a;
	cdr(x) = b;
	return (x);
}

/* ========== oblist implementation  ========== */

#ifndef USE_OBJECT_LIST

static int hash_fn(const char *key, int table_size);

static cell_ptr_t oblist_initial_value(scheme_t *sc)
{
	return mk_vector(sc, 461); /* probably should be bigger */
}

/* returns the new symbol */
static cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name)
{
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

INLINE cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name)
{
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

static cell_ptr_t oblist_all_symbols(scheme_t *sc)
{
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

static cell_ptr_t oblist_initial_value(scheme_t *sc)
{
	return sc->NIL;
}

static INLINE cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name)
{
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
static cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name)
{
	cell_ptr_t x;

	x = immutable_cons(sc, mk_string(sc, name), sc->NIL);
	typeflag(x) = T_SYMBOL;
	setimmutable(car(x));
	sc->oblist = immutable_cons(sc, x, sc->oblist);
	return x;
}
static cell_ptr_t oblist_all_symbols(scheme_t *sc)
{
	return sc->oblist;
}

#endif


/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
static void mark(cell_ptr_t a) {
	cell_ptr_t t, q, p;

	t = (cell_ptr_t) 0;
	p = a;
E2:  setmark(p);
	if(is_vector(p)) {
		int i;
		int number_t=ivalue_unchecked(p)/2+ivalue_unchecked(p)%2;
		for(i=0; i<number_t; i++) {
			/* Vector cells will be treated like ordinary cells */
			mark(p+1+i);
		}
	}
	if (is_atom(p))
		goto E6;
	/* E4: down car */
	q = car(p);
	if (q && !is_mark(q)) {
		setatom(p);  /* a note that we have moved car */
		car(p) = t;
		t = p;
		p = q;
		goto E2;
	}
E5:  q = cdr(p); /* down cdr */
	if (q && !is_mark(q)) {
		cdr(p) = t;
		t = p;
		p = q;
		goto E2;
	}
E6:   /* up.  Undo the link switching from steps E4 and E5. */
	if (!t)
		return;
	q = t;
	if (is_atom(q)) {
		clratom(q);
		t = car(q);
		car(q) = p;
		p = q;
		goto E5;
	} else {
		t = cdr(q);
		cdr(q) = p;
		p = q;
		goto E6;
	}
}

/* garbage collection. parameter a, b is marked. */
static void gc(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	cell_ptr_t p;
	int i;

	if(sc->gc_verbose) {
		putstr(sc, "gc...");
	}

	/* mark system globals */
	mark(sc->oblist);
	mark(sc->global_env);

	/* mark current registers */
	mark(sc->args);
	mark(sc->envir);
	mark(sc->code);
	dump_stack_mark(sc);
	mark(sc->value);
	mark(sc->inport);
	mark(sc->save_inport);
	mark(sc->outport);
	mark(sc->loadport);

	/* Mark recent objects the interpreter doesn't know about yet. */
	mark(car(sc->sink));
	/* Mark any older stuff above nested C calls */
	mark(sc->c_nest);

	/* mark variables a, b */
	mark(a);
	mark(b);

	/* garbage collect */
	clrmark(sc->NIL);
	sc->fcells = 0;
	sc->free_cell = sc->NIL;
	/* free-list is kept sorted by address so as to maintain consecutive
     ranges, if possible, for use with vectors. Here we scan the cells
     (which are also kept sorted by address) downwards to build the
     free-list in sorted order.
  */
	for (i = sc->last_cell_seg; i >= 0; i--) {
		p = sc->cell_seg[i] + CELL_SEGSIZE;
		while (--p >= sc->cell_seg[i]) {
			if (is_mark(p)) {
				clrmark(p);
			} else {
				/* reclaim cell */
				if (typeflag(p) != 0) {
					finalize_cell(sc, p);
					typeflag(p) = 0;
					car(p) = sc->NIL;
				}
				++sc->fcells;
				cdr(p) = sc->free_cell;
				sc->free_cell = p;
			}
		}
	}

	if (sc->gc_verbose) {
		char msg[80];
		snprintf(msg,80,"done: %ld cells were recovered.\n", sc->fcells);
		putstr(sc,msg);
	}
}

static void finalize_cell(scheme_t *sc, cell_ptr_t a) {
	if(is_string(a)) {
		sc->free(strvalue(a));
	} else if(is_port(a)) {
		if(a->_object._port->kind&port_file
				&& a->_object._port->rep.stdio.closeit) {
			port_close(sc,a,port_input|port_output);
		}
		sc->free(a->_object._port);
	}
}

/* ========== Routines for Reading ========== */

static int file_push(scheme_t *sc, const char *fname) {
	FILE *fin = NULL;

	if (sc->file_i == MAXFIL-1)
		return 0;
	fin=fopen(fname,"r");
	if(fin!=0) {
		sc->file_i++;
		sc->load_stack[sc->file_i].kind=port_file|port_input;
		sc->load_stack[sc->file_i].rep.stdio.file=fin;
		sc->load_stack[sc->file_i].rep.stdio.closeit=1;
		sc->nesting_stack[sc->file_i]=0;
		sc->loadport->_object._port=sc->load_stack+sc->file_i;

#if SHOW_ERROR_LINE
		sc->load_stack[sc->file_i].rep.stdio.curr_line = 0;
		if(fname)
			sc->load_stack[sc->file_i].rep.stdio.filename = store_string(sc, strlen(fname), fname, 0);
#endif
	}
	return fin!=0;
}

static void file_pop(scheme_t *sc) {
	if(sc->file_i != 0) {
		sc->nesting=sc->nesting_stack[sc->file_i];
		port_close(sc,sc->loadport,port_input);
		sc->file_i--;
		sc->loadport->_object._port=sc->load_stack+sc->file_i;
	}
}

static int file_interactive(scheme_t *sc) {
	return sc->file_i==0 && sc->load_stack[0].rep.stdio.file==stdin
			&& sc->inport->_object._port->kind&port_file;
}


/* read characters up to delimiter, but cater to character constants */
static char *readstr_upto(scheme_t *sc, char *delim) {
	char *p = sc->strbuff;

	while ((p - sc->strbuff < sizeof(sc->strbuff)) &&
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
		if(c == EOF || p-sc->strbuff > sizeof(sc->strbuff)-1) {
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
			if (c < '0' || c > '7')
			{
				*p++=c1;
				backchar(sc, c);
				state=st_ok;
			}
			else
			{
				if (state==st_oct2 && c1 >= 32)
					return sc->F;

				c1=(c1<<3)+(c-'0');

				if (state == st_oct1)
					state=st_oct2;
				else
				{
					*p++=c1;
					state=st_ok;
				}
			}
			break;

		}
	}
}

/* check c is in chars */
INLINE int is_one_of(char *s, int c) {
	if(c==EOF) return 1;
	while (*s)
		if (*s++ == c)
			return (1);
	return (0);
}

/* skip white characters */
INLINE int skipspace(scheme_t *sc) {
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
	}
	else
	{ return EOF; }
}

/* get token */
static int token(scheme_t *sc) {
	int c;
	c = skipspace(sc);
	if(c == EOF) { return (TOK_EOF); }
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

		if(c == EOF)
		{ return (TOK_EOF); }
		else
		{ return (token(sc));}
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

			if(c == EOF)
			{ return (TOK_EOF); }
			else
			{ return (token(sc));}
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

/* ========== Routines for Printing ========== */
#define   ok_abbrev(x)   (is_pair(x) && cdr(x) == sc->NIL)

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
static void printatom(scheme_t *sc, cell_ptr_t l, int f) {
	char *p;
	int len;
	atom2str(sc,l,f,&p,&len);
	putchars(sc,p,len);
}


/* Uses internal buffer unless string cell_ptr_t is already available */
static void atom2str(scheme_t *sc, cell_ptr_t l, int f, char **pp, int *plen) {
	char *p;

	if (l == sc->NIL) {
		p = "()";
	} else if (l == sc->T) {
		p = "#t";
	} else if (l == sc->F) {
		p = "#f";
	} else if (l == sc->EOF_OBJ) {
		p = "#<EOF>";
	} else if (is_port(l)) {
		p = sc->strbuff;
		snprintf(p, STRBUFFSIZE, "#<PORT>");
	} else if (is_number(l)) {
		p = sc->strbuff;
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
/* ========== Routines for Evaluation Cycle ========== */

static cell_ptr_t list_star(scheme_t *sc, cell_ptr_t d) {
	cell_ptr_t p, q;
	if(cdr(d)==sc->NIL) {
		return car(d);
	}
	p=cons(sc,car(d),cdr(d));
	q=p;
	while(cdr(cdr(p))!=sc->NIL) {
		d=cons(sc,car(p),cdr(p));
		if(cdr(cdr(p))!=sc->NIL) {
			p=cdr(d);
		}
	}
	cdr(p)=car(cdr(p));
	return q;
}

/* reverse list -- produce new list */
cell_ptr_t reverse(scheme_t *sc, cell_ptr_t a) {
	/* a must be checked by gc */
	cell_ptr_t p = sc->NIL;

	for ( ; is_pair(a); a = cdr(a)) {
		p = cons(sc, car(a), p);
	}
	return (p);
}

/* reverse list --- in-place */
cell_ptr_t reverse_in_place(scheme_t *sc, cell_ptr_t term, cell_ptr_t list) {
	cell_ptr_t p = list, result = term, q;

	while (p != sc->NIL) {
		q = cdr(p);
		cdr(p) = result;
		result = p;
		p = q;
	}
	return (result);
}

/* append list -- produce new list (in reverse order) */
cell_ptr_t revappend(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	cell_ptr_t result = a;
	cell_ptr_t p = b;

	while (is_pair(p)) {
		result = cons(sc, car(p), result);
		p = cdr(p);
	}

	if (p == sc->NIL) {
		return result;
	}

	return sc->F;   /* signal an error */
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


/* ========== Environment implementation  ========== */

#if !defined(USE_ALIST_ENV) || !defined(USE_OBJECT_LIST)

static int hash_fn(const char *key, int table_size)
{
	unsigned int hashed = 0;
	const char *c;
	int bits_per_int = sizeof(unsigned int)*8;

	for (c = key; *c; c++) {
		/* letters have about 5 bits in them */
		hashed = (hashed<<5) | (hashed>>(bits_per_int-5));
		hashed ^= *c;
	}
	return hashed % table_size;
}
#endif

#ifndef USE_ALIST_ENV

/*
 * In this implementation, each frame of the environment may be
 * a hash table: a vector of alists hashed by variable name.
 * In practice, we use a vector only for the initial frame;
 * subsequent frames are too small and transient for the lookup
 * speed to out-weigh the cost of making a new vector.
 */

static void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env)
{
	cell_ptr_t new_frame;

	/* The interaction-environment has about 300 variables in it. */
	if (old_env == sc->NIL) {
		new_frame = mk_vector(sc, 461);
	} else {
		new_frame = sc->NIL;
	}

	sc->envir = immutable_cons(sc, new_frame, old_env);
	setenvironment(sc->envir);
}

INLINE void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env,
					cell_ptr_t variable, cell_ptr_t value)
{
	cell_ptr_t slot = immutable_cons(sc, variable, value);

	if (is_vector(car(env))) {
		int location = hash_fn(symname(variable), ivalue_unchecked(car(env)));

		set_vector_elem(car(env), location,
				immutable_cons(sc, slot, vector_elem(car(env), location)));
	} else {
		car(env) = immutable_cons(sc, slot, car(env));
	}
}

static cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all)
{
	cell_ptr_t x,y;
	int location;

	for (x = env; x != sc->NIL; x = cdr(x)) {
		if (is_vector(car(x))) {
			location = hash_fn(symname(hdl), ivalue_unchecked(car(x)));
			y = vector_elem(car(x), location);
		} else {
			y = car(x);
		}
		for ( ; y != sc->NIL; y = cdr(y)) {
			if (caar(y) == hdl) {
				break;
			}
		}
		if (y != sc->NIL) {
			break;
		}
		if(!all) {
			return sc->NIL;
		}
	}
	if (x != sc->NIL) {
		return car(y);
	}
	return sc->NIL;
}

#else /* USE_ALIST_ENV */

static INLINE void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env)
{
	sc->envir = immutable_cons(sc, sc->NIL, old_env);
	setenvironment(sc->envir);
}

static INLINE void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env,
					cell_ptr_t variable, cell_ptr_t value)
{
	car(env) = immutable_cons(sc, immutable_cons(sc, variable, value), car(env));
}

static cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all)
{
	cell_ptr_t x,y;
	for (x = env; x != sc->NIL; x = cdr(x)) {
		for (y = car(x); y != sc->NIL; y = cdr(y)) {
			if (caar(y) == hdl) {
				break;
			}
		}
		if (y != sc->NIL) {
			break;
		}
		if(!all) {
			return sc->NIL;
		}
	}
	if (x != sc->NIL) {
		return car(y);
	}
	return sc->NIL;
}

#endif /* USE_ALIST_ENV else */

INLINE void new_slot_in_env(scheme_t *sc, cell_ptr_t variable, cell_ptr_t value)
{
	new_slot_spec_in_env(sc, sc->envir, variable, value);
}

INLINE void set_slot_in_env(scheme_t *sc, cell_ptr_t slot, cell_ptr_t value)
{
	cdr(slot) = value;
}

INLINE cell_ptr_t slot_value_in_env(cell_ptr_t slot)
{
	return cdr(slot);
}

/* ========== Evaluation Cycle ========== */


cell_ptr_t
_error_1(scheme_t *sc,
	 const char *s,
	 cell_ptr_t a)
{
	const char *str = s;
#if USE_ERROR_HOOK
	cell_ptr_t x;
	cell_ptr_t hdl=sc->ERROR_HOOK;
#endif

#if SHOW_ERROR_LINE
	char sbuf[STRBUFFSIZE];

	/* make sure error is not in REPL */
	if (sc->load_stack[sc->file_i].kind & port_file &&
			sc->load_stack[sc->file_i].rep.stdio.file != stdin) {
		int ln = sc->load_stack[sc->file_i].rep.stdio.curr_line;
		const char *fname = sc->load_stack[sc->file_i].rep.stdio.filename;

		/* should never happen */
		if(!fname) fname = "<unknown>";

		/* we started from 0 */
		ln++;
		snprintf(sbuf, STRBUFFSIZE, "(%s : %i) %s", fname, ln, s);

		str = (const char*)sbuf;
	}
#endif

#if USE_ERROR_HOOK
	x=find_slot_in_env(sc,sc->envir,hdl,1);
	if (x != sc->NIL) {
		if(a!=0) {
			sc->code = cons(sc, cons(sc, sc->QUOTE, cons(sc,(a), sc->NIL)), sc->NIL);
		} else {
			sc->code = sc->NIL;
		}
		sc->code = cons(sc, mk_string(sc, str), sc->code);
		setimmutable(car(sc->code));
		sc->code = cons(sc, slot_value_in_env(x), sc->code);
		sc->op = (int)OP_EVAL;
		return sc->T;
	}
#endif

	if(a!=0) {
		sc->args = cons(sc, (a), sc->NIL);
	} else {
		sc->args = sc->NIL;
	}
	sc->args = cons(sc, mk_string(sc, str), sc->args);
	setimmutable(car(sc->args));
	sc->op = (int)OP_ERR0;
	return sc->T;
}


INLINE void dump_stack_reset(scheme_t *sc)
{
	sc->dump = sc->NIL;
}

INLINE void dump_stack_initialize(scheme_t *sc)
{
	dump_stack_reset(sc);
}

static void dump_stack_free(scheme_t *sc)
{
	sc->dump = sc->NIL;
}

cell_ptr_t _s_return(scheme_t *sc, cell_ptr_t a) {
	sc->value = (a);
	if(sc->dump==sc->NIL) return sc->NIL;
	sc->op = ivalue(car(sc->dump));
	sc->args = cadr(sc->dump);
	sc->envir = caddr(sc->dump);
	sc->code = cadddr(sc->dump);
	sc->dump = cddddr(sc->dump);
	return sc->T;
}

static void s_save(scheme_t *sc, enum scheme_opcodes op, cell_ptr_t args, cell_ptr_t code) {
	sc->dump = cons(sc, sc->envir, cons(sc, (code), sc->dump));
	sc->dump = cons(sc, (args), sc->dump);
	sc->dump = cons(sc, mk_integer(sc, (long)(op)), sc->dump);
}

INLINE void dump_stack_mark(scheme_t *sc)
{
	mark(sc->dump);
}



/* Result is:
   proper list: length
   circular list: -1
   not even a pair: -2
   dotted list: -2 minus length before dot
*/
int list_length(scheme_t *sc, cell_ptr_t a) {
	int i=0;
	cell_ptr_t slow, fast;

	slow = fast = a;
	while (1)
	{
		if (fast == sc->NIL)
			return i;
		if (!is_pair(fast))
			return -2 - i;
		fast = cdr(fast);
		++i;
		if (fast == sc->NIL)
			return i;
		if (!is_pair(fast))
			return -2 - i;
		++i;
		fast = cdr(fast);

		/* Safe because we would have already returned if `fast'
	   encountered a non-pair. */
		slow = cdr(slow);
		if (fast == slow)
		{
			/* the fast cell_ptr_t has looped back around and caught up
	       with the slow cell_ptr_t, hence the structure is circular,
	       not of finite length, and therefore not a list */
			return -1;
		}
	}
}

static cell_ptr_t opexe_5(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t	x;

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
		}
		else
		{
			sc->args = mk_integer(sc,sc->file_i);
			s_goto(sc,OP_T0LVL);
		}

	case OP_T0LVL: /* top level */
		/* If we reached the end of file, this loop is done. */
		if(sc->loadport->_object._port->kind & port_saw_EOF)
		{
			if(sc->file_i == 0)
			{
				sc->args=sc->NIL;
				s_goto(sc,OP_QUIT);
			}
			else
			{
				file_pop(sc);
				s_return(sc,sc->value);
			}
			/* NOTREACHED */
		}

		/* If interactive, be nice to user. */
		if(file_interactive(sc))
		{
			sc->envir = sc->global_env;
			dump_stack_reset(sc);
			putstr(sc,"\n");
			putstr(sc, prompt);
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
		if( sc->tok == TOK_EOF ){ s_return(sc, sc->EOF_OBJ); }
		s_goto(sc,OP_RDSEXPR);

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
		if (sc->tok == TOK_EOF)
		{ s_return(sc,sc->EOF_OBJ); }
		else if (sc->tok == TOK_RPAREN) {
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
	return sc->T; /* NOTREACHED */
}

typedef cell_ptr_t (*dispatch_func)(scheme_t *, enum scheme_opcodes);

typedef int (*test_predicate)(cell_ptr_t);
static int is_any(cell_ptr_t p) { return 1;}

static int is_nonneg(cell_ptr_t p) {
	return ivalue(p)>=0 && is_integer(p);
}

/* Correspond carefully with following defines! */
static struct {
	test_predicate fct;
	const char *kind;
} tests[]={
{0,0}, /* unused */
{is_any, 0},
{is_string, "string"},
{is_symbol, "symbol"},
{is_port, "port"},
{is_inport,"input port"},
{is_outport,"output port"},
{is_environment, "environment"},
{is_pair, "pair"},
{0, "pair or '()"},
{is_character, "character"},
{is_vector, "vector"},
{is_number, "number"},
{is_integer, "integer"},
{is_nonneg, "non-negative integer"}
};

#define TST_NONE 0
#define TST_ANY "\001"
#define TST_STRING "\002"
#define TST_SYMBOL "\003"
#define TST_PORT "\004"
#define TST_INPORT "\005"
#define TST_OUTPORT "\006"
#define TST_ENVIRONMENT "\007"
#define TST_PAIR "\010"
#define TST_LIST "\011"
#define TST_CHAR "\012"
#define TST_VECTOR "\013"
#define TST_NUMBER "\014"
#define TST_INTEGER "\015"
#define TST_NATURAL "\016"

typedef struct {
	dispatch_func func;
	char *name;
	int min_arity;
	int max_arity;
	char *arg_tests_encoding;
} op_code_info;

#define INF_ARG 0xffff

static op_code_info dispatch_table[]= {
	#define _OP_DEF(A,B,C,D,E,OP) {A,B,C,D,E},
	#include "opdefines.h"
	{ 0 }
};

const char *procname(cell_ptr_t x) {
	int n=procnum(x);
	const char *name=dispatch_table[n].name;
	if(name==0) {
		name="ILLEGAL!";
	}
	return name;
}

/* kernel of this interpreter */
static void Eval_Cycle(scheme_t *sc, enum scheme_opcodes op) {
	sc->op = op;
	for (;;) {
		op_code_info *pcd=dispatch_table+sc->op;
		if (pcd->name!=0) { /* if built-in function, check arguments */
			char msg[STRBUFFSIZE];
			int ok=1;
			int n=list_length(sc,sc->args);

			/* Check number of arguments */
			if(n<pcd->min_arity) {
				ok=0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
					 pcd->name,
					 pcd->min_arity==pcd->max_arity?"":" at least",
					 pcd->min_arity);
			}
			if(ok && n>pcd->max_arity) {
				ok=0;
				snprintf(msg, STRBUFFSIZE, "%s: needs%s %d argument(s)",
					 pcd->name,
					 pcd->min_arity==pcd->max_arity?"":" at most",
					 pcd->max_arity);
			}
			if(ok) {
				if(pcd->arg_tests_encoding!=0) {
					int i=0;
					int j;
					const char *t=pcd->arg_tests_encoding;
					cell_ptr_t arglist=sc->args;
					do {
						cell_ptr_t arg=car(arglist);
						j=(int)t[0];
						if(j==TST_LIST[0]) {
							if(arg!=sc->NIL && !is_pair(arg)) break;
						} else {
							if(!tests[j].fct(arg)) break;
						}

						if(t[1]!=0) {/* last test is replicated as necessary */
							t++;
						}
						arglist=cdr(arglist);
						i++;
					} while(i<n);
					if(i<n) {
						ok=0;
						snprintf(msg, STRBUFFSIZE, "%s: argument %d must be: %s",
							 pcd->name,
							 i+1,
							 tests[j].kind);
					}
				}
			}
			if(!ok) {
				if(_error_1(sc,msg,0)==sc->NIL) {
					return;
				}
				pcd=dispatch_table+sc->op;
			}
		}
		ok_to_freely_gc(sc);
		if (pcd->func(sc, (enum scheme_opcodes)sc->op) == sc->NIL) {
			return;
		}
		if(sc->no_memory) {
			fprintf(stderr,"No memory!\n");
			return;
		}
	}
}

/* ========== Initialization of internal keywords ========== */

static void assign_syntax(scheme_t *sc, char *name) {
	cell_ptr_t x;

	x = oblist_add_by_name(sc, name);
	typeflag(x) |= T_SYNTAX;
}

static void assign_proc(scheme_t *sc, enum scheme_opcodes op, char *name) {
	cell_ptr_t x, y;

	x = mk_symbol(sc, name);
	y = mk_proc(sc,op);
	new_slot_in_env(sc, x, y);
}

/* Hard-coded for the given keywords. Remember to rewrite if more are added! */
int syntaxnum(cell_ptr_t p) {
	const char *s=strvalue(car(p));
	switch(strlength(car(p))) {
	case 2:
		if(s[0]=='i') return OP_IF0;        /* if */
		else return OP_OR0;                 /* or */
	case 3:
		if(s[0]=='a') return OP_AND0;      /* and */
		else return OP_LET0;               /* let */
	case 4:
		switch(s[3]) {
		case 'e': return OP_CASE0;         /* case */
		case 'd': return OP_COND0;         /* cond */
		case '*': return OP_LET0AST;       /* let* */
		default: return OP_SET0;           /* set! */
		}
	case 5:
		switch(s[2]) {
		case 'g': return OP_BEGIN;         /* begin */
		case 'l': return OP_DELAY;         /* delay */
		case 'c': return OP_MACRO0;        /* macro */
		default: return OP_QUOTE;          /* quote */
		}
	case 6:
		switch(s[2]) {
		case 'm': return OP_LAMBDA;        /* lambda */
		case 'f': return OP_DEF0;          /* define */
		default: return OP_LET0REC;        /* letrec */
		}
	default:
		return OP_C0STREAM;                /* cons-stream */
	}
}

/* initialization of TinyScheme */
#if USE_INTERFACE
INTERFACE static cell_ptr_t s_cons(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	return cons(sc,a,b);
}
INTERFACE static cell_ptr_t s_immutable_cons(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	return immutable_cons(sc,a,b);
}

static struct scheme_interface vtbl ={
	scheme_define,
	s_cons,
	s_immutable_cons,
	reserve_cells,
	mk_integer,
	mk_real,
	mk_symbol,
	gensym,
	mk_string,
	mk_counted_string,
	mk_character,
	mk_vector,
	mk_foreign_func,
	putstr,
	putcharacter,

	is_string,
	string_value,
	is_number,
	nvalue,
	ivalue,
	rvalue,
	is_integer,
	is_real,
	is_character,
	charvalue,
	is_list,
	is_vector,
	list_length,
	ivalue,
	fill_vector,
	vector_elem,
	set_vector_elem,
	is_port,
	is_pair,
	pair_car,
	pair_cdr,
	set_car,
	set_cdr,

	is_symbol,
	symname,

	is_syntax,
	is_proc,
	is_foreign,
	syntaxname,
	is_closure,
	is_macro,
	closure_code,
	closure_env,

	is_continuation,
	is_promise,
	is_environment,
	is_immutable,
	setimmutable,

	scheme_load_file,
	scheme_load_string
};
#endif

scheme_t *scheme_init_new() {
	scheme_t *sc=(scheme_t*)malloc(sizeof(scheme_t));
	if(!scheme_init(sc)) {
		free(sc);
		return 0;
	} else {
		return sc;
	}
}

scheme_t *scheme_init_new_custom_alloc(func_alloc malloc, func_dealloc free) {
	scheme_t *sc=(scheme_t*)malloc(sizeof(scheme_t));
	if(!scheme_init_custom_alloc(sc,malloc,free)) {
		free(sc);
		return 0;
	} else {
		return sc;
	}
}


int scheme_init(scheme_t *sc) {
	return scheme_init_custom_alloc(sc,malloc,free);
}

int scheme_init_custom_alloc(scheme_t *sc, func_alloc malloc, func_dealloc free) {
	int i, n=sizeof(dispatch_table)/sizeof(dispatch_table[0]);
	cell_ptr_t x;

	sc->num_zero.is_integer		= 1;
	sc->num_zero.value.ivalue	= 0;
	sc->num_one.is_integer		= 1;
	sc->num_one.value.ivalue	= 1;

#if USE_INTERFACE
	sc->vptr=&vtbl;
#endif
	sc->gensym_cnt=0;
	sc->malloc=malloc;
	sc->free=free;
	sc->last_cell_seg = -1;
	sc->sink = &sc->_sink;
	sc->NIL = &sc->_NIL;
	sc->T = &sc->_HASHT;
	sc->F = &sc->_HASHF;
	sc->EOF_OBJ=&sc->_EOF_OBJ;
	sc->free_cell = &sc->_NIL;
	sc->fcells = 0;
	sc->no_memory=0;
	sc->inport=sc->NIL;
	sc->outport=sc->NIL;
	sc->save_inport=sc->NIL;
	sc->loadport=sc->NIL;
	sc->nesting=0;
	sc->interactive_repl=0;

	if (alloc_cellseg(sc,FIRST_CELLSEGS) != FIRST_CELLSEGS) {
		sc->no_memory=1;
		return 0;
	}
	sc->gc_verbose = 0;
	dump_stack_initialize(sc);
	sc->code = sc->NIL;
	sc->tracing=0;

	/* init sc->NIL */
	typeflag(sc->NIL) = (T_ATOM | MARK);
	car(sc->NIL) = cdr(sc->NIL) = sc->NIL;
	/* init T */
	typeflag(sc->T) = (T_ATOM | MARK);
	car(sc->T) = cdr(sc->T) = sc->T;
	/* init F */
	typeflag(sc->F) = (T_ATOM | MARK);
	car(sc->F) = cdr(sc->F) = sc->F;
	/* init sink */
	typeflag(sc->sink) = (T_PAIR | MARK);
	car(sc->sink) = sc->NIL;
	/* init c_nest */
	sc->c_nest = sc->NIL;

	sc->oblist = oblist_initial_value(sc);
	/* init global_env */
	new_frame_in_env(sc, sc->NIL);
	sc->global_env = sc->envir;
	/* init else */
	x = mk_symbol(sc,"else");
	new_slot_in_env(sc, x, sc->T);

	assign_syntax(sc, "lambda");
	assign_syntax(sc, "quote");
	assign_syntax(sc, "define");
	assign_syntax(sc, "if");
	assign_syntax(sc, "begin");
	assign_syntax(sc, "set!");
	assign_syntax(sc, "let");
	assign_syntax(sc, "let*");
	assign_syntax(sc, "letrec");
	assign_syntax(sc, "cond");
	assign_syntax(sc, "delay");
	assign_syntax(sc, "and");
	assign_syntax(sc, "or");
	assign_syntax(sc, "cons-stream");
	assign_syntax(sc, "macro");
	assign_syntax(sc, "case");

	for(i=0; i<n; i++) {
		if(dispatch_table[i].name!=0) {
			assign_proc(sc, (enum scheme_opcodes)i, dispatch_table[i].name);
		}
	}

	/* initialization of global pointers to special symbols */
	sc->LAMBDA = mk_symbol(sc, "lambda");
	sc->QUOTE = mk_symbol(sc, "quote");
	sc->QQUOTE = mk_symbol(sc, "quasiquote");
	sc->UNQUOTE = mk_symbol(sc, "unquote");
	sc->UNQUOTESP = mk_symbol(sc, "unquote-splicing");
	sc->FEED_TO = mk_symbol(sc, "=>");
	sc->COLON_HOOK = mk_symbol(sc,"*colon-hook*");
	sc->ERROR_HOOK = mk_symbol(sc, "*error-hook*");
	sc->SHARP_HOOK = mk_symbol(sc, "*sharp-hook*");
	sc->COMPILE_HOOK = mk_symbol(sc, "*compile-hook*");

	return !sc->no_memory;
}

void scheme_set_input_port_file(scheme_t *sc, FILE *fin) {
	sc->inport=port_from_file(sc,fin,port_input);
}

void scheme_set_input_port_string(scheme_t *sc, char *start, char *past_the_end) {
	sc->inport=port_from_string(sc,start,past_the_end,port_input);
}

void scheme_set_output_port_file(scheme_t *sc, FILE *fout) {
	sc->outport=port_from_file(sc,fout,port_output);
}

void scheme_set_output_port_string(scheme_t *sc, char *start, char *past_the_end) {
	sc->outport=port_from_string(sc,start,past_the_end,port_output);
}

void scheme_set_external_data(scheme_t *sc, void *p) {
	sc->ext_data=p;
}

void scheme_deinit(scheme_t *sc) {
	int i;

#if SHOW_ERROR_LINE
	char *fname;
#endif

	sc->oblist=sc->NIL;
	sc->global_env=sc->NIL;
	dump_stack_free(sc);
	sc->envir=sc->NIL;
	sc->code=sc->NIL;
	sc->args=sc->NIL;
	sc->value=sc->NIL;
	if(is_port(sc->inport)) {
		typeflag(sc->inport) = T_ATOM;
	}
	sc->inport=sc->NIL;
	sc->outport=sc->NIL;
	if(is_port(sc->save_inport)) {
		typeflag(sc->save_inport) = T_ATOM;
	}
	sc->save_inport=sc->NIL;
	if(is_port(sc->loadport)) {
		typeflag(sc->loadport) = T_ATOM;
	}
	sc->loadport=sc->NIL;
	sc->gc_verbose=0;
	gc(sc,sc->NIL,sc->NIL);

	for(i=0; i<=sc->last_cell_seg; i++) {
		sc->free(sc->alloc_seg[i]);
	}

#if SHOW_ERROR_LINE
	for(i=0; i<=sc->file_i; i++) {
		if (sc->load_stack[i].kind & port_file) {
			fname = sc->load_stack[i].rep.stdio.filename;
			if(fname)
				sc->free(fname);
		}
	}
#endif
}

void scheme_load_file(scheme_t *sc, FILE *fin)
{ scheme_load_named_file(sc,fin,0); }

void scheme_load_named_file(scheme_t *sc, FILE *fin, const char *filename) {
	dump_stack_reset(sc);
	sc->envir = sc->global_env;
	sc->file_i=0;
	sc->load_stack[0].kind=port_input|port_file;
	sc->load_stack[0].rep.stdio.file=fin;
	sc->loadport=mk_port(sc,sc->load_stack);
	sc->retcode=0;
	if(fin==stdin) {
		sc->interactive_repl=1;
	}

#if SHOW_ERROR_LINE
	sc->load_stack[0].rep.stdio.curr_line = 0;
	if(fin!=stdin && filename)
		sc->load_stack[0].rep.stdio.filename = store_string(sc, strlen(filename), filename, 0);
#endif

	sc->inport=sc->loadport;
	sc->args = mk_integer(sc,sc->file_i);
	Eval_Cycle(sc, OP_T0LVL);
	typeflag(sc->loadport)=T_ATOM;
	if(sc->retcode==0) {
		sc->retcode=sc->nesting!=0;
	}
}

void scheme_load_string(scheme_t *sc, const char *cmd) {
	dump_stack_reset(sc);
	sc->envir = sc->global_env;
	sc->file_i=0;
	sc->load_stack[0].kind=port_input|port_string;
	sc->load_stack[0].rep.string.start=(char*)cmd; /* This func respects const */
	sc->load_stack[0].rep.string.past_the_end=(char*)cmd+strlen(cmd);
	sc->load_stack[0].rep.string.curr=(char*)cmd;
	sc->loadport=mk_port(sc,sc->load_stack);
	sc->retcode=0;
	sc->interactive_repl=0;
	sc->inport=sc->loadport;
	sc->args = mk_integer(sc,sc->file_i);
	Eval_Cycle(sc, OP_T0LVL);
	typeflag(sc->loadport)=T_ATOM;
	if(sc->retcode==0) {
		sc->retcode=sc->nesting!=0;
	}
}

void scheme_define(scheme_t *sc, cell_ptr_t envir, cell_ptr_t symbol, cell_ptr_t value) {
	cell_ptr_t x;

	x=find_slot_in_env(sc,envir,symbol,0);
	if (x != sc->NIL) {
		set_slot_in_env(sc, x, value);
	} else {
		new_slot_spec_in_env(sc, envir, symbol, value);
	}
}

#if !STANDALONE
void scheme_register_foreign_func(scheme_t * sc, scheme_registerable * sr)
{
	scheme_define(sc,
		      sc->global_env,
		      mk_symbol(sc,sr->name),
		      mk_foreign_func(sc, sr->f));
}

void scheme_register_foreign_func_list(scheme_t * sc,
				       scheme_registerable * list,
				       int count)
{
	int i;
	for(i = 0; i < count; i++)
	{
		scheme_register_foreign_func(sc, list + i);
	}
}

cell_ptr_t scheme_apply0(scheme_t *sc, const char *procname)
{ return scheme_eval(sc, cons(sc,mk_symbol(sc,procname),sc->NIL)); }

void save_from_C_call(scheme_t *sc)
{
	cell_ptr_t saved_data =
			cons(sc,
			     car(sc->sink),
			     cons(sc,
				  sc->envir,
				  sc->dump));
	/* Push */
	sc->c_nest = cons(sc, saved_data, sc->c_nest);
	/* Truncate the dump stack so TS will return here when done, not
     directly resume pre-C-call operations. */
	dump_stack_reset(sc);
}
void restore_from_C_call(scheme_t *sc)
{
	car(sc->sink) = caar(sc->c_nest);
	sc->envir = cadar(sc->c_nest);
	sc->dump = cdr(cdar(sc->c_nest));
	/* Pop */
	sc->c_nest = cdr(sc->c_nest);
}

/* "func" and "args" are assumed to be already eval'ed. */
cell_ptr_t scheme_call(scheme_t *sc, cell_ptr_t func, cell_ptr_t args)
{
	int old_repl = sc->interactive_repl;
	sc->interactive_repl = 0;
	save_from_C_call(sc);
	sc->envir = sc->global_env;
	sc->args = args;
	sc->code = func;
	sc->retcode = 0;
	Eval_Cycle(sc, OP_APPLY);
	sc->interactive_repl = old_repl;
	restore_from_C_call(sc);
	return sc->value;
}

cell_ptr_t scheme_eval(scheme_t *sc, cell_ptr_t obj)
{
	int old_repl = sc->interactive_repl;
	sc->interactive_repl = 0;
	save_from_C_call(sc);
	sc->args = sc->NIL;
	sc->code = obj;
	sc->retcode = 0;
	Eval_Cycle(sc, OP_EVAL);
	sc->interactive_repl = old_repl;
	restore_from_C_call(sc);
	return sc->value;
}


#endif

/* ========== Main ========== */

#if STANDALONE

#if defined(__APPLE__) && !defined (OSX)
int main()
{
	extern MacTS_main(int argc, char **argv);
	char**    argv;
	int argc = ccommand(&argv);
	MacTS_main(argc,argv);
	return 0;
}
int MacTS_main(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
	scheme_t sc;
	FILE *fin;
	char *file_name=InitFile;
	int retcode;
	int isfile=1;

	if(argc==1) {
		printf(banner);
	}
	if(argc==2 && strcmp(argv[1],"-?")==0) {
		printf("Usage: tinyscheme -?\n");
		printf("or:    tinyscheme [<file1> <file2> ...]\n");
		printf("followed by\n");
		printf("          -1 <file> [<arg1> <arg2> ...]\n");
		printf("          -c <Scheme commands> [<arg1> <arg2> ...]\n");
		printf("assuming that the executable is named tinyscheme.\n");
		printf("Use - as filename for stdin.\n");
		return 1;
	}
	if(!scheme_init(&sc)) {
		fprintf(stderr,"Could not initialize!\n");
		return 2;
	}
	scheme_set_input_port_file(&sc, stdin);
	scheme_set_output_port_file(&sc, stdout);
#if USE_DL
	scheme_define(&sc,sc.global_env,mk_symbol(&sc,"load-extension"),mk_foreign_func(&sc, scm_load_ext));
#endif
	argv++;
	if(access(file_name,0)!=0) {
		char *p=getenv("TINYSCHEMEINIT");
		if(p!=0) {
			file_name=p;
		}
	}
	do {
		if(strcmp(file_name,"-")==0) {
			fin=stdin;
		} else if(strcmp(file_name,"-1")==0 || strcmp(file_name,"-c")==0) {
			cell_ptr_t args=sc.NIL;
			isfile=file_name[1]=='1';
			file_name=*argv++;
			if(strcmp(file_name,"-")==0) {
				fin=stdin;
			} else if(isfile) {
				fin=fopen(file_name,"r");
			}
			for(;*argv;argv++) {
				cell_ptr_t value=mk_string(&sc,*argv);
				args=cons(&sc,value,args);
			}
			args=reverse_in_place(&sc,sc.NIL,args);
			scheme_define(&sc,sc.global_env,mk_symbol(&sc,"*args*"),args);

		} else {
			fin=fopen(file_name,"r");
		}
		if(isfile && fin==0) {
			fprintf(stderr,"Could not open file %s\n",file_name);
		} else {
			if(isfile) {
				scheme_load_named_file(&sc,fin,file_name);
			} else {
				scheme_load_string(&sc,file_name);
			}
			if(!isfile || fin!=stdin) {
				if(sc.retcode!=0) {
					fprintf(stderr,"Errors encountered reading %s\n",file_name);
				}
				if(isfile) {
					fclose(fin);
				}
			}
		}
		file_name=*argv++;
	} while(file_name!=0);
	if(argc==1) {
		scheme_load_named_file(&sc,stdin,0);
	}
	retcode=sc.retcode;
	scheme_deinit(&sc);

	return retcode;
}

#endif

/*
Local variables:
c-file-style: "k&r"
End:
*/
