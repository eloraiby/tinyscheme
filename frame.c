#include "scheme-private.h"

/* ========== Environment implementation  ========== */

#if !defined(USE_ALIST_ENV) || !defined(USE_OBJECT_LIST)

int hash_fn(const char *key, int table_size) {
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

void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env) {
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

void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env,
					cell_ptr_t variable, cell_ptr_t value) {
	cell_ptr_t slot = immutable_cons(sc, variable, value);

	if (is_vector(car(env))) {
		int location = hash_fn(symname(variable), ivalue_unchecked(car(env)));

		set_vector_elem(car(env), location,
				immutable_cons(sc, slot, vector_elem(car(env), location)));
	} else {
		car(env) = immutable_cons(sc, slot, car(env));
	}
}

cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all) {
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

void new_frame_in_env(scheme_t *sc, cell_ptr_t old_env) {
	sc->envir = immutable_cons(sc, sc->NIL, old_env);
	setenvironment(sc->envir);
}

void new_slot_spec_in_env(scheme_t *sc, cell_ptr_t env,
					cell_ptr_t variable, cell_ptr_t value) {
	car(env) = immutable_cons(sc, immutable_cons(sc, variable, value), car(env));
}

cell_ptr_t find_slot_in_env(scheme_t *sc, cell_ptr_t env, cell_ptr_t hdl, int all) {
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

