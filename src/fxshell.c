/******************************************************************************
 *                                                                            *
 *                  S h e l l   W i n d o w   W i d g e t                     *
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
 * $Id: FXShell.cpp,v 1.81 2006/01/22 17:58:41 fox Exp $                      *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "fxshell.h"

long DtkShellConfigure(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkShell_onLayout(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

static struct dkMapEntry dkShellMapEntry[] = {
  FXMAPFUNC(SEL_CONFIGURE, 0, DtkShellConfigure),
  FXMAPFUNC(SEL_CHORE, ID_LAYOUT, dkShell_onLayout)
};

static struct dkMetaClass dkShellMetaClass = {
  "dkShell", dkShellMapEntry, sizeof(dkShellMapEntry) / sizeof(dkShellMapEntry[0]), sizeof(struct dkMapEntry)
};

void
DtkTopWindowShellCtor(struct dkWindow *pthis, struct dkApp *app, DKuint opts, int x, int y, int w, int h)
{
  /* Pass to base class constructor */
  DtkCompositeShellCtor(pthis, app, NULL, opts, x, y, w, h);

  /* Setup the vtbl */
  pthis->create = DtkCreateShellWindow;
  pthis->show = DtkShellWindowShow;
  ((struct dkObject *)pthis)->handle = dkShell_handle;
  pthis->recalc = dkShellRecalc;
}

/* Handle message */
long
dkShell_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	struct dkMapEntry *me;

	me = DKMetaClassSearch(&dkShellMetaClass, DKSEL(selhi, sello));
	return me ? me->func(pthis, obj, selhi, sello, data) : dkComposite_handle(pthis, obj, selhi, sello, data);
}

/* Schedule layout to be peformed during idle time */
void
dkShellRecalc(struct dkWindow *pthis)
{
	dkAppRemoveChore(pthis->app, (struct dkObject *)pthis, ID_LAYOUT);
	dkAppAddChore(pthis->app, (struct dkObject *)pthis, ID_LAYOUT, NULL);
	pthis->flags |= FLAG_DIRTY;
}

void
DtkCreateShellWindow(void *pthis)
{
	struct dkWindow *sh;
	int w, h;

	sh = (struct dkWindow *)pthis;
	DtkCreateComposite((struct dkComposite *)pthis);

	/* Adjust size if necessary */
	w = (1 < sh->width) ? sh->width : sh->getDefaultWidth(sh);
	h = (1 < sh->height) ? sh->height : sh->getDefaultHeight(sh);

	/* Resize this widget */
	sh->resize(sh, w, h);
}

long
DtkShellConfigure(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	struct dkEvent *ev = (struct dkEvent *)data;
	struct dkWindow *w = (struct dkWindow *)pthis;

	w->xpos = ev->rect.x;
	w->ypos = ev->rect.y;

	if ((ev->rect.w != w->width) || (ev->rect.h != w->height)) {
		w->width = ev->rect.w;               /* Record new size */
		w->height = ev->rect.h;
		/* Delayed layout optimization. The delayed layout optimization
		 * currently only works on UNIX.  On Windows, the program enters
		 * a modal loop during a window-resize operation.  During this
		 * modal loop, which is somewhere inside WIN32 code, we are completely
		 * deaf to other event sources such as timers, chores, file i/o, and
		 * are unable to perform idle processing.  So the chore we would set
		 * in recalc() would never fire until we're all done with the resizing.
		 * We'd love to have a fix for this, but it seems difficult because of
		 * the need to pass "non-client" events over to the DefWindowProc... */
#ifndef WIN32
		w->recalc(w);           // On UNIX, we process idle messages during a resize
#else
		w->layout(w);           // On Windows, we are in a modal loop and we have to force it
#endif
	}

	return 1;
}

/* Perform layout; return 0 because no GUI update is needed */
long
dkShell_onLayout(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkWindow *s = (struct dkWindow *)pthis;

  s->layout(s);
  return 0;
}

void
DtkShellWindowShow(struct dkWindow *w)
{
	DtkWindowShow(w);
}
