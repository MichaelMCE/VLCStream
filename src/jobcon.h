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


#ifndef _JOBCON_H_
#define _JOBCON_H_

#if 0


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

	int pageSource;					// not required
	unsigned int hash;				// media path hash
	char *path;						// utf8 path
	unsigned int playlistUID;		// playlist id
	int trackPosition;				// position in above playlist
	//TMETACOMPLETIONCB *mccb;		// sender notification on completion
}job_art_detail;


// job description
typedef struct {
	int jobId;				// job identifier
	uint64_t time1;
	int retryCount;
	job_art_detail detail;
}job_desc;

// jobs' container (job_desc held within .storage of linked list item)
typedef struct {
	TLISTITEM *root;
	TLISTITEM *last;
	
	//int jobIdSrc;
	int retryMax;			// upon failure a job is retried retryMax times before being considered a total waste of time
}job_queue;

typedef struct {
	job_queue *queue;
	TMLOCK *lock;			// queueLock
	int jobCount;
}job_worker;

typedef struct {
	job_worker *work;
	TMLOCK *lock;

	HANDLE jobEventTrigger;
	int waitPeriod;
	
	uintptr_t hThread;
	unsigned int threadId;
	int threadState;

	TVLCPLAYER *vp;
}job_Thread;

// thread controller
typedef struct {
	job_Thread **threads;	// list of worker threads, end of list == NULL
	int tThreads;
	unsigned int (__stdcall *workerFunc) (void *);
	TMLOCK *lock;
	
	int jobIdSrc;
	int waitPeriod;
	TVLCPLAYER *vp;
}job_controller;



__stdcall unsigned int jobThreadWorkerFunc (void *ptr);


job_controller *jobControllerNew (TVLCPLAYER *vp, unsigned int (__stdcall *workerFunc) (void *));
void jobControllerDelete (job_controller *jc);



int jobThreadAdd (job_controller *jc);
int jobControllerAddJob (job_controller *jc, job_art_detail *jobDetail);


#endif

#endif
