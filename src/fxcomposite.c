/******************************************************************************
 *                                                                            *
 *                    C o m p o s i t e   W i d g e t                         *
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
 * $Id: FXComposite.cpp,v 1.54.2.2 2007/04/29 14:31:43 fox Exp $              *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "fxapp.h"
#include "fxcomposite.h"
#include "fxkeys.h"

/*
  Notes:
  - Rather a slim class.
  - Focus should be assigned to a window via SEL_FOCUSELF message.
    Composite widgets won't have focus so SEL_FOCUSELF should return 0
    and do nothing.
  - Maybe add flag to exempt a widget from maxChildWidth() and/or maxChildHeight()
    so that things like separators can stay small when everything else gets
    as big as the biggest child.
*/

/* Message handlers */
static long dkComposite_onKeyPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkComposite_onKeyRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);

/* Map */
static struct dkMapEntry dkCompositeMapEntry[] = {
  FXMAPFUNC(SEL_KEYPRESS, 0, dkComposite_onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE, 0, dkComposite_onKeyRelease)
};

/* Object implementation */
static struct dkMetaClass dkCompositeMetaClass = {
  "dkComposite", dkCompositeMapEntry, sizeof(dkCompositeMapEntry) / sizeof(dkCompositeMapEntry[0]), sizeof(struct dkMapEntry)
};

static int dkComposite_getDefaultWidth(struct dkWindow *win);
static int dkComposite_getDefaultHeight(struct dkWindow *win);

void dkCompositeVtblSetup(struct dkWindow *comp)
{
  /* Setup the vtbl with dkComposite specific functions */
  comp->create = DtkCreateComposite;
  ((struct dkObject *)comp)->handle = dkComposite_handle;
  comp->layout = dkCompositeLayout;
  comp->getDefaultWidth = dkComposite_getDefaultWidth;
  comp->getDefaultHeight = dkComposite_getDefaultHeight;
}

/* Only used for Root Window */
void
DtkCompositeRootInit(struct dkWindow *comp, struct dkApp *app, struct FXVisual *v)
{
  DtkWindowRootInit(comp, app, v);
  ((struct dkObject *)comp)->meta = &dkCompositeMetaClass;
  dkCompositeVtblSetup(comp);
}

/* Only used for Shell Window */
void
DtkCompositeShellCtor(struct dkWindow *pthis, struct dkApp *app, struct dkWindow *win, DKuint opts, int x, int y, int w, int h)
{
  DtkWindowShellCtor(pthis, app, win, opts, x, y, w, h);
  ((struct dkObject *)pthis)->meta = &dkCompositeMetaClass;
  dkCompositeVtblSetup(pthis);
}

/* Create empty composite window */
void dkCompositeInit(struct dkWindow *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h)
{
  dkWindowChildInit(pthis, p, opts, x, y, w, h);
  ((struct dkObject *)pthis)->meta = &dkCompositeMetaClass;
  dkCompositeVtblSetup(pthis);
}

/* Handle message */
long
dkComposite_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;
  DKSelector sel = DKSEL(selhi, sello);

  me = DKMetaClassSearch(&dkCompositeMetaClass, sel);
  return me ? me->func(pthis, obj, selhi, sello, data) : dkWindow_handle(pthis, obj, selhi, sello, data);
}

/* Get width */
static int dkComposite_getDefaultWidth(struct dkWindow *win)
{
	struct dkWindow *child;
	int t, w = 0;

	for (child = win->first; child; child = child->next) {
		if (dkWindowIsShown(child)) {
			t = child->xpos + child->width;
			if (w < t) w = t;
		}
	}
	return w;
}

/* Get height */
static int dkComposite_getDefaultHeight(struct dkWindow *win)
{
	struct dkWindow *child;
	int t, h = 0;

	for (child = win->first; child; child = child->next) {
		if (dkWindowIsShown(child)) {
			t = child->ypos + child->height;
			if (h < t) h = t;
		}
	}
	return h;
}

void
dkCompositeLayout(struct dkWindow *w)
{
	struct dkWindow *child;

	for (child = w->first; child; child = child->next) {
		if (dkWindowIsShown(child)) {
			child->position(child, child->xpos, child->ypos, child->width, child->height);
		}
	}
	w->flags &= ~FLAG_DIRTY;
}

/* Get maximum child width */
int dkCompositeMaxChildWidth(struct dkWindow *comp)
{
	struct dkWindow *child;
	DKuint hints;
	int t, m;
	for (m = 0, child = comp->first; child; child = child->next) {
		if (dkWindowIsShown(child)) {
			hints = dkWindowGetLayoutHints(child);
			if (hints & LAYOUT_FIX_WIDTH) t = child->width;
			else t = child->getDefaultWidth(child);
			if (m < t) m = t;
		}
	}
	return m;
}

/* Get maximum child height */
int dkCompositeMaxChildHeight(struct dkWindow *comp)
{
	struct dkWindow *child;
	DKuint hints;
	int t, m;
	for (m = 0, child = comp->first; child; child = child->next) {
		if (dkWindowIsShown(child)) {
			hints = dkWindowGetLayoutHints(child);
			if (hints & LAYOUT_FIX_HEIGHT) t = child->height;
			else t = child->getDefaultHeight(child);
			if (m < t) m = t;
		}
	}
	return m;
}

void
DtkCreateComposite(void *pthis)
{
	struct dkWindow *w = (struct dkWindow *)pthis;
	struct dkWindow *child;

	DtkCreateWindow(w);
	/* Create all the children */
	for (child = w->first; child; child = child->next)
		child->create(child);
}

static long dkComposite_onKeyPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  struct dkEvent *ev = (struct dkEvent *)ptr;

  DKTRACE((200, "%p->%s::onKeyPress keysym=0x%04x state=%04x\n", pthis, ((struct dkObject *)w)->meta->className, ev->code, ev->state));

  /* Bounce to focus widget */
  if (w->focus && ((struct dkObject *)w->focus)->handle(w->focus, obj, selhi, sello, ptr))
    return 1;

  /* Try target first */
  if (dkWindowIsEnabled(w) && w->target && w->target->handle(w->target, pthis, SEL_KEYPRESS, w->message, ptr))
    return 1;

  /* Check the accelerators */
  /* XXX: We don't support accelerators yet */

  /* Otherwise, perform the default keyboard processing */
  switch (MKUINT(ev->code, ev->state & (SHIFTMASK | CONTROLMASK | ALTMASK | METAMASK))) {
    case KEY_Tab:
    case KEY_Next:
      return dkComposite_handle(pthis, (struct dkObject *)w, SEL_FOCUS_NEXT, 0, ptr);
    case KEY_Prior:
    case KEY_ISO_Left_Tab:
    case MKUINT(KEY_ISO_Left_Tab, SHIFTMASK):
    case MKUINT(KEY_Tab, SHIFTMASK):
      return dkComposite_handle(pthis, (struct dkObject *)w, SEL_FOCUS_PREV, 0, ptr);
    case KEY_Up:
    case KEY_KP_Up:
      return dkComposite_handle(pthis, (struct dkObject *)w, SEL_FOCUS_UP, 0, ptr);
    case KEY_Down:
    case KEY_KP_Down:
      return dkComposite_handle(pthis, (struct dkObject *)w, SEL_FOCUS_DOWN, 0, ptr);
    case KEY_Left:
    case KEY_KP_Left:
      return dkComposite_handle(pthis, (struct dkObject *)w, SEL_FOCUS_LEFT, 0, ptr);
    case KEY_Right:
    case KEY_KP_Right:
      return dkComposite_handle(pthis, (struct dkObject *)w, SEL_FOCUS_RIGHT, 0, ptr);
  }
  return 0;
}

static long dkComposite_onKeyRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  struct dkEvent *ev = (struct dkEvent *)ptr;

  DKTRACE((200, "%p->%s::onKeyPress keysym=0x%04x state=%04x\n", pthis, ((struct dkObject *)w)->meta->className, ev->code, ev->state));

  /* Bounce to focus widget */
  if (w->focus && ((struct dkObject *)w->focus)->handle(w->focus, obj, selhi, sello, ptr))
    return 1;

  /* Try target first */
  if (dkWindowIsEnabled(w) && w->target && w->target->handle(w->target, pthis, SEL_KEYRELEASE, w->message, ptr))
    return 1;

  /* Check the accelerators */
  /* XXX: TODO!! */

  return 0;
}

