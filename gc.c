#include "scheme-private.h"

/* ========== garbage collector ========== */

/*--
 *  We use algorithm E (Knuth, The Art of Computer Programming Vol.1,
 *  sec. 2.3.5), the Schorr-Deutsch-Waite link-inversion algorithm,
 *  for marking.
 */
void mark(cell_ptr_t a) {
	cell_ptr_t t, q, p;

	t = (cell_ptr_t) 0;
	p = a;
E2:
	setmark(p);
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
E5:
	q = cdr(p); /* down cdr */
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
void gc(scheme_t *sc, cell_ptr_t a, cell_ptr_t b) {
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
	p = sc->cell_seg + CELL_MAX_COUNT;
	while (--p >= sc->cell_seg) {
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

	if (sc->gc_verbose) {
		char msg[80];
		snprintf(msg,80,"done: %ld cells were recovered.\n", sc->fcells);
		putstr(sc,msg);
	}
}
