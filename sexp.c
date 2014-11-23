#include "scheme-private.h"

#if USE_STRCASECMP
#include <strings.h>
# ifndef __APPLE__
#  define stricmp strcasecmp
# endif
#endif


/* make closure. c is code. e is environment */
cell_ptr_t mk_closure(scheme_t *sc, cell_ptr_t c, cell_ptr_t e) {
	cell_ptr_t x = get_cell(sc, c, e);

	typeflag(x) = T_CLOSURE;
	car(x) = c;
	cdr(x) = e;
	return (x);
}

/* make continuation. */
cell_ptr_t mk_continuation(scheme_t *sc, cell_ptr_t d) {
	cell_ptr_t x = get_cell(sc, sc->syms.NIL, d);

	typeflag(x) = T_CONTINUATION;
	cont_dump(x) = d;
	return (x);
}

cell_ptr_t mk_proc(scheme_t *sc, enum scheme_opcodes op) {
	cell_ptr_t y;

	y = get_cell(sc, sc->syms.NIL, sc->syms.NIL);
	typeflag(y) = (T_PROC | T_ATOM);
	ivalue_unchecked(y) = (long) op;
	set_num_integer(y);
	return y;
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
		if (fast == sc->syms.NIL)
			return i;
		if (!is_pair(fast))
			return -2 - i;
		fast = cdr(fast);
		++i;
		if (fast == sc->syms.NIL)
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

/* ========== Environment implementation  ========== */

#if !defined(USE_ALIST_ENV) || !defined(USE_OBJECT_LIST)

int hash_fn(const char *key, int table_size)
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

void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env)
{
	cell_ptr_t new_frame;

	/* The interaction-environment has about 300 variables in it. */
	if (old_env == sc->syms.NIL) {
		new_frame = mk_vector(sc, 461);
	} else {
		new_frame = sc->syms.NIL;
	}

	sc->regs.envir = immutable_cons(sc, new_frame, old_env);
	setenvironment(sc->regs.envir);
}



cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all)
{
	cell_ptr_t x,y;
	int location;

	for (x = env; x != sc->syms.NIL; x = cdr(x)) {
		if (is_vector(car(x))) {
			location = hash_fn(symname(hdl), ivalue_unchecked(car(x)));
			y = vector_elem(car(x), location);
		} else {
			y = car(x);
		}
		for ( ; y != sc->syms.NIL; y = cdr(y)) {
			if (caar(y) == hdl) {
				break;
			}
		}
		if (y != sc->syms.NIL) {
			break;
		}
		if(!all) {
			return sc->syms.NIL;
		}
	}
	if (x != sc->syms.NIL) {
		return car(y);
	}
	return sc->syms.NIL;
}

#else /* USE_ALIST_ENV */

static INLINE void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env)
{
	sc->regs.envir = immutable_cons(sc, sc->syms.NIL, old_env);
	setenvironment(sc->regs.envir);
}

static INLINE void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env,
					cell_ptr_t variable, cell_ptr_t value)
{
	car(env) = immutable_cons(sc, immutable_cons(sc, variable, value), car(env));
}

static cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all)
{
	cell_ptr_t x,y;
	for (x = env; x != sc->syms.NIL; x = cdr(x)) {
		for (y = car(x); y != sc->syms.NIL; y = cdr(y)) {
			if (caar(y) == hdl) {
				break;
			}
		}
		if (y != sc->syms.NIL) {
			break;
		}
		if(!all) {
			return sc->syms.NIL;
		}
	}
	if (x != sc->syms.NIL) {
		return car(y);
	}
	return sc->syms.NIL;
}

#endif /* USE_ALIST_ENV else */

/* ========== oblist implementation  ========== */

#ifndef USE_OBJECT_LIST

cell_ptr_t oblist_initial_value(scheme_t *sc)
{
	return mk_vector(sc, 461); /* probably should be bigger */
}

/* returns the new symbol */
cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name)
{
	cell_ptr_t x;
	int location;

	x = immutable_cons(sc, mk_string(sc, name), sc->syms.NIL);
	typeflag(x) = T_SYMBOL;
	setimmutable(car(x));

	location = hash_fn(name, ivalue_unchecked(sc->oblist));
	set_vector_elem(sc->oblist, location,
			immutable_cons(sc, x, vector_elem(sc->oblist, location)));
	return x;
}

cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name)
{
	int location;
	cell_ptr_t x;
	char *s;

	location = hash_fn(name, ivalue_unchecked(sc->oblist));
	for (x = vector_elem(sc->oblist, location); x != sc->syms.NIL; x = cdr(x)) {
		s = symname(car(x));
		/* case-insensitive, per R5RS section 2. */
		if(stricmp(name, s) == 0) {
			return car(x);
		}
	}
	return sc->syms.NIL;
}

cell_ptr_t oblist_all_symbols(scheme_t *sc)
{
	int i;
	cell_ptr_t x;
	cell_ptr_t ob_list = sc->syms.NIL;

	for (i = 0; i < ivalue_unchecked(sc->oblist); i++) {
		for (x  = vector_elem(sc->oblist, i); x != sc->syms.NIL; x = cdr(x)) {
			ob_list = cons(sc, x, ob_list);
		}
	}
	return ob_list;
}

#else

cell_ptr_t oblist_initial_value(scheme_t *sc)
{
	return sc->syms.NIL;
}

cell_ptr_t oblist_find_by_name(scheme_t *sc, const char *name)
{
	cell_ptr_t x;
	char    *s;

	for (x = sc->oblist; x != sc->syms.NIL; x = cdr(x)) {
		s = symname(car(x));
		/* case-insensitive, per R5RS section 2. */
		if(stricmp(name, s) == 0) {
			return car(x);
		}
	}
	return sc->syms.NIL;
}

/* returns the new symbol */
cell_ptr_t oblist_add_by_name(scheme_t *sc, const char *name)
{
	cell_ptr_t x;

	x = immutable_cons(sc, mk_string(sc, name), sc->syms.NIL);
	typeflag(x) = T_SYMBOL;
	setimmutable(car(x));
	sc->oblist = immutable_cons(sc, x, sc->oblist);
	return x;
}

cell_ptr_t oblist_all_symbols(scheme_t *sc)
{
	return sc->oblist;
}

#endif

/* ========== Routines for Evaluation Cycle ========== */

cell_ptr_t list_star(scheme_t *sc, cell_ptr_t d) {
	cell_ptr_t p, q;
	if(cdr(d)==sc->syms.NIL) {
		return car(d);
	}
	p=cons(sc,car(d),cdr(d));
	q=p;
	while(cdr(cdr(p))!=sc->syms.NIL) {
		d=cons(sc,car(p),cdr(p));
		if(cdr(cdr(p))!=sc->syms.NIL) {
			p=cdr(d);
		}
	}
	cdr(p)=car(cdr(p));
	return q;
}

/* reverse list -- produce new list */
cell_ptr_t reverse(scheme_t *sc, cell_ptr_t a) {
	/* a must be checked by gc */
	cell_ptr_t p = sc->syms.NIL;

	for ( ; is_pair(a); a = cdr(a)) {
		p = cons(sc, car(a), p);
	}
	return (p);
}

/* reverse list --- in-place */
cell_ptr_t reverse_in_place(scheme_t *sc, cell_ptr_t term, cell_ptr_t list) {
	cell_ptr_t p = list, result = term, q;

	while (p != sc->syms.NIL) {
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

	if (p == sc->syms.NIL) {
		return result;
	}

	return sc->syms.F;   /* signal an error */
}
