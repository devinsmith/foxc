/******************************************************************************
 *                                                                            *
 *           P r i v a t e   I n t e r n a l   F u n c t i o n s              *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 2000,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: fxpriv.cpp,v 1.47.2.2 2007/06/04 13:09:50 fox Exp $                   *
 *****************************************************************************/

#ifdef WIN32
#include <windows.h>
#endif

#include "fxpriv.h"


/*
  Notes:
  - This file does actual data transfer for clipboard, selection, and drag and drop.
  - Perhaps we should also implement INCR for sending; however, we don't know for
    sure if the other side supports this.
*/

#ifdef WIN32
/*******************************************************************************/

// When called, grab the true API from the DLL if we can
static BOOL WINAPI MyGetMonitorInfo(HANDLE monitor,MYMONITORINFO* minfo){
  HINSTANCE hUser32;
  PFNGETMONITORINFO gmi;
  if((hUser32=GetModuleHandleA("USER32")) && (gmi=(PFNGETMONITORINFO)GetProcAddress(hUser32,"GetMonitorInfoA"))){
    dkGetMonitorInfo=gmi;
    return dkGetMonitorInfo(monitor,minfo);
    }
  return 0;
  }


// When called, grab the true API from the DLL if we can
static HANDLE WINAPI MyMonitorFromRect(RECT* rect,DWORD flags){
  HINSTANCE hUser32;
  PFNMONITORFROMRECT mfr;
  if((hUser32=GetModuleHandleA("USER32")) && (mfr=(PFNMONITORFROMRECT)GetProcAddress(hUser32,"MonitorFromRect"))){
    dkMonitorFromRect=mfr;
    return dkMonitorFromRect(rect,flags);
    }
  return NULL;
}

PFNGETMONITORINFO dkGetMonitorInfo = MyGetMonitorInfo;
PFNMONITORFROMRECT dkMonitorFromRect = MyMonitorFromRect;
#endif

