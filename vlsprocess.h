
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

#ifndef _VLSPROCESS_H_
#define _VLSPROCESS_H_

#include <tlhelp32.h>
#include <psapi.h>


#ifdef __has_include
#	if __has_include(<winternl.h>)
#	  define has_winternl_h 1
#	else
#	  define has_winternl_h 0
#	endif
#else
#	  define has_winternl_h 1
#endif

#if has_winternl_h
#include <winternl.h>
#else
#include <ddk/ntapi.h>
#include <ddk/ntddk.h>
#endif




/*
#ifndef _POSIX_

#define _P_WAIT 0
#define _P_NOWAIT 1
#define _OLD_P_OVERLAY 2
#define _P_NOWAITO 3
#define _P_DETACH 4
#define _P_OVERLAY 2
#define _WAIT_CHILD 0
#define _WAIT_GRANDCHILD 1

  _CRTIMP uintptr_t __cdecl _beginthread(void (__cdecl *_StartAddress) (void *),unsigned _StackSize,void *_ArgList);
  _CRTIMP void __cdecl _endthread(void) __MINGW_ATTRIB_NORETURN;
  _CRTIMP uintptr_t __cdecl _beginthreadex(void *_Security,unsigned _StackSize,unsigned (__stdcall *_StartAddress) (void *),void *_ArgList,unsigned _InitFlag,unsigned *_ThrdAddr);
  _CRTIMP void __cdecl _endthreadex(unsigned _Retval) __MINGW_ATTRIB_NORETURN;
  _CRTIMP int __cdecl _getpid(void);
#endif
*/




int processGetWindowThreadPid (HANDLE hWnd);
wchar_t *processGetFilename (const int pid);
void processSetErrorMode (const int mode);
void processSetPriority (const int classLevel);
HANDLE processGetCurrent ();
uint64_t processGetMemUsage (const int pid);

void processClose (HANDLE hProcess);
HANDLE processOpen (const int id);
int processGetId ();

int processCreate (const char *cmdLine);
int processCreateW (const wchar_t *cmdLine);

int processKillWindow (const int pid);
int processKillOpenThread (const int pid);
int processKillRemoteThread (const int pid);

int processGetPid (const char *exeModule);
int processGetPidW (const wchar_t *exeModule);
//int processGetTotal (const HANDLE hSnap);



typedef struct process_list {
	PROCESSENTRY32W entry;
	struct process_list *next;
}process_list;

typedef struct module_list {
	MODULEENTRY32W entry;
	struct module_list *next;
}module_list;

typedef struct {
	struct {
    	int processId;
    	int totalThreads;

    	uint64_t createTime;
    	uint64_t userTime;
    	uint64_t kernelTime;
    	uint64_t sumTime;
    	uint64_t deltaTime;

    	wchar_t *path;
    	wchar_t *imageName;
    	char *imageName8;

    	int basePriority;
    	int inheritedFromProcessId;
    	int totalHandles;
    	//int privatePageCount;
    	VM_COUNTERS vm_counters;
    	IO_COUNTERS io_counters;

    	struct {
    		int mask;
    		int total;		// number of cores in use by process
    	}affinity;
	}info;

	SYSTEMTIME time;
	int tmp;	// do not remove me
	uint64_t freq;
	uint64_t snapshotTime;
	float multiplier;
	float cpuTime;
	//uint64_t ticks;
	int64_t udata;
}process_list_extended;


void enablePrivileges ();

process_list *processGetProcessList (int *total);
void processFreeProcessList (process_list *list);


module_list *processGetModuleList (const int pid, int *total);
void processFreeModuleList (module_list *list);

uint64_t processGetMemoryWorkingSize (const int pid);
int processGetMemoryStats (const int pid, PROCESS_MEMORY_COUNTERS *pmc);

void processFreeProcessListExtended (process_list_extended *list);
process_list_extended *processGetProcessListExtended (int *count);

int processSubtractProcessList (process_list_extended *dst, const process_list_extended *src);
int processMultiplyProcessList (process_list_extended *dst, const float value);

const process_list_extended *processFindProcess (const process_list_extended *ple, const int pid);


void _enablePrivileges ();


#endif


