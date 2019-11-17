            
// vlc.c. libvlc 1.1.0 wrapper
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
//  GNU LIBRARY GENERAL PUBLIC LICENSE for details.



#include "common.h"


#if (!USEEXTMEMFUNC && USEINTERNALMEMMANGER)

#define MAGIC		((uint32_t)0x88888889)
#ifndef ALIGN_TYPE
#define ALIGN_TYPE uintptr_t
#endif

#if MEM_DEBUG
#define MEM_DEBUG_FUNCDEC 	, const char *func, const int line
#define MEM_DEBUG_FUNCSTR 	"%s:%i"
#define MEM_DEBUG_FUNCARG 	, func, line
#define _memError(a,b,c)	memError((a),((void*)(b)),((void*)(c)) MEM_DEBUG_FUNCARG)
#else
#define MEM_DEBUG_FUNCDEC	
#define MEM_DEBUG_FUNCSTR	""
#define MEM_DEBUG_FUNCARG	
#define _memError(a,b,c)	memError(a,((void*)(b)),((void*)(c)))
#endif




static inline size_t alignSize (size_t sizeofobject)
{
	size_t odd_bytes = sizeofobject % sizeof(ALIGN_TYPE);
	if (odd_bytes > 0)
		sizeofobject += sizeof(ALIGN_TYPE) - odd_bytes;
	return sizeofobject;
}

static inline size_t getLength (const void *const ptr)
{
	return *(size_t*)(ptr - sizeof(size_t));
}

static inline uint32_t getMagicBefore (const void *const ptr)
{
	const unsigned char *mptr = (unsigned char*)(ptr - sizeof(size_t) - sizeof(uint32_t));
	const uint32_t magic = *(uint32_t*)mptr;
	return magic;
}

static inline uint32_t getMagicAfter (const void *const ptr)
{
	const size_t len = getLength(ptr);
	const unsigned char *mptr = (unsigned char*)ptr + len;
	const uint32_t magic = *(uint32_t*)mptr;
	return magic;
}

static inline uint32_t magicCheck (const void *const ptr)
{
	const uint32_t b = getMagicBefore(ptr);
	const uint32_t a = getMagicAfter(ptr);
	return (b == MAGIC && a == MAGIC);
}

enum _mem_error {
	MEM_ERROR_FREE = 1,
	MEM_ERROR_MALLOC,
	MEM_ERROR_CALLOC,
	MEM_ERROR_REALLOC
};

static inline void memError (const int errorType, const void *arg1, const void *arg2 MEM_DEBUG_FUNCDEC)
{
	if (errorType == MEM_ERROR_FREE){
		printf("## free(%p): MAGIC MISMATCH " MEM_DEBUG_FUNCSTR "\n",arg1 MEM_DEBUG_FUNCARG);
		
	}else if (errorType == MEM_ERROR_MALLOC){
		printf("## malloc(%u): ZERO LENGTH " MEM_DEBUG_FUNCSTR "\n",(size_t)arg1 MEM_DEBUG_FUNCARG);
		
	}else if (errorType == MEM_ERROR_CALLOC){
		printf("## calloc(%u, %u): ZERO LENGTH " MEM_DEBUG_FUNCSTR "\n",(size_t)arg1, (size_t)arg2 MEM_DEBUG_FUNCARG);
		
	}else if (errorType == MEM_ERROR_REALLOC){
		printf("## realloc(%p, %u): MAGIC MISMATCH " MEM_DEBUG_FUNCSTR "\n",arg1, (size_t)arg2 MEM_DEBUG_FUNCARG);
	}
}

void x_free (void *ptr MEM_DEBUG_FUNCDEC)
{
	if (!ptr) return;

	if (!magicCheck(ptr)){
		_memError(MEM_ERROR_FREE, ptr, 0);
		return;
	}
	free(ptr - sizeof(uint32_t) - sizeof(size_t));
}

void * x_malloc (size_t size MEM_DEBUG_FUNCDEC)
{
	
	if (!size){
		_memError(MEM_ERROR_MALLOC, size, 0);
		size = sizeof(void*);
		//abort();
	}

	void *ptr = malloc(alignSize(sizeof(uint32_t) + sizeof(size_t)+ size + sizeof(size_t)));
	if (!ptr) return NULL;

	unsigned char *mptr = (unsigned char*)ptr;
	*(uint32_t*)mptr = MAGIC;

	*(size_t*)(ptr+sizeof(uint32_t)) = (size_t)size;

	mptr = (unsigned char*)ptr + sizeof(uint32_t) + sizeof(size_t) + size;
	*(uint32_t*)mptr = MAGIC;

	return ptr + sizeof(uint32_t)+ sizeof(size_t);
}

void * x_calloc (size_t nelem, size_t elsize MEM_DEBUG_FUNCDEC)
{

	size_t len = (nelem * elsize);
	if (!len){
		_memError(MEM_ERROR_CALLOC, nelem, elsize);
		len = sizeof(void*);
		//abort();
	}
	//len += 4;
	
	void *ptr = x_malloc(len MEM_DEBUG_FUNCARG);
	if (ptr)
		memset(ptr, 0, len);

	return ptr;
}

void * x_realloc (void *_ptr, size_t size MEM_DEBUG_FUNCDEC)
{
	if (!_ptr || !size)
		return x_malloc(size MEM_DEBUG_FUNCARG);

	if (!magicCheck(_ptr)){
		_memError(MEM_ERROR_REALLOC, _ptr, size);
		return NULL;
	}
	

	void *ptr = x_malloc(size MEM_DEBUG_FUNCARG);
	if (!ptr) return NULL;

	size_t len = getLength(_ptr);
	if (len > size) len = size;
	
	my_memcpy(ptr, _ptr, len);
	if (size > len)
		memset(ptr+len, 0, size - len);
	
	x_free(_ptr MEM_DEBUG_FUNCARG);
	return ptr;
}

char * x_strdup (const char *str MEM_DEBUG_FUNCDEC)
{
	if (!str) return NULL;

	int len = strlen(str);
	if (len > 0){
		if (len < 4) len = 4;
		char *ptr = x_malloc((len*sizeof(char)) + sizeof(char) MEM_DEBUG_FUNCARG);
		if (ptr){
			my_memcpy(ptr, str, len*sizeof(char));
			ptr[len] = 0;

			return ptr;
		}
	}
	
	//printf("strdup(%p): %s:%i\n", str, func, line);
	return NULL;
}

wchar_t * x_wcsdup (const wchar_t *str MEM_DEBUG_FUNCDEC)
{
	int len = wcslen(str);
	if (len > 0){
		wchar_t *ptr = x_malloc((len*sizeof(wchar_t)) + sizeof(wchar_t) MEM_DEBUG_FUNCARG);
		if (ptr){
			my_memcpy(ptr, str, len*sizeof(wchar_t));
			ptr[len] = 0;

			return ptr;
		}
	}
	
	//printf("wcsdup(%p): %s:%i\n", str, func, line);
	return NULL;
}

#endif

