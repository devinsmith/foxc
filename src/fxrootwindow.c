/******************************************************************************
 *                                                                            *
 *                  R o o t   W i n d o w   W i d g e t                       *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 1997,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: FXRootWindow.cpp,v 1.35 2006/01/22 17:58:40 fox Exp $                 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#endif

#include "fxapp.h"
#include "fxdc.h"
#include "fxdrv.h"

#include "fxrootwindow.h"

/*
  Notes:

  - Size of FXRootWindow is now size of the entire virtual display, which
    is a tiled virtual area of primary and secondary display adapters.
*/

static void DtkCreateRootWindow(void *pthis);
static void dkRootWindowPosition(struct dkWindow *pthis, int x, int y, int w, int h);
static void dkRootWindow_setFocus(struct dkWindow *w);
static void dkRootWindow_killFocus(struct dkWindow *w);
static void dkRootWindow_recalc(struct dkWindow *w);

struct dkWindow *
DtkNewRootWindow(App *app, FXVisual *v)
{
  struct dkWindow *ret;

  /* Allocate our root and send it off to the base class */
  ret = fx_alloc(sizeof(struct dkWindow));
  DtkCompositeRootInit(ret, app, v);

  /* Setup the vtbl with RootWindow specific functions */
  ret->create = DtkCreateRootWindow;
  ret->position = dkRootWindowPosition;
  ret->setFocus = dkRootWindow_setFocus;
  ret->killFocus = dkRootWindow_killFocus;
  ret->recalc = dkRootWindow_recalc;
  return ret;
}

static void
DtkCreateRootWindow(void *pthis)
{
  struct dkWindow *child;
  struct dkWindow *w;

  w = (struct dkWindow *)pthis;

  /* If it's already created, don't bother re-creating */
  if (!w->xid) {
#ifdef WIN32
	HDC hdc;

	/* Got to have a visual */
	if (!w->visual) {
		printf("No visual allocated\n");
		return;
	}

	/* Initialize visual */
	DtkCreateVisual(w->visual);

	/* Get HWND of desktop window */
	w->xid = GetDesktopWindow();

	/* Obtain size */
	hdc = GetDC((HWND)w->xid);
	w->width = GetDeviceCaps(hdc, HORZRES);
	w->height = GetDeviceCaps(hdc, VERTRES);
	ReleaseDC((HWND)w->xid, hdc);
#else
    Display *d;

    d = w->app->display;

    DtkCreateVisual(w->visual);

    w->xid = RootWindow(d, DefaultScreen(d));
    w->width = DisplayWidth(d, DefaultScreen(d));
    w->height = DisplayHeight(d, DefaultScreen(d));
#endif
    /* Normally create children */
    for (child = ((struct dkWindow *)w)->first; child; child = child->next)
      child->create(child);
  }
}

/* Move and resize root has no effect */
static void dkRootWindowPosition(struct dkWindow *pthis, int x, int y, int w, int h)
{
}

/* Root can not be focused on */
void dkRootWindow_setFocus(struct dkWindow *unused)
{
}

/* Root can not be unfocused */
void dkRootWindow_killFocus(struct dkWindow *unused)
{
}

/* Mark as dirty (does nothing) */
void dkRootWindow_recalc(struct dkWindow *unused)
{
}

#ifdef WIN32
DKID
drvGetDC(void)
{
	LockWindowUpdate(GetDesktopWindow());
	return GetDCEx(GetDesktopWindow(), NULL, DCX_CACHE | DCX_LOCKWINDOWUPDATE);
}

int
drvReleaseDC(DKID hdc)
{
	int status = ReleaseDC(GetDesktopWindow(), (HDC)hdc);
	LockWindowUpdate(NULL);
	return status;
}
#endif
