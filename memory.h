
// libmylcd - http://mylcd.sourceforge.net/
// An LCD framebuffer library
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2009  Michael McElligott
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for more details.
//
//	You should have received a copy of the GNU Library General Public
//	License along with this library; if not, write to the Free
//	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



#ifndef _XMEMORY_H_
#define _XMEMORY_H_



#define MEM_DEBUG		0
#define funcname		__FILE__
#define linenumber		(__LINE__)



MYLCD_EXPORT void my_MemStatsDump (THWD *hw);


MYLCD_EXPORT void * my_Malloc (size_t size, const char *func, const int line)
	__attribute__((malloc));
MYLCD_EXPORT void * my_Calloc (size_t nelem, size_t elsize, const char *func, const int line)
	__attribute__((malloc));
MYLCD_EXPORT void * my_Realloc (void *ptr, size_t size, const char *func, const int line)
	__attribute__((malloc));
MYLCD_EXPORT void my_Free (void *ptr, const char *func, const int line);
MYLCD_EXPORT char * my_Strdup (const char *str, const char *func, const int line)
	__attribute__((nonnull(1)));
MYLCD_EXPORT wchar_t * my_Wcsdup (const wchar_t *str, const char *func, const int line)
	__attribute__((nonnull(1)));
MYLCD_EXPORT void * my_Memcpy (void *s1, const void *s2, size_t n)
	__attribute__((nonnull(1, 2)));
//MYLCD_EXPORT void my_MemStatsDump (THWD *hw);



#if USEEXTMEMFUNC


//#define my_memcpy(s1,s2,n)	my_Memcpy(s1, s2, n)
#if USE_MMX_MEMCPY
#define my_memcpy			mmx_memcpy
else
#define my_memcpy			memcpy
#endif

#define my_malloc(n)		my_Malloc(n, funcname, linenumber)
#define my_calloc(n,e)		my_Calloc(n, e, funcname, linenumber)
#define my_realloc(p,n)		my_Realloc(p, n, funcname, linenumber)
#define my_free(p)			my_Free(p, funcname, linenumber)
#define my_strdup(p)		my_Strdup(p, funcname, linenumber)
#define my_wcsdup(p)		my_Wcsdup(p, funcname, linenumber)


#else

#if USE_MMX_MEMCPY
#define my_memcpy(s1, s2, n) mmx_memcpy(s1, s2, n)
#else
#define my_memcpy(s1, s2, n) memcpy(s1, s2, n)
#endif


#if (!USEINTERNALMEMMANGER)

#if 0			/* use dr_alloc routines, warning: they're slow!! */

#include "dralloc.h"

#define my_malloc(n)		dr_malloc((n))
#define my_calloc(n, e)		dr_calloc((n), (e))
#define my_realloc(p, n)	dr_realloc((p), (n))
#define my_free(p)			dr_free((p))
#define my_strdup(p)		dr_strdup((p))
#define my_wcsdup(p)		dr_wcsdup((p))
#else
#define my_malloc(n)		malloc(n)
#define my_calloc(n, e)		calloc(n, e)
#define my_realloc(p, n)	realloc(p, n)
#define my_free(p)			free(p)
#define my_strdup(p)		strdup(p)
#define my_wcsdup(p)		wcsdup(p)
#endif

#else

#if MEM_DEBUG
void * x_malloc (size_t size, const char *func, const int line);
void * x_calloc (size_t nelem, size_t elsize, const char *func, const int line);
void * x_realloc (void *ptr, size_t size, const char *func, const int line);
void x_free (void *ptr, const char *func, const int line);
char * x_strdup (const char *str, const char *func, const int line);
wchar_t * x_wcsdup (const wchar_t *str, const char *func, const int line);

#define my_malloc(n)		x_malloc((n), (funcname), (linenumber))
#define my_calloc(n,e)		x_calloc((n), (e), (funcname), (linenumber))
#define my_realloc(p,n)		x_realloc((p), (n), (funcname), (linenumber))
#define my_free(p)			x_free((p), (funcname), (linenumber))
#define my_strdup(p)		x_strdup((p), (funcname), (linenumber))
#define my_wcsdup(p)		x_wcsdup((p), (funcname), (linenumber))

#else

void * x_malloc (size_t size);
void * x_calloc (size_t nelem, size_t elsize);
void * x_realloc (void *ptr, size_t size);
void x_free (void *ptr);
char * x_strdup (const char *str);
wchar_t * x_wcsdup (const wchar_t *str);

#define my_malloc(n)		x_malloc((n))
#define my_calloc(n,e)		x_calloc((n), (e))
#define my_realloc(p,n)		x_realloc((p), (n))
#define my_free(p)			x_free((p))
#define my_strdup(p)		x_strdup((p))
#define my_wcsdup(p)		x_wcsdup((p))
#endif

#endif

#endif

#endif

