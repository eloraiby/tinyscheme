#include "scheme-private.h"

void dump_stack_reset(scheme_t *sc) {
	sc->dump = cell_ptr(SPCELL_NIL);
}

void dump_stack_initialize(scheme_t *sc) {
	dump_stack_reset(sc);
}

void dump_stack_free(scheme_t *sc) {
	sc->dump = cell_ptr(SPCELL_NIL);
}

cell_ptr_t _s_return(scheme_t *sc, cell_ptr_t a) {
	sc->value	= (a);
	if( sc->dump == cell_ptr(SPCELL_NIL) ) return cell_ptr(SPCELL_NIL);
	sc->op		= ivalue(car(sc->dump));
	sc->args	= cadr(sc->dump);
	sc->envir	= caddr(sc->dump);
	sc->code	= cadddr(sc->dump);
	sc->dump	= cddddr(sc->dump);
	return sc->T;
}

void s_save(scheme_t *sc, enum scheme_opcodes op, cell_ptr_t args, cell_ptr_t code) {
	sc->dump = cons(sc, sc->envir, cons(sc, (code), sc->dump));
	sc->dump = cons(sc, (args), sc->dump);
	sc->dump = cons(sc, mk_integer(sc, (long)(op)), sc->dump);
}

void dump_stack_mark(scheme_t *sc) {
	mark(sc->dump);
}

