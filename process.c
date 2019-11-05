
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
#include <time.h>



HANDLE processGetCurrent ()
{
	return GetCurrentProcess();
}

HANDLE processOpen (const int pid)
{
	return OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ/*|PROCESS_QUERY_LIMITED_INFORMATION*/, FALSE, pid);
}

void processClose (HANDLE hProcess)
{
	CloseHandle(hProcess);
}

int processGetId ()
{
	return _getpid(); 		//	GetCurrentProcessId();
}

int processCreateW (const wchar_t *cmdLine)
{
	PROCESS_INFORMATION pi = {0};
	STARTUPINFOW si = {0};
	si.cb = sizeof(si);

	resetCurrentDirectory();
	CreateProcessW(NULL, (wchar_t*)cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);

	//printf("processCreateW: %p %p %i\n", pi.hProcess, pi.hThread, (int)pi.dwProcessId);

	/*printf("waiting..\n");
	int ret = (int)WaitForSingleObject(pi.hThread, INFINITE);
	printf("waiting.. done %i\n", ret == WAIT_OBJECT_0);
	*/
	return pi.dwProcessId;
}

int processCreate (const char *cmdLine)
{
	int pid = 0;
	wchar_t *path = converttow(cmdLine);
	if (path){
		pid = processCreateW(path);
		my_free(path);
	}

	return pid;
}

static inline int processSetPrivilege (HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege)
{
	LUID luid;
	if (!LookupPrivilegeValue(NULL, Privilege, &luid))
		return FALSE;

	TOKEN_PRIVILEGES tp = {0};
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;

	if (bEnablePrivilege)
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED|SE_PRIVILEGE_USED_FOR_ACCESS;
	else
		tp.Privileges[0].Attributes = 0;

	DWORD cb = sizeof(TOKEN_PRIVILEGES);
	AdjustTokenPrivileges(hToken, FALSE, &tp, cb, NULL, NULL);
	if (GetLastError() != ERROR_SUCCESS)
		return FALSE;

	return TRUE;
}

static inline HANDLE processElevateEnable ()
{

	HANDLE hToken = NULL;
	if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken)){
		if (GetLastError() == ERROR_NO_TOKEN){
			if (!ImpersonateSelf(SecurityImpersonation))
				return NULL;

			if (!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES|TOKEN_IMPERSONATE | TOKEN_QUERY, FALSE, &hToken))
            	return NULL;
		}else{
            return NULL;
		}
	}
    if (!processSetPrivilege(hToken, SE_DEBUG_NAME, TRUE)){
		CloseHandle(hToken);
        return NULL;
    }

    return hToken;
}

static inline void processElevateDisable (HANDLE hToken)
{
	if (hToken){
		processSetPrivilege(hToken, SE_DEBUG_NAME, FALSE);
		CloseHandle(hToken);
	}
}

int processKillOpenThread (const int pid)
{

	HANDLE hToken = processElevateEnable();

	HANDLE hProcess = NULL;
	if ((hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid)) == NULL){
		CloseHandle(hToken);
		return 0;
	}

    processElevateDisable(hToken);

	int success = TerminateProcess(hProcess, 0xffffffff);
    CloseHandle(hProcess);
    return success;
}

int processKillRemoteThread (const int pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, TRUE, pid);
	if (hProcess){
		DWORD dwThreadId = 0;
		HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,(void*)GetProcAddress(GetModuleHandleA("KERNEL32.DLL"),"ExitProcess"),0,0, &dwThreadId);
		if (hThread){
			WaitForSingleObject(hThread, 6);
			CloseHandle(hThread);
			CloseHandle(hProcess);
			return 1;
		}else{
			if (TerminateProcess(hProcess, 0xffffffff)){
				CloseHandle(hProcess);
				return 1;
			}
			CloseHandle(hProcess);
			return 0;
		}
	}
	return 0;
}

int processKillWindow (const int pid)
{

	HANDLE hwnd = FindWindow(0,0);
	while (hwnd){
 		if (!GetParent(hwnd)){
			int test_pid = processGetWindowThreadPid(hwnd);
     		if (test_pid == pid){
				if (IsWindow(hwnd)){
     				PostThreadMessage(GetWindowThreadProcessId(hwnd, NULL), WM_CLOSE, 0, 0);
     				Sleep(10);
	     			PostThreadMessage(GetWindowThreadProcessId(hwnd, NULL), WM_QUIT, 0, 0);
	     			Sleep(1);
	     		}
	     		if (IsWindow(hwnd)){
     				PostMessage(hwnd, WM_DESTROY, 0, 0);
     				Sleep(1);
     			}
     			if (IsWindow(hwnd)){
     				PostMessage(hwnd, WM_QUIT, 0, 0);
     				Sleep(1);
     			}
     			if (IsWindow(hwnd)){
     				PostMessage(hwnd, WM_CLOSE, 0, 0);
     				Sleep(1);
     			}
     			if (IsWindow(hwnd)){
     				PostMessage(hwnd, SC_CLOSE, 0, 0);
     				Sleep(1);
     			}
     			if (IsWindow(hwnd)){
     				CloseWindow(hwnd);
     				Sleep(1);
     			}
				return IsWindow(hwnd);
     		}
  		}
  		hwnd = GetWindow(hwnd, GW_HWNDNEXT);
 	}
 	return 0;
}

int processGetPid (const char *exeModule)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32 proc;
	proc.dwSize = sizeof(PROCESSENTRY32);

	Process32First(hSnap, &proc);
	while(Process32Next(hSnap, &proc)){
		//printf("'%s'\n", proc.szExeFile);
		if (stristr(proc.szExeFile, exeModule))
			return (int)proc.th32ProcessID;
	};

	CloseHandle(hSnap);
	return 0;
}

int processGetPidW (const wchar_t *exeModule)
{
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap == INVALID_HANDLE_VALUE)
		return 0;

	PROCESSENTRY32W proc;
	proc.dwSize = sizeof(PROCESSENTRY32);

	Process32FirstW(hSnap, &proc);
	while(Process32NextW(hSnap, &proc)){
		//printf("'%s'\n", proc.szExeFile);
		if (wcsistr(proc.szExeFile, exeModule))
			return (int)proc.th32ProcessID;
	};

	CloseHandle(hSnap);
	return 0;
}

uint64_t processGetMemUsage (const int pid)
{
    PROCESS_MEMORY_COUNTERS pmc;

	uint64_t mem = 0;
    HANDLE hProcess = processOpen(pid);
    if (hProcess){
    	pmc.WorkingSetSize = 0;
    	//pmc.PrivateUsage = 0;
    	if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    		mem = (uint64_t)pmc.WorkingSetSize;
    		//mem = (uint64_t)pmc.PrivateUsage;
    	processClose(hProcess);
	}


	/*
	GetProcessTimes(
    IN HANDLE hProcess,
    OUT LPFILETIME lpCreationTime,
    OUT LPFILETIME lpExitTime,
    OUT LPFILETIME lpKernelTime,
    OUT LPFILETIME lpUserTime
    );
	*/
    return mem;
}

void processSetPriority (const int classLevel)
{
	SetPriorityClass(processGetCurrent(), classLevel);
}

void processSetErrorMode (const int mode)
{
	SetErrorMode(mode);
}

int processGetWindowThreadPid (HANDLE hWnd)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(hWnd, &pid);
	return (int)pid;
}

wchar_t *processGetFilename (const int pid)
{
	HANDLE hProcess = processOpen(pid);
  	if (hProcess){
		wchar_t path[MAX_PATH+1] = {0};
		GetProcessImageFileNameW(hProcess, path, MAX_PATH);
		processClose(hProcess);
		if (*path) return my_wcsdup(path);
	}

	return NULL;
}

/*
  typedef struct tagPROCESSENTRY32W {
    DWORD dwSize;
    DWORD cntUsage;
    DWORD th32ProcessID;
    ULONG_PTR th32DefaultHeapID;
    DWORD th32ModuleID;
    DWORD cntThreads;
    DWORD th32ParentProcessID;
    LONG pcPriClassBase;
    DWORD dwFlags;
    WCHAR szExeFile[MAX_PATH];
  } PROCESSENTRY32W;


typedef struct process_list {
	PROCESSENTRY32W entry;
	struct process_list *next;
}process_list;


*/


static inline void freeProclist (process_list *list)
{
	while (list){
		process_list *next = list->next;
		my_free(list);
		list = next;
	}
}

static inline int getProcTotal (const HANDLE hSnap)
{
	int total = 0;
	PROCESSENTRY32W proc;
	proc.dwSize = sizeof(PROCESSENTRY32);

	Process32FirstW(hSnap, &proc);
	while(Process32NextW(hSnap, &proc))
		total++;

	return total;
}

static inline int getProclist (HANDLE hSnap, process_list *list)
{
	int i = 0;
	list->entry.dwSize = sizeof(list->entry);
	if (!list->next)
		list->next = my_calloc(1, sizeof(*list));

	if (Process32FirstW(hSnap, &list->entry)){
		while(Process32NextW(hSnap, &list->entry)){

			//wprintf(L"%i, '%s'\n", i, list->entry.szExeFile);

			if (!list->next)
				list->next = my_calloc(1, sizeof(*list));

			list = list->next;
			list->entry.dwSize = sizeof(list->entry);
			i++;
		}
	}

	return i;
}

process_list *processGetProcessList (int *total)
{
	if (total) *total = 0;
	process_list *list = NULL;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap){
		list = my_calloc(1, sizeof(*list));
		if (list){
			int listtotal = getProclist(hSnap, list);
			if (total) *total = listtotal;
		}
		CloseHandle(hSnap);
	}
	return list;
}

void processFreeProcessList (process_list *list)
{
	freeProclist(list);
}

static inline void freeModulelist (module_list *list)
{
	while (list){
		module_list *next = list->next;
		my_free(list);
		list = next;
	}
}

static inline int getModuleTotal (const HANDLE hSnap)
{
	int total = 0;
	MODULEENTRY32W mod;
	mod.dwSize = sizeof(PROCESSENTRY32);

	Module32FirstW(hSnap, &mod);
	while(Module32NextW(hSnap, &mod))
		total++;

	return total;
}

static inline int getModulelist (HANDLE hSnap, module_list *list)
{
	int i = 0;
	list->entry.dwSize = sizeof(list->entry);
	if (!list->next)
		list->next = my_calloc(1, sizeof(module_list));

	if (Module32FirstW(hSnap, &list->entry)){
		while(Module32NextW(hSnap, &list->entry)){
			if (!list->next)
				list->next = my_calloc(1, sizeof(*list));

			list = list->next;
			list->entry.dwSize = sizeof(list->entry);
			i++;
		}
	}
	return i;
}

module_list *processGetModuleList (const int pid, int *total)
{
	if (total) *total = 0;
	module_list *list = NULL;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
	if (hSnap){
		list = my_calloc(1, sizeof(*list));
		if (list){
			int listtotal = getModulelist(hSnap, list);
			if (total) *total = listtotal;
		}
		CloseHandle(hSnap);
	}
	return list;
}

void processFreeModuleList (module_list *list)
{
	freeModulelist(list);
}

int processGetMemoryStats (const int pid, PROCESS_MEMORY_COUNTERS *pmc)
{
	HANDLE hProcess = processOpen(pid);
    if (NULL == hProcess) return 0;

	memset(pmc, 0, sizeof(*pmc));
    GetProcessMemoryInfo(hProcess, pmc, sizeof(*pmc));
    CloseHandle(hProcess);

   	return 1;
}

uint64_t processGetMemoryWorkingSize (const int pid)
{

    HANDLE hProcess = processOpen(pid);
    if (NULL == hProcess) return 0;

    PROCESS_MEMORY_COUNTERS pmc = {.WorkingSetSize = 0};
    GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc));
    CloseHandle(hProcess);

   	return pmc.WorkingSetSize;
}

int processQueryVariable (
     HANDLE ProcessHandle,
     PROCESSINFOCLASS ProcessInformationClass,
     PVOID *Buffer
    )
{
    NTSTATUS status;
    PVOID buffer;
    ULONG returnLength = 0;

    NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        NULL,
        0,
        &returnLength
        );
    buffer = my_calloc(3, returnLength);
    status = NtQueryInformationProcess(
        ProcessHandle,
        ProcessInformationClass,
        buffer,
        returnLength,
        &returnLength
        );

    if (NT_SUCCESS(status))
    {
        *Buffer = buffer;
    }
    else
    {
        my_free(buffer);
    }

    return (int)status;
}


static inline process_list_extended *getProcListExtended (int *procTotal)
{

	HANDLE hToken = processElevateEnable();

	*procTotal = 0;
	const int tCores = cpuGetProcessorCount();
	ULONG bufferlen = 0;
	NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &bufferlen);
	if (!bufferlen) return NULL;
	unsigned char *info = my_calloc(1, bufferlen+1);
	if (!info) return NULL;

	NtQuerySystemInformation(SystemProcessInformation, info, bufferlen, &bufferlen);
	unsigned char *pinfo = (unsigned char*)info;

	int count = 0;
	for (int i = 0; i < 256; i++){
		SYSTEM_PROCESS_INFORMATION *spi = (SYSTEM_PROCESS_INFORMATION*)pinfo;
		count++;
		if (!spi->NextEntryOffset) break;
		pinfo += spi->NextEntryOffset;
	}


	process_list_extended *ple = my_calloc(count+1, sizeof(*ple));
	process_list_extended *proc = ple;
	pinfo = (unsigned char*)info;

	for (int i = 0; i < count; i++){
		SYSTEM_PROCESS_INFORMATION *spi = (SYSTEM_PROCESS_INFORMATION*)pinfo;

		proc->info.createTime = *(uint64_t*)&spi->CreateTime;
		proc->info.userTime = *(uint64_t*)&spi->UserTime;
		proc->info.kernelTime = *(uint64_t*)&spi->KernelTime;
		proc->info.sumTime = proc->info.userTime + proc->info.kernelTime;
		proc->info.totalThreads = spi->NumberOfThreads;
		proc->info.processId = (intptr_t)spi->UniqueProcessId;
		if (!proc->info.processId)
			proc->info.imageName = my_wcsdup(L"Idle Process");
		else
			proc->info.imageName = my_wcsdup(spi->ImageName.Buffer);

		proc->info.basePriority = spi->BasePriority;
		proc->info.inheritedFromProcessId = (intptr_t)spi->InheritedFromUniqueProcessId;
		proc->info.totalHandles = spi->HandleCount;
		//proc->info.privatePageCount = spi->PrivatePageCount;
		my_memcpy(&proc->info.vm_counters, &spi->VirtualMemoryCounters, sizeof(spi->VirtualMemoryCounters));
		my_memcpy(&proc->info.io_counters, &spi->IoCounters, sizeof(spi->IoCounters));

		if (!proc->info.processId){
			proc->info.path = my_wcsdup(proc->info.imageName);
		}else{
			HANDLE hprocess = processOpen(proc->info.processId);
			if (hprocess){
				/*KERNEL_USER_TIMES kut;
				ULONG len = 0;
				NtQueryInformationProcess(hprocess, ProcessTimes, &kut, sizeof(KERNEL_USER_TIMES), &len);
				uint64_t kernelTime = *(uint64_t*)&kut.KernelTime;
    			uint64_t userTime = *(uint64_t*)&kut.UserTime;
				uint64_t ttime = userTime + kernelTime;
				printf("%i: %I64d %I64d %I64d\n", proc->info.processId, kernelTime, userTime, ttime-proc->info.sumTime);*/

				int len = MAX_PATH+8;
				wchar_t buffer[len];
				STRING *str = (STRING*)buffer;
				NtQueryInformationProcess(hprocess, ProcessImageFileNameWin32, str, len, (ULONG*)&len);
				proc->info.path = my_wcsdup((wchar_t*)str->Buffer);
				//wprintf(L"len %i '%s'\n", str->Length, proc->info.path);

				//char *varTmp = NULL;
				/*UNICODE_STRING *varTmp = NULL;
				processQueryVariable(hprocess, ProcessCommandLineInformation, (void*)&varTmp);
				if (varTmp)
					wprintf(L":: %i %i, '%s'\n", proc->info.processId, varTmp->Length, varTmp->Buffer);*/

#if ENABLE_PHSTUFF
				HANDLE tokenHandle;
				NTSTATUS status = OpenProcessToken(&tokenHandle, TOKEN_QUERY, hprocess);
				printf("OpenProcessToken %i\n", (int)status);
				if (NT_SUCCESS(status)){
					// User name
            		{
                		PTOKEN_USER user = NULL;
                		status = PhGetTokenUser(tokenHandle, &user);
                		printf("PhGetTokenUser %i\n", (int)status);

                		if (NT_SUCCESS(status)){
                    		wchar_t *UserName = PhGetSidFullName(user->User.Sid, TRUE, NULL);
                    		__mingw_wprintf(L"username '%ls'\n", UserName);
                    		free(UserName);
                		}
            		}

            		NtClose(tokenHandle);
        		}
#endif

				/*DWORD lpcchReturnLength;
				GetVolumePathNamesForVolumeNameW((LPCWSTR)fileNameTmp, buffer, MAX_PATH, &lpcchReturnLength);
				wprintf(L"-- '%s'\n", buffer);*/


				DWORD group = 0;
				GetProcessAffinityMask(hprocess, (DWORD*)&proc->info.affinity.mask, &group);
				processClose(hprocess);

				for (int i = 0; i < 32; i++)
					proc->info.affinity.total += ((proc->info.affinity.mask>>i)&0x01);

				//printf("## %i: %X %i\n", proc->info.processId, proc->info.affinity.mask, proc->info.affinity.total);
			}else{
				//wprintf(L"procesSOpen FAILED %i '%s'\n", proc->info.processId, proc->info.imageName);

				//proc->info.path = my_calloc(1, 2048);
				//_snwprintf(proc->info.path, 2048, L"n:\\windows\\system32\\%s", proc->info.imageName);
				proc->info.path = my_wcsdup(proc->info.imageName);
			}
		}


		if (proc->info.imageName)
			proc->info.imageName8 = convertto8(proc->info.imageName);

		if (proc->info.affinity.total < 1)
			proc->info.affinity.total = tCores;

		const FILETIME ft = *(FILETIME*)&proc->info.sumTime;
		FileTimeToSystemTime(&ft, &proc->time);

		//wprintf(L"%i: pid:%i thrds:%i '%s', %.2i:%.2i:%.2i.%.3i\n", i, (int)spi->UniqueProcessId, (int)spi->NumberOfThreads, spi->ImageName.Buffer, proc->time.wHour, proc->time.wMinute, proc->time.wSecond, proc->time.wMilliseconds);

		proc++;
		if (!spi->NextEntryOffset) break;
		pinfo += spi->NextEntryOffset;
	}

	processElevateDisable(hToken);

	proc->info.processId = -666;
	*procTotal = count;
	my_free(info);
	return ple;
}

process_list_extended *processGetProcessListExtended (int *count)
{
	int total;
	uint64_t snapshotTime;
	uint64_t freq;
	//enablePrivileges();

	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	QueryPerformanceCounter((LARGE_INTEGER*)&snapshotTime);
	process_list_extended *ple = getProcListExtended(&total);
	//printf("processGetProcessListExtended: %p %I64d %I64d\n", ple, freq, snapshotTime);

	if (ple){
		ple->snapshotTime = snapshotTime;
		ple->freq = freq;

	}
	if (count) *count = total;

	return ple;
}

void processFreeProcessListExtended (process_list_extended *list)
{
	for (int i = 0; list[i].info.processId >= 0; i++){
		process_list_extended *ple = &list[i];
		if (ple){
			if (ple->info.imageName)
				my_free(ple->info.imageName);
			if (ple->info.imageName8)
				my_free(ple->info.imageName8);
			if (ple->info.path)
				my_free(ple->info.path);
		}
	}

	my_free(list);
}

static inline const process_list_extended *processGetProcessExtended (const process_list_extended *ple, const int pid)
{
	for (int i = 0; ple->info.processId >= 0; i++, ple++){
		if (ple->info.processId == pid)
			return ple;
	}

	return NULL;
}

const process_list_extended *processFindProcess (const process_list_extended *ple, const int pid)
{
	return processGetProcessExtended(ple, pid);
}

// dst = dst - src
int processSubtractProcessList (process_list_extended *dst, const process_list_extended *src)
{
	const float t1 = src->snapshotTime / (float)src->freq;
	const float t2 = dst->snapshotTime / (float)dst->freq;
	dst->multiplier = 1.0f / (t2 - t1);

	int count;
	for (count = 0; dst->info.processId >= 0; count++, dst++){
		const process_list_extended *ple = processGetProcessExtended(src, dst->info.processId);
		if (ple){
			dst->info.deltaTime = (dst->info.sumTime - ple->info.sumTime) / dst->info.affinity.total;

			const FILETIME ft = *(FILETIME*)&dst->info.deltaTime;
			FileTimeToSystemTime(&ft, &dst->time);
		}
	}
	return count;
}

// dst = dst * n
int processMultiplyProcessList (process_list_extended *dst, const float value)
{

	int count;
	for (count = 0; dst->info.processId >= 0; count++, dst++){
		dst->info.deltaTime *= value;

		const FILETIME ft = *(FILETIME*)&dst->info.deltaTime;
		FileTimeToSystemTime(&ft, &dst->time);
		dst->cpuTime = dst->time.wMilliseconds / 10.0f;
	}
	return count;
}

