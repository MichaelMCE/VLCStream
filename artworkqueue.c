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


extern volatile int SHUTDOWN;


int artThreadLockWait (job_thread *jt)
{
	if (jt && jt->threadLock)
		return lockWait(jt->threadLock, INFINITE);
	else
		return 0;
}

void artThreadLockRelease (job_thread *jt)
{
	lockRelease(jt->threadLock);
}

int artQueueLockWait (art_work *aw)
{
	if (aw && aw->listLock)
		return lockWait(aw->listLock, INFINITE);
	else
		return 0;
}

void artQueueLockRelease (art_work *aw)
{
	lockRelease(aw->listLock);
}

static inline art_job *artEntryGetJob (TLISTITEM *entry)
{
	return listGetStorage(entry);
}

static inline art_queue *artQueueAlloc (const int maxRetries)
{
	art_queue *aq = my_calloc(1, sizeof(art_queue));
	if (aq)
		aq->maxRetries = maxRetries;
	return aq;
}

static inline art_work *artWorkCreate (const int maxRetries, HANDLE hEventTrigger)
{
	art_work *aw = my_calloc(1, sizeof(art_work));
	if (aw){
		aw->jobTriggerEvent = hEventTrigger;
		aw->listLock = lockCreate("artWorkCreate");
		aw->queue = artQueueAlloc(maxRetries);
	}
	return aw;
}

art_work *artThreadGetWork (job_thread *jt)
{
	art_work *aw = NULL;
	if (artThreadLockWait(jt)){
		aw = jt->artwork;
		artThreadLockRelease(jt);
	}
	return aw;
}

static inline void artThreadDestroy (job_thread *jt)
{
	//printf("# artThreadDestroy\n");
	
	my_free(jt);
}

static inline job_thread *artThreadAlloc ()
{
	job_thread *jt = my_calloc(1, sizeof(job_thread));
	if (jt) jt->threadState = ARTWORK_THREAD_CREATING;
	return jt;
}

void artWorkSignalReady (art_work *aw)
{
	SetEvent(aw->jobTriggerEvent);
}

void artThreadExit (job_thread *jt)
{
	//SuspendThread((HANDLE)at->hThread);
	
	
	jt->threadState = ARTWORK_THREAD_EXIT;
	for (int i = 0; i < jt->refCount; i++){
		artWorkSignalReady(jt->artwork);
		lSleep(10);
	}

	WaitForSingleObject((HANDLE)jt->hThread, INFINITE);
	CloseHandle((HANDLE)jt->hThread);
	jt->hThread = 0;
		
	if (jt->isClone){
		jt->parent->refCount--;
	}else{
		jt->refCount--;
		CloseHandle(jt->jobTriggerEvent);
	}
}

static inline void artJobDestroy (art_job *aj)
{
	//printf("artJobDestroy %p\n", aj);
	// free job storage, handle and other stuff
	// free event manager
	// free mccb
	
	job_detail *jd = &aj->detail;

	if (jd->m){
		if (jd->em){
			libvlc_event_detach(jd->em, libvlc_MediaParsedChanged, artwork_EventCB, (void*)aj->jobId);
			jd->em = NULL;
		}
		libvlc_media_release(jd->m);
		jd->m = NULL;
	}
	if (jd->path) my_free(jd->path);
	if (jd->mccb) my_free(jd->mccb);
	my_free(aj);
}

static inline void artQueueDestroy (art_queue *aq)
{
	//printf("# artQueueDestroy\n");
	
	if (aq->root)
		listDestroyAll(aq->root);
	my_free(aq);
}

static inline void artQueueRelease (art_queue *aq)
{
	TLISTITEM *entry = aq->root;
	while(entry){
		art_job *aj = artEntryGetJob(entry);
		if (aj) artJobDestroy(aj);
		entry = entry->next;
	}

	artQueueDestroy(aq);
}

static inline void artWorkDestroy (art_work *aw)
{
	//printf("# artWorkDestroy\n");
	
	my_free(aw);
}

static inline void artWorkRelease (art_work *aw)
{
	if (artQueueLockWait(aw)){
		artQueueRelease(aw->queue);
	
		TMLOCK *lock = aw->listLock;
		aw->listLock = NULL;
		lockClose(lock);
	
		artWorkDestroy(aw);
	}
}

void artThreadRelease (job_thread *jt)
{
	if (artThreadLockWait(jt)){
		artThreadExit(jt);
		artWorkRelease(jt->artwork);
		
		TMLOCK *lock = jt->threadLock;
		jt->threadLock = NULL;
		lockClose(lock);
		artThreadDestroy(jt);
	}
}

job_thread *artThreadNew (TVLCPLAYER *vp, const int maxRetries, unsigned (__stdcall *func)(void *), void *dataPtr, const int waitPeriod)
{
	job_thread *jt = artThreadAlloc();
	if (jt){
		jt->vp = vp;
		jt->func = func;
		jt->dataPtr = jt;//dataPtr;
		jt->threadLock = lockCreate("artThreadNew");
		jt->jobTriggerEvent = CreateEvent(NULL, 0, 0, NULL);
		jt->artwork = artWorkCreate(maxRetries, jt->jobTriggerEvent);
		jt->refCount = 1;
		jt->waitPeriod = waitPeriod;
		jt->parent = jt;
		jt->isClone = 0;
		jt->hThread = _beginthreadex(NULL, 0, jt->func, jt, CREATE_SUSPENDED, &jt->threadId);
		if (jt->hThread){
			jt->threadState = ARTWORK_THREAD_RUNNING;
			ResumeThread((HANDLE)jt->hThread);		// check return and/or GLE
			return jt;
		}
		artThreadDestroy(jt);
	}
	return NULL;
}

job_thread *artThreadClone (job_thread *jt)
{
	job_thread *jtClone = artThreadAlloc();
	if (jtClone){
		if (artThreadLockWait(jt)){
			my_memcpy(jtClone, jt, sizeof(job_thread));
			artThreadLockRelease(jt);
		}
		
		jtClone->hThread = _beginthreadex(NULL, 0, jtClone->func, jtClone, CREATE_SUSPENDED, &jtClone->threadId);
		if (jtClone->hThread){
			jtClone->isClone = 1;
			jtClone->parent = jt;
			jtClone->waitPeriod += (jtClone->parent->refCount*333);
			jtClone->parent->refCount++;
			jtClone->threadState = ARTWORK_THREAD_RUNNING;
			ResumeThread((HANDLE)jtClone->hThread);		// check return and/or GLE
			return jtClone;
		}
		artThreadDestroy(jtClone);
	}
	return NULL;
}

static inline uint64_t artJobTimeGet (art_job *aj)
{
	return aj->time1;
}

void artJobTimeSet (art_job *aj, const uint64_t time1)
{
	aj->time1 = time1;
}

static inline art_job *artJobAlloc (art_queue *aq, const job_detail *detail)
{
	 art_job *aj = my_calloc(1, sizeof(art_job));
	 if (aj){
	 	aj->jobId = ++aq->jobIdSrc;
	 	aj->retryCount = 0;
	 	my_memcpy(&aj->detail, detail, sizeof(job_detail));
	 }
	 return aj;
}

static inline void artQueueAddEntry (art_queue *aq, TLISTITEM *entry)
{
	listInsert(entry, aq->last, NULL);
	aq->last = entry;
	if (!aq->root) aq->root = aq->last;
}

static inline TLISTITEM *artQueueAdd (art_queue *aq, art_job *aj)
{
	// create a list item then add this job to queue
	TLISTITEM *entry = listNewItem(aj);
	if (entry)
		artQueueAddEntry(aq, entry);

	return entry;
}

art_job *jobDetailAttach (art_work *aw, job_detail *jd)
{
	art_job *aj = NULL;
	
	if (artQueueLockWait(aw)){
		aj = artJobAlloc(aw->queue, jd);
		if (aj){
			artJobTimeSet(aj, getTickCount());
			artQueueAdd(aw->queue, aj);
		}
		artQueueLockRelease(aw);
	}
	return aj;
}

static inline TLISTITEM *artQueueGetOldestEntry (art_queue *aq)
{
	// remove first job from list then return with it
	if (aq->root){
		TLISTITEM *entry = aq->root;
		TLISTITEM *next = listRemove(entry);
		if (!next) aq->last = NULL;
		aq->root = next;
		return entry;
	}
	return aq->root;
}

art_job *artJobFind (art_work *aw, const int juid)
{
	art_job *aj = NULL;

	if (artQueueLockWait(aw)){
		TLISTITEM *entry = aw->queue->root;
		
		while(entry){
			aj = artEntryGetJob(entry);
			if (aj->jobId == juid) break;
			entry = entry->next;
		}
		artQueueLockRelease(aw);
	}
	return aj;
}

void artQueueFlush (art_work *aw)
{
	if (artQueueLockWait(aw)){
		TLISTITEM *entry = aw->queue->root;
		while(entry){
			art_job *aj = artEntryGetJob(entry);
			if (aj) artJobDestroy(aj);
			entry = entry->next;
		}
	
		if (aw->queue->root){
			listDestroyAll(aw->queue->root);
			aw->queue->root = NULL;
			aw->queue->last = NULL;
		}
		artQueueLockRelease(aw);
	}
}

job_detail *artDetailFind (art_work *aw, const unsigned int hash)
{
	job_detail *jd = NULL;
	
	if (artQueueLockWait(aw)){
		TLISTITEM *entry = aw->queue->root;
		while(entry){
			art_job *aj = artEntryGetJob(entry);
			if (aj->detail.hash == hash){
				jd = &aj->detail;
				break;
			}
			entry = entry->next;
		}
		artQueueLockRelease(aw);
	}
	return jd;
}

static inline TTAGIMG *tagGetSharedArtDesc (TMETATAGCACHE *tagc, const unsigned int hash, const char *mrl, int *status)
{
	TTAGIMG *art = NULL;
	*status = 0;
	
	if (tagLock(tagc)){
   		if (!(art=g_tagArtworkGetByHash(tagc, hash))){
   			if ((art=g_tagArtworkAlloc(tagc, getHash(mrl))))
	   			g_tagArtworkAddByHash(tagc, hash, art);
	   	}

	   	if (art){
	   		*status = art->enabled;
	   		if (art->isBeingProcessed)
	   			art = NULL;
	   		else
	   			art->isBeingProcessed = 1;
	   	}
	   	
	   	tagUnlock(tagc);
	}
	return art;
}

int artThreadProcessJob (job_thread *jt, art_work *aw, art_job *aj)
{
	job_detail *jd = &aj->detail;
	//printf("artThreadProcessJob: %i %p %p %p, %p\n", aj->jobId, jt, aw, aj, jd->m);

	char mrl[MAX_PATH_UTF8+1];
	
	tagRetrieveByHash(jd->tagc, jd->hash, MTAG_ArtworkURL, mrl, MAX_PATH_UTF8);
	if (!*mrl && jd->m){
		char *tag = libvlc_media_get_meta(jd->m, MTAG_ArtworkURL);
		if (tag){
			if (!strncmp(tag, "file:", 5)){
				strncpy(mrl, tag, MAX_PATH_UTF8);
				tagAddByHash(jd->tagc, NULL, jd->hash, MTAG_ArtworkURL, mrl, 1);
			}
			libvlc_free(tag);
		}
	}
	if (!*mrl) return 0;

	int complete = 0;
	int status = 0;

	TTAGIMG *art = tagGetSharedArtDesc(jd->tagc, jd->hash, mrl, &status);
	if (art){
		if (!status){
			int len = strlen(mrl);
			char tmp[len+1];
			memset(tmp, 0, sizeof(tmp));

			if ((len=decodeURIEx(mrl, len, tmp))){
				wchar_t *path = converttow(tmp);
				if (path){
					status = complete = (artworkLoad(jt->vp, art, path)/* > 0*/);
					my_free(path);
				}
			}
		}else{
			complete = 1;
		}
		if (status){
			if (g_artworkAccessCacheSearch(jd->tagc, art) == -1)
				g_artworkAccessCacheAdd(jd->tagc, art);
		}
		art->isBeingProcessed = 0;
	}

	if (complete){
		PLAYLISTCACHE *plc = playlistManagerGetPlaylistByUID(jt->vp->plm, jd->playlistUID);
		if (plc && jd->hash){
			if (!plc->artHash) plc->artHash = jd->hash;
			if (plc->parent /*&& !plc->parent->artHash*/){
				plc->parent->artHash = jd->hash;
				if (plc->parent->parent /*&& !plc->parent->artHash*/)
					plc->parent->parent->artHash = jd->hash;
			}
			//printf("%i, %i %x #%s#, %x #%s#\n", art->enabled, jd->playlistUID, jd->hash, plc->title, jd->hash, plc->parent->title);
		}
	}
	
	if (complete || status){
		if (jd->mccb && jd->mccb->cb){
			jd->mccb->cb(jt->vp, 0, jd->mccb->dataInt1, jd->mccb->dataInt2, jd->mccb->dataPtr1, jd->mccb->dataPtr2);
			jd->mccb->cb = NULL;	// has been fired, don't refire
		}
	}

	return complete;
}

void artThreadProcessQueue (job_thread *jt, art_work *aw)
{

	//const int tid = GetCurrentThreadId();
	int firstUnsuccess = 0;

	while(jt->threadState == ARTWORK_THREAD_RUNNING){
		if (SHUTDOWN) break;

		TLISTITEM *entry = NULL;
		if (artQueueLockWait(aw)){
			entry = artQueueGetOldestEntry(aw->queue);
			artQueueLockRelease(aw);
		}
		if (!entry) break;

		art_job *aj = artEntryGetJob(entry);
		if (!aj) continue;
		
		int resubmit = 0;
		int complete = 0;
		int destroy = 0;

		int64_t dt = aj->time1 - getTickCount();
		//printf("artThreadProcessQueueDt %i %p %i\n", aj->jobId, aj, dt);

		if (dt <= 0)
			complete = artThreadProcessJob(jt, aw, aj);
		else
			resubmit = 1;

		//if (aj->jobId == 1)
		//printf("%i: %i, '%s', %i %i :%i\n", aj->jobId, dt, aj->detail.path, complete, resubmit, tid);

		if (!complete){
			if (dt <= 0 && aj->retryCount <= aw->queue->maxRetries){
				aj->retryCount++;
				artJobTimeSet(aj, getTickCount() + 1500);
				//firstUnsuccess = 0;
				resubmit = 1;
			}else{
				destroy = 1;
			}
		}else{
			destroy = 1;
		}

		if (destroy && !resubmit){
			if (artQueueLockWait(aw)){
				if (firstUnsuccess == aj->jobId)
					firstUnsuccess = 0;
				artJobDestroy(aj);
				listDestroy(entry);
				artQueueLockRelease(aw);
			}
		}

		if (resubmit){
			const int jid = aj->jobId;
				
			if (artQueueLockWait(aw)){
				artQueueAddEntry(aw->queue, entry);
				artQueueLockRelease(aw);
			}
			if (!firstUnsuccess){
				firstUnsuccess = jid;
			}else if (firstUnsuccess == jid){
				firstUnsuccess = 0;
				break;
			}
			Sleep(10);
		}
	}
}

unsigned int __stdcall artThreadWorker (void *ptr)
{
	job_thread *jt = (job_thread*)ptr;
	
	//const int tid = GetCurrentThreadId();
	const int waitPeriod = jt->waitPeriod;
	const int isClone = jt->isClone;

	//printf("artThreadWorker in %p, %i, %i\n", jt, tid, waitPeriod);
	if (isClone) jt = jt->parent;
	
	while(jt->threadState == ARTWORK_THREAD_RUNNING){
		int ret = WaitForSingleObject(jt->jobTriggerEvent, waitPeriod);
		if (SHUTDOWN || jt->threadState == ARTWORK_THREAD_EXIT){
			jt->threadState = ARTWORK_THREAD_EXITING;
			break;
		}
		
		if (!getIdle(jt->vp) || !isClone){
			if (ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT)
				artThreadProcessQueue(jt, jt->artwork);
			else
				break;
		}
	}
	
	//printf("artThreadWorker out %p, %i\n", jt, tid);
	
	jt->threadState = ARTWORK_THREAD_EXITED;
	if (isClone) artThreadDestroy(ptr);

	_endthreadex(1);
	return 1;
}
