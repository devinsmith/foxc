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
 * $Id: FXThread.h,v 1.40.2.2 2006/07/26 15:25:53 fox Exp $                   *
 *****************************************************************************/

#ifndef FX_THREAD_H
#define FX_THREAD_H

#include "fxdefs.h"

/**
* FXMutex provides a mutex which can be used to enforce critical
* sections around updates of data shared by multiple threads.
*/
struct dkMutex {
  DKuval data[24];
};

/*
** Return time in nanoseconds since Epoch (Jan 1, 1970).
*/
DKlong dkThreadTime();

void dkMutexInit(struct dkMutex *pthis, DKbool recursive);
void dkMutexLock(struct dkMutex *m);
DKbool dkMutexTryLock(struct dkMutex *m);
void dkMutexUnlock(struct dkMutex *m);
DKbool dkMutexIsLocked(struct dkMutex *m);
void dkMutexDestroy(struct dkMutex *m);

#endif /* FX_THREAD_H */
