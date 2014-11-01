#include "scheme-private.h"

#ifndef USE_SCHEME_STACK

/* this structure holds all the interpreter's registers */
struct dump_stack_frame {
	enum scheme_opcodes op;
	cell_ptr_t args;
	cell_ptr_t envir;
	cell_ptr_t code;
};

#define STACK_GROWTH 3

void s_save(scheme_t *sc, enum scheme_opcodes op, cell_ptr_t args, cell_ptr_t code) {
	int nframes = (int)sc->dump;
	struct dump_stack_frame *next_frame;

	/* enough room for the next frame? */
	if (nframes >= sc->dump_size) {
		sc->dump_size += STACK_GROWTH;
		/* alas there is no sc->realloc */
		sc->dump_base = realloc(sc->dump_base,
					sizeof(struct dump_stack_frame) * sc->dump_size);
	}
	next_frame = (struct dump_stack_frame *)sc->dump_base + nframes;
	next_frame->op = op;
	next_frame->args = args;
	next_frame->envir = sc->envir;
	next_frame->code = code;
	sc->dump = (cell_ptr_t)(nframes+1);
}

cell_ptr_t _s_return(scheme_t *sc, cell_ptr_t a) {
	int nframes = (int)sc->dump;
	struct dump_stack_frame *frame;

	sc->value = (a);
	if (nframes <= 0) {
		return sc->NIL;
	}
	nframes--;
	frame = (struct dump_stack_frame *)sc->dump_base + nframes;
	sc->op = frame->op;
	sc->args = frame->args;
	sc->envir = frame->envir;
	sc->code = frame->code;
	sc->dump = (cell_ptr_t)nframes;
	return sc->T;
}

void dump_stack_reset(scheme_t *sc) {
	/* in this implementation, sc->dump is the number of frames on the stack */
	sc->dump = (cell_ptr_t)0;
}

void dump_stack_initialize(scheme_t *sc) {
	sc->dump_size = 0;
	sc->dump_base = NULL;
	dump_stack_reset(sc);
}

void dump_stack_free(scheme_t *sc) {
	free(sc->dump_base);
	sc->dump_base = NULL;
	sc->dump = (cell_ptr_t)0;
	sc->dump_size = 0;
}

void dump_stack_mark(scheme_t *sc) {
	int nframes = (int)sc->dump;
	int i;
	for(i=0; i<nframes; i++) {
		struct dump_stack_frame *frame;
		frame = (struct dump_stack_frame *)sc->dump_base + i;
		mark(frame->args);
		mark(frame->envir);
		mark(frame->code);
	}
}

#else

void dump_stack_reset(scheme_t *sc) {
	sc->dump = sc->NIL;
}

void dump_stack_initialize(scheme_t *sc) {
	dump_stack_reset(sc);
}

void dump_stack_free(scheme_t *sc) {
	sc->dump = sc->NIL;
}

cell_ptr_t _s_return(scheme_t *sc, cell_ptr_t a) {
	sc->value	= (a);
	if( sc->dump == sc->NIL ) return sc->NIL;
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

#endif
