/******************************************************************************
 *                                                                            *
 *              M u l i t h r e a d i n g   S u p p o r t                     *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 2004,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
 ******************************************************************************
 * This library is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU Lesser General Public                 *
 * License as published by the Free Software Foundation; either               *
 * version 2.1 of the License, or (at your option) any later version.         *
 *                                                                            *
 * This library is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU          *
 * Lesser General Public License for more details.                            *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public           *
 * License along with this library; if not, write to the Free Software        *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA. *
 ******************************************************************************
 * $Id: FXThread.cpp,v 1.53.2.12 2008/06/18 20:03:46 fox Exp $                *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#include <windows.h>
#else
#include "config.h"
#include <pthread.h>
#endif

#include "fxthread.h"


#ifndef WIN32

/* Initialize mutex */
void dkMutexInit(struct dkMutex *pthis, DKbool recursive)
{
  pthread_mutexattr_t mutexatt;
  // If this fails on your machine, determine what value
  // of sizeof(pthread_mutex_t) is supposed to be on your
  // machine and mail it to: jeroen@fox-toolkit.org!!
  //FXTRACE((150,"sizeof(pthread_mutex_t)=%d\n",sizeof(pthread_mutex_t)));
  //FXASSERT(sizeof(data)>=sizeof(pthread_mutex_t));
  pthread_mutexattr_init(&mutexatt);
  pthread_mutexattr_settype(&mutexatt, recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_DEFAULT);
  pthread_mutex_init((pthread_mutex_t*)pthis->data, &mutexatt);
  pthread_mutexattr_destroy(&mutexatt);
}

/* Lock the mutex */
void dkMutexLock(struct dkMutex *m)
{
  pthread_mutex_lock((pthread_mutex_t*)m->data);
}

/* Try lock the mutex */
DKbool dkMutexTryLock(struct dkMutex *m)
{
  return pthread_mutex_trylock((pthread_mutex_t*)m->data) == 0;
}

/* Unlock mutex */
void dkMutexUnlock(struct dkMutex *m)
{
  pthread_mutex_unlock((pthread_mutex_t*)m->data);
}

/* Test if locked */
DKbool dkMutexIsLocked(struct dkMutex *m)
{
  if (pthread_mutex_trylock((pthread_mutex_t*)m->data) == 0) {
    pthread_mutex_unlock((pthread_mutex_t*)m->data);
    return 0;
  }
  return 1;
}

/* Delete mutex */
void dkMutexDestroy(struct dkMutex *m)
{
  pthread_mutex_destroy((pthread_mutex_t*)m->data);
}

#else

/* Initialize mutex */
void dkMutexInit(struct dkMutex *pthis, DKbool recursive)
{
  /* If this fails on your machine, determine what value
   * of sizeof(CRITICAL_SECTION) is supposed to be on your
   * machine and mail it to: jeroen@fox-toolkit.org!! */
  //FXTRACE((150,"sizeof(CRITICAL_SECTION)=%d\n", sizeof(CRITICAL_SECTION)));
  //FXASSERT(sizeof(data)>=sizeof(CRITICAL_SECTION));
  InitializeCriticalSection((CRITICAL_SECTION *)pthis->data);
}

/* Lock the mutex */
void dkMutexLock(struct dkMutex *m)
{
  EnterCriticalSection((CRITICAL_SECTION *)m->data);
}

/* Try lock the mutex */
DKbool dkMutexTryLock(struct dkMutex *m)
{
#if (_WIN32_WINNT >= 0x0400)
  return TryEnterCriticalSection((CRITICAL_SECTION *)m->data) != 0;
#else
  return FALSE;
#endif
}

/* Unlock mutex */
void dkMutexUnlock(struct dkMutex *m)
{
  LeaveCriticalSection((CRITICAL_SECTION *)m->data);
}

/* Test if locked */
DKbool dkMutexIsLocked(struct dkMutex *m)
{
#if (_WIN32_WINNT >= 0x0400)
  if (TryEnterCriticalSection((CRITICAL_SECTION *)m->data) != 0) {
    LeaveCriticalSection((CRITICAL_SECTION *)m->data);
    return FALSE;
  }
#endif
  return TRUE;
}

/* Delete mutex */
void dkMutexDestroy(struct dkMutex *m)
{
  DeleteCriticalSection((CRITICAL_SECTION *)m->data);
}

#endif

#ifndef WIN32
/* Get time in nanoseconds since Epoch */
DKlong dkThreadTime()
{
#ifdef __USE_POSIX199309
	DKlong seconds = 1000000000;
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME,&ts);
	return ts.tv_sec * seconds + ts.tv_nsec;
#else
	DKlong seconds = 1000000000;
	DKlong microseconds = 1000;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * seconds + tv.tv_usec * microseconds;
#endif
}

#else
/* Get time in nanoseconds since Epoch */
DKlong dkThreadTime()
{
	DKlong now;
	GetSystemTimeAsFileTime((FILETIME*)&now);
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__SC__)
	return (now - 116444736000000000LL) * 100LL;
#else
	return (now - 116444736000000000L) * 100L;
#endif
}
#endif
