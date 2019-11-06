
// libmylcd
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
#include "reg.h"
#include <gdiplus.h>


#if 0
#include <ddk/ntddstor.h>
#include <shellapi.h>
#else
#include <ntddstor.h>
#endif




#define MY_COMPUTER_LOCALE	MY_COMPUTER_2K

static int FileID = 1024;
#define MAKEID	(FileID++)






typedef struct {
	uint64_t data64;
	int id;
	int type;
	char string[MAX_PATH_UTF8+1];
}TFBSORT;




static inline int getDriveBusType (const int drive)
{
	wchar_t buffer[128];
	__mingw_snwprintf(buffer, 127, L"\\\\.\\%lc:", (wchar_t)(drive&0xFF));
	
	HANDLE deviceHandle = CreateFileW(
    	buffer,
		0,                // no access to the drive
		FILE_SHARE_READ | FILE_SHARE_WRITE,  // share mode
		NULL,             // default security attributes
		OPEN_EXISTING,    // disposition
		0,                // file attributes
		NULL);            // do not copy file attributes

	if (deviceHandle == INVALID_HANDLE_VALUE) return BusTypeUnknown;

	// setup query
	STORAGE_PROPERTY_QUERY query;
	memset(&query, 0, sizeof(query));
	query.PropertyId = StorageDeviceProperty;
	query.QueryType = PropertyStandardQuery;

	// issue query
	STORAGE_DEVICE_DESCRIPTOR devd;

	DWORD bytes;
	STORAGE_BUS_TYPE busType = BusTypeUnknown;


	if (DeviceIoControl(deviceHandle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &devd, sizeof(devd), &bytes, NULL))
		busType = devd.BusType;

	CloseHandle(deviceHandle);

	//printf("bustype: %c: %i %X\n", drive&0xFF, devd.RemovableMedia, devd.BusType);

	return busType;
}

int fbIsUsbDrive (const char drive)
{
	return getDriveBusType(drive) == BusTypeUsb;
}

static inline TDECOMPOSEPATH *decomposeDirectoryAlloc (const int size)
{
	TDECOMPOSEPATH *decomp = my_malloc(sizeof(TDECOMPOSEPATH));
	if (decomp){
		decomp->size = size+1;
		decomp->dirs = my_calloc(decomp->size, sizeof(TDECOMPOSEDIR));
		decomp->total = 0;
	}
	return decomp;
}

void decomposePathFree (TDECOMPOSEPATH *decomp)
{
	while(decomp->total--){
		my_free(decomp->dirs[decomp->total].dirComplete);
		my_free(decomp->dirs[decomp->total].folder);
		my_free(decomp->dirs[decomp->total].dir);
		my_free(decomp->dirs[decomp->total].drive);
	}
	
	my_free(decomp->dirs);
	my_free(decomp);
}

static inline int decomposeDirectoryAdd (TDECOMPOSEPATH *decomp, const char *srcPath)
{
	char *path = my_strdup(srcPath);
	const int len = strlen(path);
	if (path[len-1] == '\\') path[len-1] = 0;
	else if (path[len-1] == '/') path[len-1] = 0;
	
	char folder[MAX_PATH_UTF8+1];
	char dir[MAX_PATH_UTF8+1];
	char drive[64];
	
	_splitpath(path, drive, dir, folder, NULL);

	decomp->dirs[decomp->total].folder = my_strdup(folder);
	decomp->dirs[decomp->total].drive = my_strdup(drive);
	decomp->dirs[decomp->total].dir = my_strdup(dir);
	if (!dir[0])
		decomp->dirs[decomp->total].folderType = FB_OBJTYPE_DRIVE;
	else
		decomp->dirs[decomp->total].folderType = FB_OBJTYPE_FOLDER;
	
	__mingw_snprintf(dir, MAX_PATH_UTF8, "%s\\", path);
	decomp->dirs[decomp->total].dirComplete = my_strdup(dir);

	return ++decomp->total;
}

static inline int decomposeDirectory (TDECOMPOSEPATH *decomp, char *srcDir)
{
	
	char *dir = my_strdup(srcDir);
	const int len = strlen(dir);
	if (dir[len-1] == '\\') dir[len-1] = 0;
	else if (dir[len-1] == '/') dir[len-1] = 0;
	
	char *last;
	char *lastB = strrchr(dir, '\\');
	char *lastF = strrchr(dir, '/');
	if (lastF > lastB)
		last = lastF;
	else
		last = lastB;
	
	if (last){
		//printf("ADD '%s'\n", dir);
		*last = 0;
		decomposeDirectory(decomp, dir);
	}
	
	int total = decomposeDirectoryAdd(decomp, srcDir);
	
	
	//printf("srcDir '%s'\n", srcDir);
	my_free(dir);
	
	return total;
}

TDECOMPOSEPATH *decomposePath (char *path)
{
	int total = countChr(path, '\\');
	total += countChr(path, '/');
	
	TDECOMPOSEPATH *decomp = decomposeDirectoryAlloc(total);
	total = decomposeDirectory(decomp, path);
/*
	for (int i = 0; i < decomp->total; i++){
		printf("\ndirComplete '%s'\n", decomp->dirs[i].dirComplete);
		printf("folder '%s'\n", decomp->dirs[i].folder);
		printf("dir '%s'\n", decomp->dirs[i].dir);
		printf("drive '%s'\n", decomp->dirs[i].drive);
	}
*/
	if (!total){
		decomposePathFree(decomp);
		return NULL;
	}
	
	return decomp;
}

static inline char tlower (const char b)
{
	if (b >= 'A' && b <= 'Z')
		return b - 'A' + 'a';
	else
		return b;
}

static inline char isnum (const char b)
{
	if (b >= '0' && b <= '9')
		return 1;
	else
		return 0;
}

static inline int parsenum (const char *a)
{
	int result = *a - '0';
	++a;

	while (isnum(*a)){
		result *= 10;
		result += *a - '0';
		++a;
	}

	--a;
	return result;
}

// http://stereopsis.com/strcmp4humans.html
static inline int stringCompare (const char *a, const char *b)
{
	if (a == b) return 0;
	if (a == NULL) return -1;
	if (b == NULL) return 1;

	while (*a && *b){
		int a0, b0;

		if (isnum(*a))
			a0 = parsenum(a) + 256;
		else
			a0 = tlower(*a);

		if (isnum(*b))
			b0 = parsenum(b) + 256;
		else
			b0 = tlower(*b);

		if (a0 < b0) return -1;
		if (a0 > b0) return 1;
		++a;
		++b;
	}

	if (*a) return 1;
	if (*b) return -1;

	return 0;
}

static inline int fbQsStringA (const void *a, const void *b)
{
	TFBSORT *item1 = (TFBSORT*)a;
	TFBSORT *item2 = (TFBSORT*)b;
	
	
	if (item1->type == TREE_TYPE_BRANCH && item2->type == TREE_TYPE_LEAF)
		return -1;
	if (item1->type == TREE_TYPE_LEAF && item2->type == TREE_TYPE_BRANCH)
		return 1;
	else
		return stringCompare(item1->string, item2->string);
}

static inline int fbQsStringD (const void *a, const void *b)
{
	TFBSORT *item1 = (TFBSORT*)a;
	TFBSORT *item2 = (TFBSORT*)b;
	

	if (item1->type == TREE_TYPE_BRANCH && item2->type == TREE_TYPE_LEAF)
		return -1;
	else if (item1->type == TREE_TYPE_LEAF && item2->type == TREE_TYPE_BRANCH)
		return 1;
	else
		return stringCompare(item2->string, item1->string);
}

static inline int fbQsData64A (const void *a, const void *b)
{
	TFBSORT *item1 = (TFBSORT*)a;
	TFBSORT *item2 = (TFBSORT*)b;
	//printf("qs: %i %i, %I64d %I64d\n", item1->id, item2->id, item1->data64, item2->data64);


	if (item1->type == TREE_TYPE_LEAF && item2->type == TREE_TYPE_LEAF){
		if (item1->data64 < item2->data64)
			return -1;
		else if (item1->data64 > item2->data64)
			return 1;
	}else if (item1->type == TREE_TYPE_BRANCH && item2->type == TREE_TYPE_LEAF){
		return -1;
	}else if (item1->type == TREE_TYPE_LEAF && item2->type == TREE_TYPE_BRANCH){
		return 1;
	}else if (item1->type == TREE_TYPE_BRANCH && item2->type == TREE_TYPE_BRANCH){
		if (item1->data64 < item2->data64)
			return -1;
		else if (item1->data64 > item2->data64)
			return 1;
	}
	return 0;
}

static inline int fbQsData64D (const void *a, const void *b)
{
	TFBSORT *item1 = (TFBSORT*)a;
	TFBSORT *item2 = (TFBSORT*)b;
	//printf("qs: %i %i, %I64d %I64d\n", item1->id, item2->id, item1->data64, item2->data64);

	if (item1->type == TREE_TYPE_LEAF && item2->type == TREE_TYPE_LEAF){
		if (item1->data64 < item2->data64)
			return 1;
		else if (item1->data64 > item2->data64)
			return -1;
	}else if (item1->type == TREE_TYPE_BRANCH && item2->type == TREE_TYPE_LEAF){
		return -1;
	}else if (item1->type == TREE_TYPE_LEAF && item2->type == TREE_TYPE_BRANCH){
		return 1;
	}else if (item1->type == TREE_TYPE_BRANCH && item2->type == TREE_TYPE_BRANCH){
		if (item1->data64 < item2->data64)
			return 1;
		else if (item1->data64 > item2->data64)
			return -1;
	}
	return 0;
}

static inline void fbQsRelease (TFBSORT *sort)
{
	my_free(sort);
}

static inline TFBSORT *fbQsBuild (TTREE *tree, const int id, int *listTotal, const int sortType)
{
	const int total = treeCountItems(tree, id);
	if (total < 2) return NULL;
	
	TTREEENTRY *entry = treeEntryFind(tree->root, id);
	if (!entry) return NULL;
	
	TFBSORT *sortlist = my_calloc(total, sizeof(TFBSORT));
	if (sortlist){
		int ct = 0;
		TFBITEM *item = entry->head;

		while(item && ct < total){
			TTREEENTRY *subentry = treeListGetSubEntry(item);
			TFB_ITEM_DESC *desc = treeEntryGetStorage(subentry);
			sortlist[ct].id = subentry->id;
			sortlist[ct].type = subentry->type;
			
			switch (sortType){
		  	case SORT_DATE_MODIFIED_A:
		  	case SORT_DATE_MODIFIED_D:
				sortlist[ct].data64 = desc->modifiedDate;
				break;
		  	case SORT_DATE_CREATION_A:
		  	case SORT_DATE_CREATION_D:
		  		sortlist[ct].data64 = desc->creationDate;
				break;
			case SORT_SIZE_FILE_A:
		  	case SORT_SIZE_FILE_D:
		  		sortlist[ct].data64 = desc->fileSize;
				break;
		  	case SORT_NAME_A:
		  	case SORT_NAME_D:
		  		strncpy(sortlist[ct].string, subentry->name, MAX_PATH_UTF8);
				break;
			case SORT_FILE_TYPE_A:
		  	case SORT_FILE_TYPE_D:{
		  		char *ext = strrchr(subentry->name, '.');
		  		if (ext)
					strncpy(sortlist[ct].string, ext, MAX_PATH_UTF8);
		  		else if (treeEntryIsBranch(subentry))
		  			strncpy(sortlist[ct].string, subentry->name, MAX_PATH_UTF8);
		  		else
		  			strcpy(sortlist[ct].string, " ");

			  	break;
			}
		  	case SORT_NOSORT:
		  	default:
				break;
			};
			
			ct++;
			item = item->next;
		}
		*listTotal = ct;
	}
	return sortlist;
}

int fbSort (TFB *fb, const int id, const int sortType)
{
	TTREE *tree = fb->tree;
	int total = 0;

	TFBSORT *sortlist = fbQsBuild(tree, id, &total, sortType);
	if (sortlist){
		
		switch (sortType){
		  case SORT_NAME_A:
		  case SORT_FILE_TYPE_A:
		  	qsort(sortlist, total, sizeof(TFBSORT), fbQsStringA);
		  	break;
		  	
		  case SORT_NAME_D:
		  case SORT_FILE_TYPE_D:
		  	qsort(sortlist, total, sizeof(TFBSORT), fbQsStringD);
			break;
			
		  case SORT_DATE_MODIFIED_A:
		  case SORT_DATE_CREATION_A:
		  case SORT_SIZE_FILE_A:
			qsort(sortlist, total, sizeof(TFBSORT), fbQsData64A);
			break;

		  case SORT_DATE_MODIFIED_D:
		  case SORT_DATE_CREATION_D:
		  case SORT_SIZE_FILE_D:
			qsort(sortlist, total, sizeof(TFBSORT), fbQsData64D);
			break;
			
		  case SORT_NOSORT:
		  default:
			break;
		};
		
		for (int i = total-2; i >= 0; i--)
			treeEntryMove(tree, sortlist[i].id, sortlist[i+1].id);

		fbQsRelease(sortlist);
	}

	return total;
}


static inline TFB_ITEM_DESC *fbItemDescAlloc ()
{
	return my_calloc(1, sizeof(TFB_ITEM_DESC));
}

TFB *fbNew ()
{
	TFB *fb = my_calloc(1, sizeof(TFB));
	if (fb) fb->refCt = 1;
	return fb;
}

int fbInit (TFB *fb, const char *root)
{
	fb->rootId = MAKEID;
	fb->rootName = my_strdup(root);

	fb->tree = treeCreate(root, fb->rootId);
	treeSetStorage(fb->tree, fb->rootId, fbItemDescAlloc());
	return fb->rootId;
}

void fbClose (TFB *fb)
{
	if (!--fb->refCt){
		if (fb->rootName){
			my_free(fb->rootName);
			fb->rootName = NULL;
		}
		if (fb->tree){
			treeFree(fb->tree);
			fb->tree = NULL;
		}
	}
}

void fbRelease (TFB *fb)
{
	fbClose(fb);
	if (!fb->refCt) my_free(fb);
}

static inline int hasExt (const wchar_t *path, wchar_t **exts)
{
	const int slen = wcslen(path);

	for (int i = 0; exts[i] && *exts[i] == L'.'; i++){
		int elen = wcslen(exts[i]);
		if (elen > slen) continue;
		
		if (*(path+slen-elen) == L'.'){
			if (!wcsicmp(path+slen-elen, exts[i]))
				return 1;
		}
	}
	return 0;
}

int fbGetTotals (TFB *fb, const int id, int *tBranch, int *tLeaf)
{
	TTREEENTRY *entry = treeEntryFind(fb->tree->root, id);
	if (!entry) return 0;
	
	int branchTotal = 0;
	int leafTotal = 0;
	
	TFBITEM *item = entry->head;
	while(item){
		if (treeEntryIsBranch(treeListGetSubEntry(item)))
			branchTotal++;
		else
			leafTotal++;
		item = item->next;
	}

	if (tBranch) *tBranch = branchTotal;
	if (tLeaf) *tLeaf = leafTotal;

	return branchTotal + leafTotal;
}


static inline int fbAddDirectory (TFB *fb, const int nodeId, wchar_t *name, const WIN32_FIND_DATAW *ffd, const int id)
{
	//char convbuffer[MAX_PATH_UTF8+1];
	
	//if (UTF16ToUTF8(name, wcslen(name), convbuffer, MAX_PATH_UTF8)){
	char *name8 = convertto8(name);
	if (name8){
		TTREEENTRY *entry = treeAddItem(fb->tree, nodeId, name8, id, TREE_TYPE_BRANCH);
		if (entry){
			TFB_ITEM_DESC *desc = fbItemDescAlloc();
			if (desc){
				desc->fileAttributes = ffd->dwFileAttributes;
				desc->creationDate = (uint64_t)*(uint64_t*)&ffd->ftCreationTime.dwLowDateTime;
				desc->modifiedDate = (uint64_t)*(uint64_t*)&ffd->ftLastWriteTime.dwLowDateTime;
				treeEntrySetStorage(entry, desc);
			}
		}
		my_free(name8);
		return (entry != NULL);
	}
	return 0;
}



static inline int fbAddFile (TFB *fb, const int nodeId, const wchar_t *path, wchar_t *name, const WIN32_FIND_DATAW *ffd, const int id)
{
#if 0
	wchar_t buffer[MAX_PATH+1];
	
	int flags = SHGFI_TYPENAME | SHGFI_LINKOVERLAY | SHGFI_SHELLICONSIZE | SHGFI_DISPLAYNAME | SHGFI_EXETYPE | SHGFI_LARGEICON | SHGFI_ICON | SHGFI_SYSICONINDEX;
	SHFILEINFOW psfi;
	memset(&psfi, 0, sizeof(SHFILEINFOW));
	
	__mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls", path, name);
	
	HIMAGELIST sysimgl = (HIMAGELIST)SHGetFileInfoW(buffer, 0, &psfi, sizeof(SHFILEINFOW), flags);
	HICON icon = ImageList_GetIcon(sysimgl, psfi.iIcon, ILD_IMAGE);
	__mingw_wprintf(L"name ct:%i, %i %p %p '%ls' '%ls' '%ls'\n", ImageList_GetImageCount(sysimgl), sysimgl, icon, psfi.hIcon, name, psfi.szDisplayName, psfi.szTypeName);
		
	IMAGEINFO pImageInfo;
	ImageList_GetImageInfo(sysimgl, psfi.iIcon, &pImageInfo);
	printf("%p %p %i %i %i %i\n", pImageInfo.hbmImage, pImageInfo.hbmMask, (int)pImageInfo.rcImage.left, (int)pImageInfo.rcImage.top, (int)pImageInfo.rcImage.right, (int)pImageInfo.rcImage.bottom);
	
	GpBitmap *bitmap;
	int ret = (int)GdipCreateBitmapFromHICON(icon, &bitmap);
	printf("%p %i\n", bitmap, ret);
	
	int w = (pImageInfo.rcImage.right - pImageInfo.rcImage.left);
	int h = (pImageInfo.rcImage.bottom - pImageInfo.rcImage.top);
	
	printf("w/h %i %i\n", w, h);
	
	/*TFRAME *frame = lNewFrame();
	ARGB rgb = 0;	
	for (int y = 0; y < h; y++)
	for (int x = 0; x < w; x++){
		GdipBitmapGetPixel(bitmap, x, y, &rgb);
		printf("%i,%i %X\n", x, y, (unsigned int)rgb);
	}*/
	
	
	
	ImageList_Destroy(sysimgl);
	DestroyIcon(icon);
#endif

	char *name8 = convertto8(name);
	if (name8){
		TTREEENTRY *entry = treeAddItem(fb->tree, nodeId, name8, id, TREE_TYPE_LEAF);
		if (entry){
			TFB_ITEM_DESC *desc = fbItemDescAlloc();
			if (desc){
				desc->fileAttributes = ffd->dwFileAttributes;
				desc->fileSize = *(uint64_t*)&ffd->nFileSizeLow;
				desc->creationDate = *(uint64_t*)&ffd->ftCreationTime;//.dwLowDateTime;
				desc->modifiedDate = *(uint64_t*)&ffd->ftLastWriteTime;//.dwLowDateTime;
				treeEntrySetStorage(entry, desc);
			}
		}
		my_free(name8);
		return (entry != NULL);
	}
	return 0;
}

int fbFindFiles (TFB *fb, const int nodeId, const wchar_t *path, const wchar_t *searchMask, const int *searchDepth, wchar_t **filesMasks)
{
	WIN32_FIND_DATAW ffd;
	memset(&ffd, 0, sizeof(WIN32_FIND_DATAW));


	int depth = (*searchDepth)-1;
	int count = 0;
	
	wchar_t buffer[MAX_PATH+1];
	__mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls", path, searchMask);
	
	//wprintf(L"FindFirstFile '%s'\n", buffer);
	
	HANDLE hFiles = FindFirstFileW(buffer, &ffd);
	if (hFiles != INVALID_HANDLE_VALUE){
		do{
			//wprintf(L"findfile %i: %s %p\n",++count, ffd.cFileName, filesMasks);
		
			if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_OFFLINE) || (ffd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)){

			}else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				if (wcscmp(ffd.cFileName, L".") && wcscmp(ffd.cFileName, L"..")){
					const int id = MAKEID;
					//if (wcscmp(ffd.cFileName, L"$RECYCLE.BIN")){
					if (ffd.cFileName[0] != L'$'){
						if (fbAddDirectory(fb, nodeId, ffd.cFileName, &ffd, id)){
							if (depth > 0){
								__mingw_snwprintf(buffer, MAX_PATH, L"%ls%ls\\", path, ffd.cFileName);
								count += fbFindFiles(fb, id, buffer, searchMask, &depth, filesMasks);
							}
							count++;
						}
					}
				}
			}else{
				if (!filesMasks || hasExt(ffd.cFileName, filesMasks)){
					if (fbAddFile(fb, nodeId, path, ffd.cFileName, &ffd, MAKEID))
						count++;
				}
			}
		}while(FindNextFileW(hFiles, &ffd));
		FindClose(hFiles);
	}
	return count;
}

//STDAPI SHGetDriveMedia (PCWSTR pszDrive, DWORD *pdwMediaContent);

TLOGICALDRIVE *fbGetLogicalDrives (int *tDrives)
{
	*tDrives = 0;
	
	char logicaldrives[MAX_PATH+1];
	const int total = GetLogicalDriveStringsA(MAX_PATH, logicaldrives);
	if (!total) return NULL;

	TLOGICALDRIVE *drives = my_calloc(total+1, sizeof(TLOGICALDRIVE));
	if (!drives) return NULL;

	const int len = 8192;
	wchar_t systemDrive[len];
	systemDrive[0] = 0;

	GetSystemWindowsDirectoryW(systemDrive, (UINT)MAX_PATH);

	int ct = 0;	
	for (int i = 0; i < total; i++){
		char *drive = strchr(logicaldrives+i, ':');
		if (drive){
			drive--;
			i++;
			
			int dtype;
			drives[ct].busType = getDriveBusType(drive[0]);
			
			//printf("%c: 0x%X %i\n", drive[0], drives[ct].busType, GetDriveTypeA(drive));
			
			if (drives[ct].busType == BusTypeUsb)
				dtype = DRIVE_USB;
			else
				dtype = GetDriveTypeA(drive);
					
			if (/*dtype != DRIVE_CDROM &&*/ dtype != DRIVE_NO_ROOT_DIR){
				if (drive[0] != 'A' && drive[0] != 'B'){	// don't add floppy drives
					drives[ct].drive[0] = drive[0];
					drives[ct].drive[1] = ':';
					drives[ct].drive[2] = '\\';
					drives[ct].drive[3] = 0;
					drives[ct].driveType = dtype;
					if (toupper(drives[ct].drive[0]) == toupper(systemDrive[0]))
						drives[ct].isSystemDrive = 1;
					else
						drives[ct].isSystemDrive = 0;
						
					//printf("%i %s\n", drives[ct].driveType, drives[ct].drive);
					//wchar_t pszDrive[3] = {drive[0], L':', 0};
					//DWORD pdwMediaContent = 0;
					//SHGetDriveMedia(pszDrive, &pdwMediaContent);
					//printf("# %c %X\n", drive[0], (int)pdwMediaContent);

					SHGetDiskFreeSpaceExA(drives[ct].drive, NULL, (ULARGE_INTEGER*)&drives[ct].totalNumberOfBytes, (ULARGE_INTEGER*)&drives[ct].totalNumberOfFreeBytes);
					//printf("'%s' %i %I64d %I64d %i\n", drives[ct].drive, dtype, drives[ct].totalNumberOfBytes, drives[ct].totalNumberOfFreeBytes, drives[ct].isSystemDrive);
					ct++;
				}
			}
		}
	}
	*tDrives = ct;
	return drives;
}

void fbGetLogicalDrivesRelease (TLOGICALDRIVE *drives)
{
	if (drives)
		my_free(drives);
}

wchar_t *fbGetSystem64Folder ()
{
	int ret = GetSystemWow64DirectoryW(NULL, 0);
	if (ret > 4){
		wchar_t *path = my_calloc(ret+8, sizeof(wchar_t));
		GetSystemWow64DirectoryW(path, ret);
		wcscat(path, L"\\");
		return path;
	}
	
	return NULL;
}

wchar_t *fbGetSystem32Folder ()
{
	int ret = GetSystemDirectoryW(NULL, 0);
	if (ret > 4){
		wchar_t *path = my_calloc(ret+8, sizeof(wchar_t));
		GetSystemDirectoryW(path, ret);
		wcscat(path, L"\\");
		return path;
	}
	
	return NULL;
}

wchar_t *fbGetWindowsFolder ()
{
	int ret = GetWindowsDirectoryW(NULL, 0);
	if (ret > 4){
		wchar_t *path = my_calloc(ret+8, sizeof(wchar_t));
		GetSystemDirectoryW(path, ret);
		wcscat(path, L"\\");
		return path;
	}
	
	return NULL;
}

TSHELLFOLDEROBJS *fbGetShellFolders (int *total)
{
	*total = 0;
	wchar_t location[MAX_PATH+1];
	wchar_t linkName[MAX_PATH+1];


	TSHELLFOLDEROBJS sfobjs[] = {
		{CSIDL_PERSONAL, getMyDocumentsName(linkName, MAX_PATH), NULL},
		{CSIDL_DESKTOPDIRECTORY, getDesktopName(linkName, MAX_PATH), NULL},
		{CSIDL_MYPICTURES, L"My Pictures", NULL},
		{CSIDL_MYMUSIC, L"My Music", NULL},
		{CSIDL_MYVIDEO, L"Videos", NULL},
		{CSIDL_NETHOOD, L"Nethood", NULL},
		{CSIDL_APPDATA, L"App data", NULL},				// roaming
		//{CSIDL_LOCAL_APPDATA, L"App data", NULL},		// local
		//{CSIDL_COMMON_APPDATA, L"App data c", NULL},	// common
		//{CSIDL_COMPUTERSNEARME, L"Computers", NULL},
		{0, L"", NULL}
	};
	
	
	*total = sizeof(sfobjs) / sizeof(TSHELLFOLDEROBJS);
	TSHELLFOLDEROBJS *folders = my_calloc(*total, sizeof(TSHELLFOLDEROBJS));
	
	*total = 0;
	for (int i = 0; sfobjs[i].csidl; i++){
		if (shGetFolderPath(sfobjs[i].csidl, location)){
			//wprintf(L"#%s#, #%s#\n", location, sfobjs[i].name);
			
			folders[i].csidl = sfobjs[i].csidl;
			folders[i].name = my_wcsdup(sfobjs[i].name);
			folders[i].location = my_wcsdup(location);
			
			(*total)++;
		}
	}

	return folders;
}

void fbGetShellFoldersRelease (TSHELLFOLDEROBJS *folders)
{
	if (!folders) return;
	
	for (int i = 0; folders[i].csidl; i++){
		my_free(folders[i].name);
		my_free(folders[i].location);
	}
	my_free(folders);
}

static inline void fbShortcutFree (TSTRSHORTCUT *sc)
{
	if (sc->path)
		my_free(sc->path);
	if (sc->link)
		my_free(sc->link);
	my_free(sc);
}

void fbShortcutsFree (TLINKSHORTCUTS *linksc)
{
	if (linksc->total){
		while(linksc->total--)
			fbShortcutFree(linksc->links[linksc->total]);
		my_free(linksc->links);
	}
	linksc->total = 0;
}

int fbShortcutsGetTotal (TLINKSHORTCUTS *linksc)
{
	return linksc->total;
}

TSTRSHORTCUT *fbShortcutsGet (TLINKSHORTCUTS *linksc, const int Idx)
{
	return linksc->links[Idx];
}

#if 0
int fbShortcutsAddW (TLINKSHORTCUTS *linksc, const wchar_t *path, const wchar_t *link)
{
	if (!linksc->total){
		linksc->links = my_malloc(sizeof(TSTRSHORTCUT**));
		if (!linksc->links) return 0;
	}

	linksc->links = (TSTRSHORTCUT**)my_realloc(linksc->links, (linksc->total+1) * sizeof(TSTRSHORTCUT*));
	if (!linksc->links){
		linksc->total = 0;
		return 0;
	}

	linksc->links[linksc->total] = my_malloc(sizeof(TSTRSHORTCUT));
	if (!linksc->links[linksc->total]){
		linksc->total = 0;
		return 0;
	}

	linksc->links[linksc->total]->path = my_wcsdup(path);
	linksc->links[linksc->total]->link = my_wcsdup(link);
	return ++linksc->total;
}
#endif

int fbShortcutsAdd (TLINKSHORTCUTS *linksc, const char *path, const char *link, const int type)
{
	if (!linksc->total){
		linksc->links = my_malloc(sizeof(TSTRSHORTCUT**));
		if (!linksc->links) return 0;
	}

	linksc->links = my_realloc(linksc->links, (linksc->total+1) * sizeof(TSTRSHORTCUT**));
	if (!linksc->links){
		linksc->total = 0;
		return 0;
	}

	linksc->links[linksc->total] = my_malloc(sizeof(TSTRSHORTCUT));
	if (!linksc->links[linksc->total]){
		linksc->total = 0;
		return 0;
	}

	linksc->links[linksc->total]->type = type;
	linksc->links[linksc->total]->path = converttow(path);
	linksc->links[linksc->total]->link = converttow(link);
	return ++linksc->total;
}

int fbShortcutsAdd8 (TLINKSHORTCUTS *linksc, const char *path, const char *link)
{
	//printf("fbShortcutsAdd8\n");
	return fbShortcutsAdd(linksc, path, link, SYMLINK_SHORTCUT);
}

int fbShortcutAddModule (TLINKSHORTCUTS *linksc, const char *module, const char *name)
{
	//printf("fbShortcutAddModule\n");
	return fbShortcutsAdd(linksc, module, name, SYMLINK_MODULE);
}

char *fbGetVolumeLabel (const int drive)
{
	size_t blen = MAX_PATH;
	wchar_t buffer[blen+1];
	 
	wchar_t driveW[] = {drive&0xFF, ':', '\\', 0};
	if (GetVolumeInformationW(driveW, buffer, blen, 0, 0, 0, 0, 0))
		return convertto8(buffer);
	else
		return NULL;
}

#if 0
char *fbGetComputerName ()
{
	size_t blen = MAX_PATH;
	wchar_t buffer[blen+1];
/*
ComputerNameNetBIOS
ComputerNameDnsHostname
ComputerNameDnsDomain
ComputerNameDnsFullyQualified
ComputerNamePhysicalNetBIOS
ComputerNamePhysicalDnsHostname
ComputerNamePhysicalDnsDomain
ComputerNamePhysicalDnsFullyQualified
ComputerNameMax
*/
	if (GetComputerNameExW(ComputerNameDnsHostname, buffer, (DWORD*)&blen)){
		__mingw_wprintf(L"GetComputerName #%ls#\n", buffer);
		return convertto8(buffer);
	}else
		return convertto8(MYCOMPUTERNAME);
}
#else
char *fbGetComputerName ()
{
	size_t blen = MAX_PATH;
	wchar_t buffer[blen+1];
		
	if (GetComputerNameW(buffer, (DWORD*)&blen))
		return convertto8(buffer);
	else
		return convertto8(MYCOMPUTERNAME);
}
#endif

char *fbGetMyComputerName ()
{
	HKEY hKey = 0;
	DWORD type = REG_SZ;
	wchar_t *ret = MYCOMPUTERNAME;
	size_t blen = MAX_PATH;
	wchar_t buffer[blen+1];
	
	if ((ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, MY_COMPUTER_LOCALE, 0, KEY_READ, &hKey))){
		if ((ERROR_SUCCESS == RegQueryValueExW(hKey, L"", 0, &type, (LPBYTE)buffer, (PDWORD)&blen)))
			ret = buffer;
		RegCloseKey(hKey);
	}

	return convertto8(ret);
}


TTREEENTRY *fbGetEntry (TFB *fb, TFBITEM *item)
{
	return treeListGetSubEntry(item);
}

static inline int fbGetEntryId (TFB *fb, TTREEENTRY *entry)
{
	if (entry) return entry->id;
	
	return 0;
}

static inline char *fbGetEntryName (TFB *fb, TTREEENTRY *entry)
{
	if (entry) return entry->name;
	
	return NULL;
}

void fbSetUserData (TFB *fb, void *data)
{
	fb->udata = data;
}

void *fbGetUserData (TFB *fb)
{
	return fb->udata;
}

TFBITEM *fbGetFirst (TFB *fb)
{
	TTREEENTRY *entry = treeEntryFind(fb->tree->root, fb->rootId);
	if (entry) return entry->head;
	return NULL;
}

TFBITEM *fbGetNext (TFB *fb, TFBITEM *item)
{
	return item->next;
}

int fbIsLeaf (TFB *fb, TFBITEM *item)
{
	return treeEntryIsLeaf(fbGetEntry(fb, item));
}

int fbIsBranch (TFB *fb, TFBITEM *item)
{
	return treeEntryIsBranch(fbGetEntry(fb, item));
}

char *fbGetName (TFB *fb, TFBITEM *item)
{
	return fbGetEntryName(fb, fbGetEntry(fb, item));
}

static inline int fbGetId (TFB *fb, TFBITEM *item)
{
	return fbGetEntryId(fb, fbGetEntry(fb, item));
}

uint64_t fbGetFilesize (TFB *fb, TFBITEM *item)
{
	TFB_ITEM_DESC *desc = treeEntryGetStorage(fbGetEntry(fb, item));
	return desc->fileSize;
}


void fbFormatSize (char *buffer, const uint64_t filesize)
{
	const int len = 32;
	if (filesize >= (uint64_t)100*1024*1024*1024)
		__mingw_snprintf(buffer, len, "%iG", (int)(filesize/1024/1024/1024));
	else if (filesize >= (uint64_t)1024*1024*1024)
		__mingw_snprintf(buffer, len, "%.1fG", filesize/1024.0/1024.0/1024.0);
	else if (filesize >= 100*1024*1024)
		__mingw_snprintf(buffer, len, "%iM", (int)(filesize/1024/1024));
	else if (filesize >= 1024*1024)
		__mingw_snprintf(buffer, len, "%.1fM", filesize/1024.0/1024.0);
	else if (filesize >= 10*1024)
		__mingw_snprintf(buffer, len, "%iK", (int)(filesize/1024));
	else if (filesize >= 1024)
		__mingw_snprintf(buffer, len, "%.1fK", filesize/1024.0);
	else
		__mingw_snprintf(buffer, len, "%ib", (int)(filesize));
}

void fbOpenExplorerLocationW (wchar_t *path)
{
	LPITEMIDLIST item;
	SFGAOF attributes;

	if (SUCCEEDED(SHParseDisplayName(path, NULL, (void*)&item, 0, &attributes))){
		SHOpenFolderAndSelectItems(item, 0, NULL, 0);
		CoTaskMemFree(item);
	}
}

void fbOpenExplorerLocationA (char *path)
{
	wchar_t *pathW = converttow(path);
	if (pathW){
		fbOpenExplorerLocationW(pathW);
		my_free(pathW);
	}
}
