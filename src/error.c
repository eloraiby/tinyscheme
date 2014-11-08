#include "scheme-private.h"

cell_ptr_t _error_1(scheme_t *sc, const char *s, cell_ptr_t a) {
	const char *str = s;
#if USE_ERROR_HOOK
	cell_ptr_t x;
	cell_ptr_t hdl=cell_ptr(SPCELL_ERROR_HOOK);
#endif

#if USE_ERROR_HOOK
	x = find_slot_in_env(sc,sc->envir, hdl, 1);
	if( !is_nil(x) ) {
		if( !is_nil(a) ) {
			sc->code = cons(sc, cons(sc, cell_ptr(SPCELL_QUOTE), cons(sc,(a), cell_ptr(SPCELL_NIL))), cell_ptr(SPCELL_NIL));
		} else {
			sc->code = cell_ptr(SPCELL_NIL);
		}
		sc->code = cons(sc, mk_string(sc, str), sc->code);
		setimmutable(sc, car(sc, sc->code));
		sc->code = cons(sc, slot_value_in_env(sc, x), sc->code);
		sc->op = (int)OP_EVAL;
		return cell_ptr(SPCELL_TRUE);
	}
#endif

	if( !is_nil(a) ) {
		sc->args = cons(sc, (a), cell_ptr(SPCELL_NIL));
	} else {
		sc->args = cell_ptr(SPCELL_NIL);
	}
	sc->args = cons(sc, mk_string(sc, str), sc->args);
	setimmutable(sc, car(sc, sc->args));
	sc->op = (int)OP_ERR0;
	return cell_ptr(SPCELL_TRUE);
}
