#include "scheme-private.h"

/* allocate new cell segment */
int alloc_cellseg(scheme_t *sc, int n) {
	cell_ptr_t newp;
	cell_ptr_t last;
	cell_ptr_t p;
	char *cp;
	long i;
	int k;
	size_t adj = ADJ;

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

cell_ptr_t get_cell_x(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	if (sc->free_cell != sc->NIL) {
		cell_ptr_t x = sc->free_cell;
		sc->free_cell = cdr(x);
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
cell_ptr_t reserve_cells(scheme_t *sc, int n) {
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

cell_ptr_t get_consecutive_cells(scheme_t *sc, int n) {
	cell_ptr_t x;

	if(sc->no_memory) {
		return sc->sink;
	}

	/* Are there any cells available? */
	x=find_consecutive_cells(sc,n);
	if (x != sc->NIL) {
		return x;
	}

	/* If not, try gc'ing some */
	gc(sc, sc->NIL, sc->NIL);
	x=find_consecutive_cells(sc,n);
	if (x != sc->NIL) {
		return x;
	}

	/* If there still aren't, try getting more heap */
	if (!alloc_cellseg(sc,1)) {
		sc->no_memory=1;
		return sc->sink;
	}

	x=find_consecutive_cells(sc,n);
	if (x != sc->NIL) {
		return x;
	}

	/* If all fail, report failure */
	sc->no_memory=1;
	return sc->sink;
}

int count_consecutive_cells(cell_ptr_t x, int needed) {
	int n=1;
	while(cdr(x)==x+1) {
		x=cdr(x);
		n++;
		if(n>needed) return n;
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
	} else if(is_port(a)) {
		if(a->_object._port->kind&port_file
			&& a->_object._port->rep.stdio.closeit) {
			port_close(sc,a,port_input|port_output);
		}
		sc->free(a->_object._port);
	}
}
