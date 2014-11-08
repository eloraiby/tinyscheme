/* SCHEME.H */

#ifndef _SCHEME_H
#define _SCHEME_H

#include <stdio.h>
#include <float.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
typedef	unsigned char	bool;
#	define false	(0)
#	define true	(!false)
#endif


typedef int8_t		sint8;
typedef int16_t		sint16;
typedef int32_t		sint32;
typedef int64_t		sint64;

typedef uint8_t		uint8;
typedef uint16_t	uint16;
typedef uint32_t	uint32;
typedef uint64_t	uint64;

typedef float		real32;
typedef double		real64;

/*
 * Default values for #define'd symbols
 */
#ifndef STANDALONE       /* If used as standalone interpreter */
# define STANDALONE 1
#endif

#ifndef _MSC_VER
# define USE_STRCASECMP 1
# ifndef USE_STRLWR
#   define USE_STRLWR 1
# endif
# define SCHEME_EXPORT
#else
# define USE_STRCASECMP 0
# define USE_STRLWR 0
# ifdef _SCHEME_SOURCE
#  define SCHEME_EXPORT __declspec(dllexport)
# else
#  define SCHEME_EXPORT __declspec(dllimport)
# endif
#endif

#if USE_NO_FEATURES
# define USE_MATH 0
# define USE_CHAR_CLASSIFIERS 0
# define USE_ASCII_NAMES 0
# define USE_STRING_PORTS 0
# define USE_ERROR_HOOK 0
# define USE_TRACING 0
# define USE_COLON_HOOK 0
# define USE_DL 0
# define USE_PLIST 0
#endif

/*
 * Leave it defined if you want continuations, and also for the Sharp Zaurus.
 * Undefine it if you only care about faster speed and not strict Scheme compatibility.
 */
#define USE_SCHEME_STACK

#ifndef USE_DL
# define USE_DL 1
#endif

#ifndef USE_MATH         /* If math support is needed */
# define USE_MATH 1
#endif

#ifndef USE_CHAR_CLASSIFIERS  /* If char classifiers are needed */
# define USE_CHAR_CLASSIFIERS 1
#endif

#ifndef USE_ASCII_NAMES  /* If extended escaped characters are needed */
# define USE_ASCII_NAMES 1
#endif

#ifndef USE_STRING_PORTS      /* Enable string ports */
# define USE_STRING_PORTS 1
#endif

#ifndef USE_TRACING
# define USE_TRACING 1
#endif

#ifndef USE_PLIST
# define USE_PLIST 0
#endif

/* To force system errors through user-defined error handling (see *error-hook*) */
#ifndef USE_ERROR_HOOK
# define USE_ERROR_HOOK 1
#endif

#ifndef USE_COLON_HOOK   /* Enable qualified qualifier */
# define USE_COLON_HOOK 1
#endif

#ifndef USE_STRCASECMP   /* stricmp for Unix */
# define USE_STRCASECMP 0
#endif

#ifndef USE_STRLWR
# define USE_STRLWR 1
#endif

#ifndef STDIO_ADDS_CR    /* Define if DOS/Windows */
# define STDIO_ADDS_CR 0
#endif

#ifndef INLINE
# define INLINE
#endif

#ifndef SHOW_ERROR_LINE   /* Show error line in file */
# define SHOW_ERROR_LINE 1
#endif

typedef struct scheme_t scheme_t;
typedef struct cell_t cell_t;

typedef struct cell_ptr_t {	/* C does not have a unit of measure */
	unsigned int	index;
} cell_ptr_t;

#define	is_nil(C)	(C.index == 0)

typedef void * (*func_alloc)(size_t);
typedef void (*func_dealloc)(void *);

/* number_t, for generic arithmetic */
typedef struct number_t {
	char is_integer;
	union {
		long ivalue;
		double rvalue;
	} value;
} number_t;

SCHEME_EXPORT cell_t*		cell_ptr_to_cell(scheme_t* sc, cell_ptr_t cell);
SCHEME_EXPORT cell_ptr_t	cell_to_cell_ptr(scheme_t* sc, cell_t* cell);

SCHEME_EXPORT scheme_t*		scheme_init_new();
SCHEME_EXPORT scheme_t*		scheme_init_new_custom_alloc(func_alloc malloc, func_dealloc free);
SCHEME_EXPORT int scheme_init(scheme_t *sc);
SCHEME_EXPORT int scheme_init_custom_alloc(scheme_t *sc, func_alloc, func_dealloc);
SCHEME_EXPORT void scheme_deinit(scheme_t *sc);
void scheme_set_input_port_file(scheme_t *sc, FILE *fin);
void scheme_set_input_port_string(scheme_t *sc, char *start, char *past_the_end);
SCHEME_EXPORT void scheme_set_output_port_file(scheme_t *sc, FILE *fin);
void scheme_set_output_port_string(scheme_t *sc, char *start, char *past_the_end);
SCHEME_EXPORT void scheme_load_file(scheme_t *sc, FILE *fin);
SCHEME_EXPORT void scheme_load_named_file(scheme_t *sc, FILE *fin, const char *filename);
SCHEME_EXPORT void scheme_load_string(scheme_t *sc, const char *cmd);
SCHEME_EXPORT cell_ptr_t scheme_apply0(scheme_t *sc, const char *procname);
SCHEME_EXPORT cell_ptr_t scheme_call(scheme_t *sc, cell_ptr_t func, cell_ptr_t args);
SCHEME_EXPORT cell_ptr_t scheme_eval(scheme_t *sc, cell_ptr_t obj);
void scheme_set_external_data(scheme_t *sc, void *p);
SCHEME_EXPORT void scheme_define(scheme_t *sc, cell_ptr_t env, cell_ptr_t symbol, cell_ptr_t value);

typedef cell_ptr_t (*foreign_func)(scheme_t *, cell_ptr_t);

cell_ptr_t _cons(scheme_t *sc, cell_ptr_t a, cell_ptr_t b, int immutable);
cell_ptr_t mk_integer(scheme_t *sc, long number_t);
cell_ptr_t mk_real(scheme_t *sc, double number_t);
cell_ptr_t mk_symbol(scheme_t *sc, const char *name);
cell_ptr_t gensym(scheme_t *sc);
cell_ptr_t mk_string(scheme_t *sc, const char *str);
cell_ptr_t mk_counted_string(scheme_t *sc, const char *str, int len);
cell_ptr_t mk_empty_string(scheme_t *sc, int len, char fill);
cell_ptr_t mk_character(scheme_t *sc, int c);
cell_ptr_t mk_foreign_func(scheme_t *sc, foreign_func f);
void putstr(scheme_t *sc, const char *s);
int list_length(scheme_t *sc, cell_ptr_t a);
int eqv(cell_ptr_t a, cell_ptr_t b);


#if !STANDALONE
typedef struct scheme_registerable {
	foreign_func  f;
	const char *  name;
}
scheme_registerable;

void scheme_register_foreign_func_list(scheme_t * sc,
                                       scheme_registerable * list,
                                       int n);

#endif /* !STANDALONE */

#ifdef __cplusplus
}
#endif

#endif


/*
Local variables:
c-file-style: "k&r"
End:
*/
