/* SCHEME.H */

#ifndef _SCHEME_H
#define _SCHEME_H

#ifdef __cplusplus
extern "C" {
#else
typedef unsigned char	bool;
#	define false	0
#	define true	(!false)
#endif

#ifdef __GNUC__
#	ifdef _POSIX_C_SOURCE
#		undef _POSIX_C_SOURCE
#	endif
#	define _POSIX_C_SOURCE 200112L
#	define INLINE static __inline__ __attribute__ ((__unused__))
#	define UNUSED __attribute__ ((__unused__))
#else
#	define INLINE __inline
#	define UNUSED
#endif

#include <stdio.h>

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

#if USE_DL
# define USE_INTERFACE 1
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

#ifndef USE_INTERFACE
# define USE_INTERFACE 0
#endif

#ifndef SHOW_ERROR_LINE   /* Show error line in file */
# define SHOW_ERROR_LINE 1
#endif

#define __USE_SVID 1
#include <ctype.h>

#if USE_STRCASECMP
#include <strings.h>
# ifndef __APPLE__
#  define stricmp strcasecmp
# endif
#endif

#include <string.h>
#include <stdlib.h>

#if USE_MATH
# include <math.h>
#endif

#include <limits.h>
#include <float.h>

typedef struct scheme_t scheme_t;
typedef struct cell_t cell_t;
typedef struct cell_t* cell_ptr_t;

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

SCHEME_EXPORT scheme_t *scheme_init_new();
SCHEME_EXPORT scheme_t *scheme_init_new_custom_alloc(func_alloc malloc, func_dealloc free);
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


#if USE_INTERFACE
struct scheme_interface {
  void (*scheme_define)(scheme_t *sc, cell_ptr_t env, cell_ptr_t symbol, cell_ptr_t value);
  cell_ptr_t (*cons)(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
  cell_ptr_t (*immutable_cons)(scheme_t *sc, cell_ptr_t a, cell_ptr_t b);
  cell_ptr_t (*reserve_cells)(scheme_t *sc, int n);
  cell_ptr_t (*mk_integer)(scheme_t *sc, long number_t);
  cell_ptr_t (*mk_real)(scheme_t *sc, double number_t);
  cell_ptr_t (*mk_symbol)(scheme_t *sc, const char *name);
  cell_ptr_t (*gensym)(scheme_t *sc);
  cell_ptr_t (*mk_string)(scheme_t *sc, const char *str);
  cell_ptr_t (*mk_counted_string)(scheme_t *sc, const char *str, int len);
  cell_ptr_t (*mk_character)(scheme_t *sc, int c);
  cell_ptr_t (*mk_vector)(scheme_t *sc, int len);
  cell_ptr_t (*mk_foreign_func)(scheme_t *sc, foreign_func f);
  void (*putstr)(scheme_t *sc, const char *s);
  void (*putcharacter)(scheme_t *sc, int c);

  int (*is_string)(cell_ptr_t p);
  char *(*string_value)(cell_ptr_t p);
  int (*is_number)(cell_ptr_t p);
  number_t (*nvalue)(cell_ptr_t p);
  long (*ivalue)(cell_ptr_t p);
  double (*rvalue)(cell_ptr_t p);
  int (*is_integer)(cell_ptr_t p);
  int (*is_real)(cell_ptr_t p);
  int (*is_character)(cell_ptr_t p);
  long (*charvalue)(cell_ptr_t p);
  int (*is_list)(scheme_t *sc, cell_ptr_t p);
  int (*is_vector)(cell_ptr_t p);
  int (*list_length)(scheme_t *sc, cell_ptr_t vec);
  long (*vector_length)(cell_ptr_t vec);
  void (*fill_vector)(cell_ptr_t vec, cell_ptr_t elem);
  cell_ptr_t (*vector_elem)(cell_ptr_t vec, int ielem);
  cell_ptr_t (*set_vector_elem)(cell_ptr_t vec, int ielem, cell_ptr_t newel);
  int (*is_port)(cell_ptr_t p);

  int (*is_pair)(cell_ptr_t p);
  cell_ptr_t (*pair_car)(cell_ptr_t p);
  cell_ptr_t (*pair_cdr)(cell_ptr_t p);
  cell_ptr_t (*set_car)(cell_ptr_t p, cell_ptr_t q);
  cell_ptr_t (*set_cdr)(cell_ptr_t p, cell_ptr_t q);

  int (*is_symbol)(cell_ptr_t p);
  char *(*symname)(cell_ptr_t p);

  int (*is_syntax)(cell_ptr_t p);
  int (*is_proc)(cell_ptr_t p);
  int (*is_foreign)(cell_ptr_t p);
  char *(*syntaxname)(cell_ptr_t p);
  int (*is_closure)(cell_ptr_t p);
  int (*is_macro)(cell_ptr_t p);
  cell_ptr_t (*closure_code)(cell_ptr_t p);
  cell_ptr_t (*closure_env)(cell_ptr_t p);

  int (*is_continuation)(cell_ptr_t p);
  int (*is_promise)(cell_ptr_t p);
  int (*is_environment)(cell_ptr_t p);
  int (*is_immutable)(cell_ptr_t p);
  void (*setimmutable)(cell_ptr_t p);
  void (*load_file)(scheme_t *sc, FILE *fin);
  void (*load_string)(scheme_t *sc, const char *input);
};
#endif

#if !STANDALONE
typedef struct scheme_registerable
{
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
