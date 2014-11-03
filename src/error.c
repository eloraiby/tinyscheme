#include "scheme-private.h"

cell_ptr_t _error_1(scheme_t *sc, const char *s, cell_ptr_t a) {
	const char *str = s;
#if USE_ERROR_HOOK
	cell_ptr_t x;
	cell_ptr_t hdl=sc->ERROR_HOOK;
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
