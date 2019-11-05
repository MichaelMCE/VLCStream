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


#ifndef _ARTWORKQUEUE_H_
#define _ARTWORKQUEUE_H_




#include <vlc/vlc.h>




#define ARTWORK_THREAD_CREATING			(0)			// shouldn't ever encounter this
#define ARTWORK_THREAD_RUNNING			(1)
#define ARTWORK_THREAD_SUSPENDED		(2)
#define ARTWORK_THREAD_EXIT				(3)
#define ARTWORK_THREAD_EXITING			(4)
#define ARTWORK_THREAD_EXITED			(5)




typedef struct {
	libvlc_media_t *m;
	libvlc_event_manager_t *em;
	
	TMETATAGCACHE *tagc;
	
	int pageSource;					// not required
	unsigned int hash;				// media path hash
	char *path;						// utf8 path
	unsigned int playlistUID;		// playlist id
	int trackPosition;				// position in above playlist
	TMETACOMPLETIONCB *mccb;		// sender notification on completion
}job_detail;


// job description
typedef struct {
	int jobId;		// job identifier
	uint64_t time1;
	int retryCount;
	job_detail detail;
}art_job;


// jobs' container (art_job held within .storage of linked list)
typedef struct {
	TLISTITEM *root;
	TLISTITEM *last;
	
	int jobIdSrc;
	int maxRetries;
}art_queue;

typedef struct {
	art_queue *queue;
	
	HANDLE jobTriggerEvent;
	TMLOCK *listLock;			// queueLock
}art_work;

typedef struct job_thread job_thread;
struct job_thread {
	art_work *artwork;
	TMLOCK *threadLock;
	
	uintptr_t hThread;
	unsigned int threadId;
	HANDLE jobTriggerEvent;
	int waitPeriod;
		
	unsigned (__stdcall *func) (void *);
	void *dataPtr;
	int refCount;		// 0:invalid, 1:original, 2:1'st clone (2 threads), 3:2'nd clone (3 threads, etc..
	int isClone;		// is this thread a clone of another
	int threadState;
	job_thread *parent;	// if this is a clone then points to parent/originator
			
	TVLCPLAYER *vp;
};




job_thread *artThreadNew (TVLCPLAYER *vp, const int maxRetries, unsigned (__stdcall *func)(void *), void *dataPtr, const int waitPeriod);
job_thread *artThreadClone (job_thread *jt);
void artThreadRelease (job_thread *jt);
unsigned int __stdcall artThreadWorker (void *ptr);


art_job *jobDetailAttach (art_work *aw, job_detail *jd);


job_detail *artDetailFind (art_work *aw, const unsigned int hash);
art_job *artJobFind (art_work *aw, const int juid);

void artJobTimeSet (art_job *aj, const uint64_t time1);
art_work *artThreadGetWork (job_thread *jt);
void artQueueFlush (art_work *aw);
void artWorkSignalReady (art_work *aw);

int artThreadLockWait (job_thread *jt);
void artThreadLockRelease (job_thread *jt);

#endif

