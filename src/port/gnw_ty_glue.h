/*
 * Forced include (-include) of the OpenTyrian sources of the port: GNW is
 * defined, and the C allocation functions use the game heap (ty_alloc.c).
 */
#ifndef GNW_TY_GLUE_H
#define GNW_TY_GLUE_H

#ifndef GNW
#define GNW 1
#endif

#include <stdlib.h>
#include <string.h>
#include "ty_platform.h"

#define malloc(n)     ty_malloc(n)
#define calloc(n, s)  ty_calloc(n, s)
#define realloc(p, n) ty_realloc(p, n)
#define free(p)       ty_free(p)

/* the game ends with exit(): back to the platform */
void ty_exit(int code) __attribute__((noreturn));
#define exit(c)       ty_exit(c)
#define atexit(f)     ((void)(f))

#endif
