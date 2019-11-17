
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


#include "common.h"
//#include <ddk/ntapi.h>
//#include <ntapi.h>



/* System information and control */
#if 0
#ifndef SystemProcessorTimes
typedef enum _SYSTEM_INFORMATION_CLASS {
	SystemInformationClassMin = 0,
	SystemBasicInformation = 0,
	SystemProcessorInformation = 1,
	SystemPerformanceInformation = 2,
	SystemTimeOfDayInformation = 3,
	SystemPathInformation = 4,
	SystemNotImplemented1 = 4,
	SystemProcessInformation = 5,
	SystemProcessesAndThreadsInformation = 5,
	SystemCallCountInfoInformation = 6,
	SystemCallCounts = 6,
	SystemDeviceInformation = 7,
	SystemConfigurationInformation = 7,
	SystemProcessorPerformanceInformation = 8,
	SystemProcessorTimes = 8,
	SystemFlagsInformation = 9,
	SystemGlobalFlag = 9,
	SystemCallTimeInformation = 10,
	SystemNotImplemented2 = 10,
	SystemModuleInformation = 11,
	SystemLocksInformation = 12,
	SystemLockInformation = 12,
	SystemStackTraceInformation = 13,
	SystemNotImplemented3 = 13,
	SystemPagedPoolInformation = 14,
	SystemNotImplemented4 = 14,
	SystemNonPagedPoolInformation = 15,
	SystemNotImplemented5 = 15,
	SystemHandleInformation = 16,
	SystemObjectInformation = 17,
	SystemPageFileInformation = 18,
	SystemPagefileInformation = 18,
	SystemVdmInstemulInformation = 19,
	SystemInstructionEmulationCounts = 19,
	SystemVdmBopInformation = 20,
	SystemInvalidInfoClass1 = 20,	
	SystemFileCacheInformation = 21,
	SystemCacheInformation = 21,
	SystemPoolTagInformation = 22,
	SystemInterruptInformation = 23,
	SystemProcessorStatistics = 23,
	SystemDpcBehaviourInformation = 24,
	SystemDpcInformation = 24,
	SystemFullMemoryInformation = 25,
	SystemNotImplemented6 = 25,
	SystemLoadImage = 26,
	SystemUnloadImage = 27,
	SystemTimeAdjustmentInformation = 28,
	SystemTimeAdjustment = 28,
	SystemSummaryMemoryInformation = 29,
	SystemNotImplemented7 = 29,
	SystemNextEventIdInformation = 30,
	SystemNotImplemented8 = 30,
	SystemEventIdsInformation = 31,
	SystemNotImplemented9 = 31,
	SystemCrashDumpInformation = 32,
	SystemExceptionInformation = 33,
	SystemCrashDumpStateInformation = 34,
	SystemKernelDebuggerInformation = 35,
	SystemContextSwitchInformation = 36,
	SystemRegistryQuotaInformation = 37,
	SystemLoadAndCallImage = 38,
	SystemPrioritySeparation = 39,
	SystemPlugPlayBusInformation = 40,
	SystemNotImplemented10 = 40,
	SystemDockInformation = 41,
	SystemNotImplemented11 = 41,
	/* SystemPowerInformation = 42, Conflicts with POWER_INFORMATION_LEVEL 1 */
	SystemInvalidInfoClass2 = 42,
	SystemProcessorSpeedInformation = 43,
	SystemInvalidInfoClass3 = 43,
	SystemCurrentTimeZoneInformation = 44,
	SystemTimeZoneInformation = 44,
	SystemLookasideInformation = 45,
	SystemSetTimeSlipEvent = 46,
	SystemCreateSession = 47,
	SystemDeleteSession = 48,
	SystemInvalidInfoClass4 = 49,
	SystemRangeStartInformation = 50,
	SystemVerifierInformation = 51,
	SystemAddVerifier = 52,
	SystemSessionProcessesInformation	= 53,
	SystemInformationClassMax
} SYSTEM_INFORMATION_CLASS;
#endif

typedef struct _SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION {
    LARGE_INTEGER IdleTime;
    LARGE_INTEGER KernelTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER Reserved1[2];
    ULONG Reserved2;
} SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION;
#endif

#ifndef SystemProcessorTimes
#define SystemProcessorTimes  8
#endif

typedef LONG (WINAPI *PROCNTQSI) (UINT, PVOID, ULONG, PULONG);
static PROCNTQSI ntQuerySystemInformation;
static int initNTQSIOnce = 0;

#ifndef VLC_CPU_MMX

#  define VLC_CPU_MMX    0x00000008
#  define VLC_CPU_3dNOW  0x00000010
#  define VLC_CPU_MMXEXT 0x00000020
#  define VLC_CPU_SSE    0x00000040
#  define VLC_CPU_SSE2   0x00000080
#  define VLC_CPU_SSE3   0x00000100
#  define VLC_CPU_SSSE3  0x00000200
#  define VLC_CPU_SSE4_1 0x00000400
#  define VLC_CPU_SSE4_2 0x00000800
#  define VLC_CPU_SSE4A  0x00001000
#  define VLC_CPU_AVX    0x00002000
#  define VLC_CPU_AVX2   0x00004000
#  define VLC_CPU_XOP    0x00008000
#  define VLC_CPU_FMA4   0x00010000
#endif

static unsigned int cpu_hasMMX (unsigned int flags)
{
	return flags&VLC_CPU_MMX;
}

static unsigned int cpu_has3DNOW (unsigned int flags)
{
	return flags&VLC_CPU_3dNOW;
}

static unsigned int cpu_hasMMXEXT (unsigned int flags)
{
	return flags&VLC_CPU_MMXEXT;
}

static unsigned int cpu_hasSSE (unsigned int flags)
{
	return flags&VLC_CPU_SSE;
}

static unsigned int cpu_hasSSE2 (unsigned int flags)
{
	return flags&VLC_CPU_SSE2;
}

static unsigned int cpu_hasSSE3 (unsigned int flags)
{
	return flags&VLC_CPU_SSE3;
}

static unsigned int cpu_hasSSSE3 (unsigned int flags)
{
	return flags&VLC_CPU_SSSE3;
}

static unsigned int cpu_hasSSE4_1 (unsigned int flags)
{
	return flags&VLC_CPU_SSE4_1;
}

static unsigned int cpu_hasSSE4_2 (unsigned int flags)
{
	return flags&VLC_CPU_SSE4_2;
}

static unsigned int cpu_hasSSE4A (unsigned int flags)
{
	return flags&VLC_CPU_SSE4A;
}

char *cpu_getCapabilityString (char *strbuffer, const int blen)
{
	char *buffer = strbuffer;
	const unsigned int flags = vlc_CPU();

	*buffer = 0;
	
	if (cpu_hasMMX(flags))
		buffer = strncat(buffer, "MMX ", blen);
	
	if (cpu_hasMMXEXT(flags))
		buffer = strncat(buffer, "MMXExt ", blen);
	
	if (cpu_has3DNOW(flags))
		buffer = strncat(buffer, "3DNOW ", blen);
	
	if (cpu_hasSSE(flags))
		buffer = strncat(buffer, "SSE ", blen);
	
	if (cpu_hasSSE2(flags))
		buffer = strncat(buffer, "SSE2 ", blen);
		
	if (cpu_hasSSE3(flags))
		buffer = strncat(buffer, "SSE3 ", blen);
		
	if (cpu_hasSSE4_1(flags))
		buffer = strncat(buffer, "SSE4_1 ", blen);
	
	if (cpu_hasSSE4_2(flags))
		buffer = strncat(buffer, "SSE4_2 ", blen);
		
	if (cpu_hasSSE4A(flags))
		buffer = strncat(buffer, "SSE4A ", blen);
		
	if (cpu_hasSSSE3(flags))
		buffer = strncat(buffer, "SSSE3 ", blen);

	return strbuffer;
}


int cpuHasMMX ()
{
	return (cpu_hasMMX(vlc_CPU()) > 0);
}

int cpuHas3DNOW ()
{
	return (cpu_has3DNOW(vlc_CPU()) > 0);
}

int cpuHasMMXEXT ()
{
	return (cpu_hasMMXEXT(vlc_CPU()) > 0);
}

int cpuHasSSE ()
{
	return (cpu_hasSSE(vlc_CPU()) > 0);
}

int cpuHasSSE2 ()
{
	return (cpu_hasSSE2(vlc_CPU()) > 0);
}

int cpuHasSSE3 ()
{
	return (cpu_hasSSE3(vlc_CPU()) > 0);
}

int cpuHasSSSE3 ()
{
	return (cpu_hasSSSE3(vlc_CPU()) > 0);
}

int cpuHasSSE4_1 ()
{
	return (cpu_hasSSE4_1(vlc_CPU()) > 0);
}

int cpuHasSSE4_2 ()
{
	return (cpu_hasSSE4_2(vlc_CPU()) > 0);
}

int cpuHasSSE4A ()
{
	return (cpu_hasSSE4A(vlc_CPU()) > 0);
}

int cpuGetProcessorCount ()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return (int)si.dwNumberOfProcessors;
}

double cpuGetCoreUsage (TVLCPLAYER *vp, const int core)
{
	static double cpuCoreTimeOld[32];
	static double oldCoreTime[32];
	static int64_t idleTimeOld[32];
	static int64_t IdleTimeZero[32];
	
	if (!initNTQSIOnce){
		HANDLE hLibNTDLL = GetModuleHandle("ntdll");
		if (!hLibNTDLL) return -1.0;

		ntQuerySystemInformation = (PROCNTQSI)GetProcAddress(hLibNTDLL, "NtQuerySystemInformation");
		initNTQSIOnce = 1;
	}

	ULONG haveCpuData = 0;
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION info[32];
	ntQuerySystemInformation(SystemProcessorTimes, info, sizeof(info), &haveCpuData);


	//if (*(int64_t*)&info[core].IdleTime == IdleTimeZero[core]) return cpuCoreTimeOld[core];
	IdleTimeZero[core] = *(int64_t*)&info[core].IdleTime;

	uint64_t coreTime64;
	QueryPerformanceCounter((LARGE_INTEGER*)&coreTime64);
	double coreTime = coreTime64 / (double)vp->freq;
	
	double coreTimeDelta = coreTime - oldCoreTime[core];
	if (coreTimeDelta < 0.25) return cpuCoreTimeOld[core];
	oldCoreTime[core] = coreTime;

	int64_t idleTime = IdleTimeZero[core];
	int64_t idleTimeDelta = idleTime - idleTimeOld[core];
	
	//if (idleTimeDelta < 1000) return cpuCoreTimeOld[core];
	idleTimeOld[core] = idleTime;

	double cpuCoreTime = fabs(100.0-((idleTimeDelta/coreTimeDelta)/100000.0));
	cpuCoreTimeOld[core] = cpuCoreTime;
	
	if (cpuCoreTime < 0.0)
		cpuCoreTime = 0.0;
	else if (cpuCoreTime > 100.0)
		cpuCoreTime = 100.0;

	/*if (!core){
		printf("cpuGetCoreUsage %i %f\n", core, cpuCoreTime);
		fflush(stdout);
	}*/

	return cpuCoreTime;
}

double cpuGetProcessorUsage (TVLCPLAYER *vp)
{
	static double cpuCoreTimeOld;
	static double oldCoreTime;
	static int64_t idleTimeOld;
	//static int64_t IdleTimeZero;
	
	if (!initNTQSIOnce){
		HANDLE hLibNTDLL = GetModuleHandle("ntdll");
		if (!hLibNTDLL) return -1.0;

		ntQuerySystemInformation = (PROCNTQSI)GetProcAddress(hLibNTDLL, "NtQuerySystemInformation");
		initNTQSIOnce = 1;
	}

	ULONG haveCpuData = 0;
	SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION info[16];
	ntQuerySystemInformation(SystemProcessorTimes, info, sizeof(info), &haveCpuData);


	//if (*(int64_t*)&info[0].IdleTime == IdleTimeZero) return cpuCoreTimeOld;
	//IdleTimeZero = *(int64_t*)&info[0].IdleTime;
	
	const int count = haveCpuData / sizeof(info[0]);
	//printf("ret %i %i\n", ret, (int)count);
	

	//for (int i = 0; i < 6; i++)
	//	printf("%i: %I64d %I64d %I64d\n", i, *(int64_t*)&info[i].IdleTime, *(int64_t*)&info[i].KernelTime, *(int64_t*)&info[i].UserTime);
	
	
	uint64_t coreTime64;// = getTime64(vp);
	QueryPerformanceCounter((LARGE_INTEGER*)&coreTime64);
	double coreTime = coreTime64 / (double)vp->freq;
	
	double coreTimeDelta = coreTime - oldCoreTime;
	if (coreTimeDelta < 0.25) return cpuCoreTimeOld;
	oldCoreTime = coreTime;

		
	int64_t idleTime = 0;
	for (int i = 0; i < count; i++)
		idleTime += *(int64_t*)&info[i].IdleTime;
	idleTime /= count;

	int64_t idleTimeDelta = idleTime - idleTimeOld;
	//if (idleTimeDelta < 1000) return cpuCoreTimeOld;
	idleTimeOld = idleTime;

	double cpuCoreTime = fabs(100.0-((idleTimeDelta/coreTimeDelta)/100000.0));
	cpuCoreTimeOld = cpuCoreTime;
	
	if (cpuCoreTime < 0.0)
		cpuCoreTime = 0.0;
	else if (cpuCoreTime > 100.0)
		cpuCoreTime = 100.0;
	return cpuCoreTime;
}

void cpuGetUpTime (date64_t *ut)
{
	unsigned long lintTicks = getTickCount();
	ut->seconds = (lintTicks / 1000) % 60;
	ut->minutes = ((lintTicks / 1000) / 60) % 60;
	ut->hours = (((lintTicks / 1000) / 60) / 60) % 24;
	ut->days = ((((lintTicks / 1000) / 60) / 60) / 24) % 7;
    ut->weeks = (((((lintTicks / 1000) / 60) / 60) / 24) / 7) % 52;
}


