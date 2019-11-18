/******************************************************************************
 *                                                                            *
 *                  M a i n   W i n d o w   W i d g e t                       *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 1998,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: FXMainWindow.cpp,v 1.31 2006/01/22 17:58:35 fox Exp $                 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include <X11/Xlib.h>
#endif

#include "fxapp.h"
#include "fxdc.h"
#include "fxmainwindow.h"

static void DtkCreateMainWindow(void *pthis);

struct dkTopWindow *
dkMainWindowNew(App *app, char *title)
{
	struct dkTopWindow *ret;

	/* Send off to our other class */
	ret = dkMainWindowNewEx(app, title, DECOR_ALL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	return ret;
}

struct dkTopWindow *
dkMainWindowNewEx(struct dkApp *app, char *title, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs)
{
	struct dkTopWindow *ret;

  /* Allocate our main window and send it off to the base class */
  ret = fx_alloc(sizeof(struct dkTopWindow));

  /* Send off to base class */
  DtkTopWindowCtor(ret, app, title, opts, x, y, w, h, pl, pr, pt, pb, hs, vs);

  /* Setup our vtbl.  The only function we implement is the create function */
  ((struct dkWindow *)ret)->create = DtkCreateMainWindow;

  return ret;

}

/* Create server-side resources */
static void
DtkCreateMainWindow(void *pthis)
{
	struct dkWindow *w = (struct dkWindow *)pthis;
	DtkCreateTopWindow(w);
	if (w->xid) {
		if (w->app->display_opened) {
#ifndef WIN32
			/* Set the WM_COMMAND hint on non-owned toplevel windows */
			XSetCommand((Display *)w->app->display, w->xid, (char**)w->app->appArgv,
			    w->app->appArgc);
#endif
		}
	}
}
