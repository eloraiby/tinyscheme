/* dynload.h */
/* Original Copyright (c) 1999 Alexander Shendi     */
/* Modifications for NT and dl_* interface: D. Souflis */

#ifndef DYNLOAD_H
#define DYNLOAD_H

#include "scheme-private.h"

SCHEME_EXPORT cell_ptr_t scm_load_ext(scheme_t *sc, cell_ptr_t arglist);

#endif
