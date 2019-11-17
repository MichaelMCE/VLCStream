
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



#if 0


#include "common.h"

extern volatile int SHUTDOWN;




static inline int jwLock (job_worker *jw)
{
	return lockWait(jw->lock, INFINITE);
}

static inline void jwUnlock (job_worker *jw)
{
	lockRelease(jw->lock);
}

static inline int jtLock (job_Thread *jt)
{
	return lockWait(jt->lock, INFINITE);
}

static inline void jtUnlock (job_Thread *jt)
{
	lockRelease(jt->lock);
}

static inline int jcLock (job_controller *jc)
{
	return lockWait(jc->lock, INFINITE);
}

static inline void jcUnlock (job_controller *jc)
{
	lockRelease(jc->lock);
}

static inline void jobThreadQueueSignalReady (job_Thread *jt)
{
	SetEvent(jt->jobEventTrigger);
}

static inline void jobThreadSetState (job_Thread *jt, const int state)
{
	jt->threadState = state;
}

static inline int jobThreadGetState (job_Thread *jt)
{
	return jt->threadState;
}

int jobControllerAddJob (job_controller *jc, job_art_detail *jobDetail)
{
	job_Thread *jt = NULL;
	int jobId = 0;
	
	// find a thread
	if (jcLock(jc)){
		jobId = ++jc->jobIdSrc;
		int jct = 999999;
		
		// find then use thread with the least amount of jobs
		for (int i = 0; jc->threads[i]; i++){
			if (jtLock(jc->threads[i])){
				if (jc->threads[i]->work->jobCount < jct){
					jt = jc->threads[i];
					jct = jt->work->jobCount;
				}
				jtUnlock(jc->threads[i]);
			}
		}
		jcUnlock(jc);
	}

	int ret = 0;
	
	if (jt){
		// we've got a thread, lock and insert job
		if (jtLock(jt)){
			if (jobThreadGetState(jt) == ARTWORK_THREAD_RUNNING){
				job_worker *jw = jt->work;
				
				if (jwLock(jw)){
					job_desc *jd = my_calloc(1, sizeof(job_desc));
					if (jd){
						jw->jobCount++;
						ret = jd->jobId = jobId;
						jd->time1 = getTickCount() + 3500;
						jd->retryCount = 0;
						my_memcpy(&jd->detail, jobDetail, sizeof(*jobDetail));
					
						TLISTITEM *item = listNewItem(jd);
						listAdd(jw->queue->root, item);

						jobThreadQueueSignalReady(jt);
					}
					jwUnlock(jw);
				}
			}
			jtUnlock(jt);
		}
	}else{
		printf("jobControllerAddJob. no threads available (add one)\n");
	}

	return ret;
}

static inline job_desc *listGetJobDecription (TLISTITEM *item)
{
	return listGetStorage(item);
}

static inline job_desc *jobQueueGetNextJob (job_queue *jq, const int jobId)
{

	if (!jobId){
		if (jq->root)
			return listGetJobDecription(jq->root);
	}else{
		TLISTITEM *item = jq->root;
		while (item){
			job_desc *jd = listGetJobDecription(item);
			if (jd->jobId == jobId){
				if (item->next)
					return listGetJobDecription(item->next);
				else
					break;
			}
			item = listGetNext(item);
		}
	}
	return NULL;
}

static inline job_desc *jobQueueRemoveJob (job_queue *jq, const int jobId)
{

	if (jobId){
		TLISTITEM *item = jq->root;
		while(item){
			TLISTITEM *next = listGetNext(item);
			
			job_desc *jd = listGetJobDecription(item);
			if (jd->jobId == jobId){
				listRemove(item);
				listDestroy(item);
				if (jq->root == item)
					jq->root = next;
				return jd;
			}
			item = next;
		}
	}
	return NULL;
}

__stdcall unsigned int jobThreadWorkerFunc (void *ptr)
{

	const int tid = GetCurrentThreadId();
	printf("workerFunc start %i\n", tid);
	
	job_Thread *jt = (job_Thread*)ptr;

	const int waitPeriod = jt->waitPeriod;
	
	while(jobThreadGetState(jt) == ARTWORK_THREAD_RUNNING){
		int ret = WaitForSingleObject(jt->jobEventTrigger, waitPeriod);
		if (SHUTDOWN || jobThreadGetState(jt) == ARTWORK_THREAD_EXIT){
			jobThreadSetState(jt, ARTWORK_THREAD_EXITING);
			break;
		}
		
		if (!getIdle(jt->vp)){
			if (ret == WAIT_OBJECT_0 || ret == WAIT_TIMEOUT){
				//int jobsProcessed = 0;
				
				if (jt->work->jobCount > 0){
					printf("workerFunc processing queue (tid:%i) ct:%i \n", tid, jt->work->jobCount);

					int jobId = 0;
					job_desc *jd = NULL;
					
					do{
						if (jwLock(jt->work)){
							jd = jobQueueGetNextJob(jt->work->queue, jobId);
							jwUnlock(jt->work);
						}
						
						if (jd){
							jobId = jd->jobId;
							
							uint64_t t0 = getTickCount();
							if ((int64_t)(t0 - jd->time1) >= 0){
								if (jd->retryCount++ < jt->work->queue->retryMax){
									printf("workerFunc got job (tid:%i) jobId:%i \n", tid, jd->jobId);
									
									// process job..
									
									jd->time1 = t0 + 3500;
									//jobsProcessed++;
								}else{
									if (jwLock(jt->work)){
										jobQueueRemoveJob(jt->work->queue, jobId);
										jt->work->jobCount--;
										jwUnlock(jt->work);
									}
									printf("workerFunc removed job (tid:%i) jobId:%i \n", tid, jd->jobId);
									jobId = 0;
								}
							}
						}
					}while(jd && !SHUTDOWN);
					
					//if (!jobsProcessed) break;
				}
			}else{
				break;
			}
		}
	}
	
	jobThreadSetState(jt, ARTWORK_THREAD_EXITED);
	
	printf("workerFunc end %i\n", tid);
	_endthreadex(1);
	return 1;
}

static inline job_Thread *jobThreadNew ()
{
	job_Thread *jt = my_calloc(1, sizeof(job_Thread));
	if (jt){
		jobThreadSetState(jt, ARTWORK_THREAD_CREATING);
		jt->lock = lockCreate(__func__);
		jt->jobEventTrigger = CreateEvent(NULL, 0, 0, NULL);
		
		jt->work = my_calloc(1, sizeof(job_worker));
		if (jt->work){
			jt->work->jobCount = 0;
			jt->work->lock = lockCreate(__func__);
			jt->work->queue = my_calloc(1, sizeof(job_queue));
			if (jt->work->queue){
				//jt->work->queue->jobIdSrc = 100;
				jt->work->queue->retryMax = 5;
			}
		}
	}
	return jt;
}

static inline int jobControllerAddThread (job_controller *jc, job_Thread *jt)
{
	for (int i = 0; i < 32; i++){
		if (!jc->threads[i]){
			jc->threads = (job_Thread**)my_realloc(jc->threads, (i+2)*sizeof(job_Thread**));
			if (jc->threads){
				jc->threads[i] = jt;
				jc->threads[i+1] = NULL;
				return i;
			}else{
				break;
			}
		}
	}
	return 0;
}

int jobThreadAdd (job_controller *jc)
{
	job_Thread *jt = jobThreadNew();
	if (jt){
		jt->hThread = _beginthreadex(NULL, 0, jc->workerFunc, jt, CREATE_SUSPENDED, &jt->threadId);
		if (jt->hThread){
			jc->tThreads++;
			jt->waitPeriod = jc->waitPeriod;
			jt->vp = jc->vp;
			jobThreadSetState(jt, ARTWORK_THREAD_SUSPENDED);
			jobControllerAddThread(jc, jt);

			jobThreadSetState(jt, ARTWORK_THREAD_RUNNING);
			ResumeThread((HANDLE)jt->hThread);		// check return and/or GLE
			return (int)jt->threadId;
		}
	}
	return 0;
}

void jobControllerDelete (job_controller *jc)
{
	jcLock(jc);
	
	//todo: suspend then free threads 
	for (int i = 0; jc->threads[i]; i++){
		job_Thread *jt = jc->threads[i];
		if (jt){
			jobThreadSetState(jt, ARTWORK_THREAD_EXIT);
			jobThreadQueueSignalReady(jt);
			WaitForSingleObject((HANDLE)jt->hThread, INFINITE);
			CloseHandle((HANDLE)jt->hThread);
			CloseHandle(jt->jobEventTrigger);
			
			job_worker *jw = jt->work;
			if (jw){
				jwLock(jw);
				job_queue *jq = jw->queue;
				if (jq){
					TLISTITEM *item = jq->root;
					while(item){
						void *ptr = listGetStorage(item);
						if (ptr){
							// free job_desc.
						}
						item = listGetNext(item);
					}
					listDestroyAll(jq->root);

					my_free(jq);
				}
				lockClose(jw->lock);
				my_free(jw);
			}
			my_free(jt);
		}
	}


	my_free(jc->threads);
	lockClose(jc->lock);
	my_free(jc);
}

job_controller *jobControllerNew (TVLCPLAYER *vp, unsigned int (__stdcall *workerFunc) (void *))
{
	job_controller *jc = my_calloc(1, sizeof(job_controller));
	if (jc){
		jc->threads = (job_Thread**)my_calloc(1, sizeof(job_Thread**));
		jc->lock = lockCreate(__func__);
		jc->workerFunc = workerFunc;
		jc->waitPeriod = 1000;
		jc->jobIdSrc = 100;
		jc->vp = vp;
	}
	
	return jc;
}

#endif
