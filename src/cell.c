#include "scheme-private.h"

/* allocate new cell segment */
int alloc_cellseg(scheme_t *sc) {
	cell_ptr_t p;


	/* insert new segment in address order */
	sc->fcells += CELL_MAX_COUNT;

	for( p.index = SPCELL_LAST + 1; p.index < CELL_MAX_COUNT; p.index++) {
		ptr_typeflag(sc, p) = 0;
		cdr(sc, p) = cell_ptr(p.index + 1);
		car(sc, p) = cell_ptr(SPCELL_NIL);
	}
	sc->free_cell = cell_ptr(SPCELL_LAST + 1);
	return 1;
}

cell_ptr_t get_cell_x(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	if( !is_nil(sc->free_cell) ) {
		cell_ptr_t x = sc->free_cell;
		sc->free_cell = cdr(sc, x);
		--sc->fcells;
		return (x);
	}
	return _get_cell (sc, a, b);
}


/* get new cell.  parameter a, b is marked by gc. */
cell_ptr_t _get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	cell_ptr_t x;

	if(sc->no_memory) {
		return sc->sink;
	}

	if( is_nil(sc->free_cell) ) {
		const int min_to_be_recovered = 8;
		gc(sc, a, b);
		if( sc->fcells < min_to_be_recovered || is_nil(sc->free_cell) ) {
			sc->no_memory	= true;
			return sc->sink;
		}
	}
	x = sc->free_cell;
	sc->free_cell = cdr(sc, x);
	--sc->fcells;
	return (x);
}

/* make sure that there is a given number of cells free */
cell_ptr_t reserve_cells(scheme_t *sc, int n) {
	if( sc->no_memory ) {
		return cell_ptr(SPCELL_NIL);
	}

	/* Are there enough cells available? */
	if( sc->fcells < n ) {
		/* If not, try gc'ing some */
		gc(sc,cell_ptr(SPCELL_NIL), cell_ptr(SPCELL_NIL));
		if (sc->fcells < n) {
			/* If all fail, report failure */
			sc->no_memory	= true;
			return cell_ptr(SPCELL_NIL);
		}
	}
	return cell_ptr(SPCELL_TRUE);
}

cell_ptr_t get_consecutive_cells(scheme_t *sc, int n) {
	cell_ptr_t x;

	if(sc->no_memory) {
		return sc->sink;
	}

	/* Are there any cells available? */
	x = find_consecutive_cells(sc,n) ;
	if( !is_nil(x) ) {
		return x;
	}

	/* If not, try gc'ing some */
	gc(sc, cell_ptr(SPCELL_NIL), cell_ptr(SPCELL_NIL));
	x = find_consecutive_cells(sc, n);
	if( !is_nil(x) ) {
		return x;
	} else {
		sc->no_memory	= true;
		return sc->sink;
	}
}

int count_consecutive_cells(scheme_t *sc, cell_ptr_t x, int needed) {
	int n = 1;
	while( cdr(sc, x).index == x.index + 1 ) {
		x = cdr(sc, x);
		n++;
		if( n > needed) return n;
	}
	return n;
}

cell_ptr_t find_consecutive_cells(scheme_t *sc, int n) {
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

void push_recent_alloc(scheme_t *sc, cell_ptr_t recent, cell_ptr_t extra) {
	cell_ptr_t holder = get_cell_x(sc, recent, extra);
	typeflag(holder) = T_PAIR | T_IMMUTABLE;
	car(holder) = recent;
	cdr(holder) = car(sc->sink);
	car(sc->sink) = holder;
}


cell_ptr_t get_cell(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
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

cell_ptr_t get_vector_object(scheme_t *sc, int len, cell_ptr_t init) {
	cell_ptr_t cells = get_consecutive_cells(sc,len/2+len%2+1);
	if(sc->no_memory) {
		return sc->sink;
	}
	/* Record it as a vector so that gc understands it. */
	typeflag(cells) = (T_VECTOR | T_ATOM);
	ivalue_unchecked(cells)=len;
	set_num_integer(cells);
	fill_vector(cells,init);
	push_recent_alloc(sc, cells, sc->NIL);
	return cells;
}

#if defined TSGRIND
void check_cell_alloced(cell_ptr_t p, int expect_alloced) {
	/* Can't use putstr(sc,str) because callers have no access to
	 sc.  */
	if(typeflag(p) & !expect_alloced) {
		fprintf(stderr,"Cell is already allocated!\n");
	}
	if(!(typeflag(p)) & expect_alloced) {
		fprintf(stderr,"Cell is not allocated!\n");
	}

}
void check_range_alloced(cell_ptr_t p, int n, int expect_alloced) {
	int i;
	for(i = 0; i<n; i++) {
		(void)check_cell_alloced(p+i,expect_alloced);
	}
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


void finalize_cell(scheme_t *sc, cell_ptr_t a) {
	if(is_string(a)) {
		sc->free(strvalue(a));
	}
}

cell_ptr_t list_star(scheme_t *sc, cell_ptr_t d) {
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

