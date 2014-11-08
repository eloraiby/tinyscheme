#include "scheme-private.h"

/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
void mark(scheme_t* sc, cell_ptr_t a) {
	cell_ptr_t t, q, p;

	t = cell_ptr(SPCELL_NIL);
	p = a;
E2:
	setmark(sc, p);
	if(is_vector(sc, p)) {
		int i;
		int number_t = ivalue_unchecked(sc, p) / 2 + ivalue_unchecked(sc, p) % 2;
		for(i=0; i<number_t; i++) {
			/* Vector cells will be treated like ordinary cells */
			mark(sc, cell_ptr(p.index + 1 + i));
		}
	}

	if( is_atom(sc, p) )
		goto E6;
	/* E4: down car */
	q = car(sc, p);
	if( !is_nil(q) && !is_mark(sc, q) ) {
		setatom(sc, p);  /* a note that we have moved car */
		car(sc, p) = t;
		t = p;
		p = q;
		goto E2;
	}
E5:
	q = cdr(sc, p); /* down cdr */
	if( !is_nil(q) && !is_mark(sc, q) ) {
		cdr(sc, p) = t;
		t = p;
		p = q;
		goto E2;
	}
E6:   /* up.  Undo the link switching from steps E4 and E5. */
	if( is_nil(t) )
		return;
	q = t;
	if( is_atom(sc, q) ) {
		clratom(sc, q);
		t = car(sc, q);
		car(sc, q) = p;
		p = q;
		goto E5;
	} else {
		t = cdr(sc, q);
		cdr(sc, q) = p;
		p = q;
		goto E6;
	}
}

/* garbage collection. parameter a, b is marked. */
void gc(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
	cell_ptr_t p;

	if(sc->gc_verbose) {
		fprintf(stdout, "gc...");
	}

	/* mark system globals */
	mark(sc, sc->oblist);
	mark(sc, sc->global_env);

	/* mark current registers */
	mark(sc, sc->args);
	mark(sc, sc->envir);
	mark(sc, sc->code);
	dump_stack_mark(sc);
	mark(sc, sc->value);

	/* Mark recent objects the interpreter doesn't know about yet. */
	mark(sc, car(sc, sc->sink));
	/* Mark any older stuff above nested C calls */
	mark(sc, sc->c_nest);

	/* mark variables a, b */
	mark(sc, a);
	mark(sc, b);

	/* garbage collect */
	clrmark(sc, cell_ptr(SPCELL_NIL));
	sc->fcells = 0;
	sc->free_cell = cell_ptr(SPCELL_NIL);
	/* free-list is kept sorted by address so as to maintain consecutive
	 ranges, if possible, for use with vectors. Here we scan the cells
	 (which are also kept sorted by address) downwards to build the
	 free-list in sorted order.
	*/
	p = cell_ptr(CELL_MAX_COUNT);
	while( --p.index ) {
		if (is_mark(sc, p)) {
			clrmark(sc, p);
		} else {
			/* reclaim cell */
			if( ptr_typeflag(sc, p) != 0 ) {
				finalize_cell(sc, p);
				ptr_typeflag(sc, p) = 0;
				car(sc, p) = cell_ptr(SPCELL_NIL);
			}
			++sc->fcells;
			cdr(sc, p) = sc->free_cell;
			sc->free_cell = p;
		}
	}

	if (sc->gc_verbose) {
		char msg[80];
		snprintf(msg,80,"done: %ld cells were recovered.\n", sc->fcells);
		fprintf(stdout, "%s", msg);
	}
}
