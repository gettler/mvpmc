/*
 *  Copyright (C) 2004, 2005, 2006, Jon Gettler
 *  http://www.mvpmc.org/
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mvp_av.h>
#include <pthread.h>
#include <unistd.h>

#if 0
#define PRINTF(x...) printf(x);fflush(stdout);
#else
#define PRINTF(x...)
#endif
#define VID_EVENT_MAX_WAIT (PTS_HZ*10)

/* When a discontinuity is signalled then check every .1 of a second for 5
 * seconds because we may end up jumping around */
#define VID_EVENT_DISCON_MAX_WAIT (PTS_HZ*.1)
#define VID_EVENT_DISCON_WAIT_COUNT 50

typedef struct event_queue_s event_queue_t;

struct event_queue_s {
    event_queue_t * pNext;
    event_queue_t * pPrev;
    unsigned int pts;
    eventq_type_t type;
    void * info;
};
static event_queue_t *pNextEvent = NULL;
static int pts_discontinuity_count = 0;
static pthread_mutex_t videvents_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t videvents_cond = PTHREAD_COND_INITIALIZER;

static inline unsigned int get_pts()
{
    pts_sync_data_t pts_struct;
    av_get_video_sync(&pts_struct);
    return pts_struct.stc & 0xFFFFFFFF;
}


int vid_event_wait_next(eventq_type_t * type, void **info)
{
    event_queue_t *ret = NULL;
    pthread_mutex_lock(&videvents_mutex);
    /* NB pts are clocked at 90kHz */
    while(ret == NULL)
    {
	unsigned int pts = get_pts();
	/*Anything less than 1 (NTSC) frame should count as now */
	unsigned int max_pts = pts + (PTS_HZ/30) -1;
	/*Assume anything less than 30 seconds behind us counts as now*/
	unsigned int seek_pts = pts - PTS_HZ*30;
	/*And anything more than 40 seconds ahead we don't really need to worry about*/
	unsigned int max_seek_pts = pts + PTS_HZ*40;
	event_queue_t *pCurrent = pNextEvent;
	event_queue_t *pStart;

	PRINTF("VEQ: Looking for a worthwhile elem: %p\n",pCurrent);
	if(pCurrent != NULL)
	{
	    /*Find first event after seek_pts and before max_seek_pts */
	    pStart = pCurrent;
	    /*First move to an element before seek_pts, if it exists */
	    while(pCurrent->pts >= seek_pts)
	    {
		pCurrent = pCurrent->pPrev;
		/*The smallest element is the best we can get. Even if it
		 * is greater than seek_pts, there's not going to be another
		 * one that is less.
		 */
		if(pCurrent->pPrev->pts > pCurrent->pts)
		    break;
		/*We went all the way around, probably only one item */
		if(pCurrent == pStart)
		    break;
	    }
	    pStart = pCurrent;
	    /* Now move forward to the first element after this that is
	     * >= seek_pts
	     */
	    while(pCurrent->pts < seek_pts)
	    {
		pCurrent = pCurrent->pNext;
		/*Numbers can't get any bigger if we're about to wrap
		 * around
		 */
		if(pCurrent->pNext->pts < pCurrent->pts)
		    break;
		/*We went all the way around, probably only one item */
		if(pCurrent == pStart)
		    break;
	    }

	    /*Check if the element we've found is between seek_pts and
	     * max_seek_pts
	     */
	    if(seek_pts > max_seek_pts)
	    {
		/* PTS is about to/has wrapped around, maths is a bit different */
		if(pCurrent->pts < seek_pts && pCurrent->pts > max_seek_pts)
		    pCurrent = NULL;
	    }
	    else
	    {
		if(pCurrent->pts < seek_pts || pCurrent->pts > max_seek_pts)
		    pCurrent = NULL;
	    }
	}
	/*Check if the element is before 1 frame from now, if that's the
	 * case then dispatch the event
	 */
	PRINTF("VEQ: Found elem: %p\n",pCurrent);
	if(pCurrent != NULL && (pCurrent->pts <= max_pts ||(seek_pts > max_pts && pCurrent->pts >= seek_pts)))
	{
	    ret = pCurrent;
	    PRINTF("VEQ: returning %p\n",ret);
	}
	else
	{
	    int to_wait;
	    struct timespec abstime;
	    /*Never wait any more than 10 seconds*/
	    int max_wait;
	    if(pts_discontinuity_count > 0)
	    {
		pts_discontinuity_count--;
		max_wait = VID_EVENT_DISCON_MAX_WAIT;
	    }
	    else
		max_wait = VID_EVENT_MAX_WAIT;
	    if(pCurrent != NULL)
	    {
		if(pCurrent->pts > pts)
		    to_wait = pCurrent->pts - pts;
		else
		    /* Bitwise not of pts is the same as 0xFFFFFFFF - pts */
		    to_wait = (~pts) + pCurrent->pts;

		/* Exponentially approach the pts we're after*/
		to_wait = (to_wait*8)/10;

		if(to_wait > max_wait)
		    to_wait = max_wait;
	    }
	    else
		to_wait = max_wait;


	    clock_gettime(CLOCK_REALTIME,&abstime);
	    abstime.tv_nsec += ((to_wait%PTS_HZ) *(1000000000/PTS_HZ));
	    while(abstime.tv_nsec >= 1000000000)
	    {
		abstime.tv_sec++;
		abstime.tv_nsec -= 1000000000;
	    }
	    abstime.tv_sec += to_wait/PTS_HZ;
	    PRINTF("VEQ: Waiting for %d pts clocks, until sec %lu\n",to_wait,(unsigned long)abstime.tv_sec);
	    pthread_cond_timedwait(&videvents_cond,&videvents_mutex,&abstime);
	    PRINTF("VEQ: I'm awake now...\n");
	}
	if(ret != NULL)
	{
	    PRINTF("Dispatching event for pts %x at pts %x (seek %x, seek_max %x)\n",ret->pts,pts,seek_pts,max_seek_pts);
	}
    }
    int retval;
    if(ret == NULL)
    {
	/* Shouldn't happen... */
	retval = -1;
    }
    else
    {
	*type = ret->type;
	*info = ret->info;
	if(ret->pNext == ret)
	{
	    /*1 item in the queue*/
	    pNextEvent = NULL;
	}
	else
	{
	    if(pNextEvent == ret)
		pNextEvent = ret->pNext;
	    ret->pPrev->pNext = ret->pNext;
	    ret->pNext->pPrev = ret->pPrev;
	}
	free(ret);
	retval = 0;
    }
    pthread_mutex_unlock(&videvents_mutex);
    return retval;
}

int vid_event_add(unsigned int pts, eventq_type_t type, void * info)
{
    event_queue_t * pNewElem = malloc(sizeof(*pNewElem));
    event_queue_t * pCurrent = NULL;
    if(pNewElem == NULL)
	return -1;
    PRINTF("VEQA: Trying to add new element...\n");
    pNewElem->pts = pts;
    pNewElem->type = type;
    pNewElem->info = info;
    pthread_mutex_lock(&videvents_mutex);
    pCurrent = pNextEvent;
    if(pCurrent == NULL)
    {
	pNewElem->pNext = pNewElem;
	pNewElem->pPrev = pNewElem;
	pNextEvent = pNewElem;
    }
    else if(pCurrent->pts <= pts)
    {
	    while(pCurrent->pts <= pts)
	    {
		pCurrent = pCurrent->pNext;
		if(pCurrent->pPrev->pts > pCurrent->pts)
		    /*We've wrapped around, so we want to slot in here*/
		    break;
		if(pCurrent == pNextEvent)
		    /*We've gone around in a circle, probably there's only
		     * one element, or all the elements have the same PTS
		     */
		    break;
	    }
	    pNewElem->pPrev = pCurrent->pPrev;
	    pNewElem->pNext = pCurrent;
	    pNewElem->pPrev->pNext = pNewElem;
	    pNewElem->pNext->pPrev = pNewElem;
	   
    }
    else if(pCurrent->pts > pts)
    {
	    while(pCurrent->pts > pts)
	    {
		pCurrent = pCurrent->pPrev;
		if(pCurrent->pNext->pts > pCurrent->pts)
		    /*We've wrapped around, so we want to slot in here*/
		    break;
		if(pCurrent == pNextEvent)
		    /*We've gone around in a circle, probably there's only
		     * one element, or all the elements have the same PTS
		     */
		    break;
	    }
	    pNewElem->pPrev = pCurrent;
	    pNewElem->pNext = pCurrent->pNext;
	    pNewElem->pPrev->pNext = pNewElem;
	    pNewElem->pNext->pPrev = pNewElem;
    }
    PRINTF("VEQA: Done...\n");
    pthread_cond_signal(&videvents_cond);
    pthread_mutex_unlock(&videvents_mutex);
    PRINTF("VEQA: Signalled and unlocked...\n");
    return 0;
}
void vid_event_discontinuity_possible()
{
    pthread_mutex_lock(&videvents_mutex);
    pts_discontinuity_count = VID_EVENT_DISCON_WAIT_COUNT;
    pthread_cond_signal(&videvents_cond);
    pthread_mutex_unlock(&videvents_mutex);
}

void vid_event_clear()
{
    event_queue_t * pCur;
    pthread_mutex_lock(&videvents_mutex);
    pCur = pNextEvent;
    while(pCur != NULL)
    {
	event_queue_t * pNext = pCur->pNext;
	if(pNext == pNextEvent)
	    pNext = NULL;
	free(pCur);
	pCur = pNext;
    }
    pNextEvent = NULL;
    pts_discontinuity_count = VID_EVENT_DISCON_WAIT_COUNT;
    /*No point in signalling any waiting thread, that'll happen as soon
     * as something is added onto the queue
     */
    pthread_mutex_unlock(&videvents_mutex);
}

