/******************************************************************************
 *                                                                            *
 *                         W i n d o w   O b j e c t                          *
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
 * $Id: FXWindow.cpp,v 1.341.2.3 2009/01/14 10:41:48 fox Exp $                *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#endif

#include "fxapp.h"
#include "fxdc.h"
#include "fxdrv.h"
#include "fxpriv.h"
#include "fxwindow.h"

#ifndef WIN32

/* Basic events */
#define BASIC_EVENT_MASK   (StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask|KeyPressMask|KeyReleaseMask|KeymapStateMask)

/* Additional events for shell widget events */
#define SHELL_EVENT_MASK   (FocusChangeMask|StructureNotifyMask)

/* Additional events for enabled widgets */
#define ENABLED_EVENT_MASK (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)

/* These events are grabbed for mouse grabs */
#define GRAB_EVENT_MASK    (ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask)

#endif

/* Side layout modes */
#define LAYOUT_SIDE_MASK (LAYOUT_SIDE_LEFT|LAYOUT_SIDE_RIGHT|LAYOUT_SIDE_TOP|LAYOUT_SIDE_BOTTOM)

/* Layout modes */
#define LAYOUT_MASK (LAYOUT_SIDE_MASK|LAYOUT_RIGHT|LAYOUT_CENTER_X|LAYOUT_BOTTOM|LAYOUT_CENTER_Y|LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT|LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_DOCK_SAME|LAYOUT_DOCK_NEXT)

static struct dtkHash *dtkWindowHash;
static int dkWindowGetDefaultWidth(struct dkWindow *w);
static int dkWindowGetDefaultHeight(struct dkWindow *w);
static int dkWindowGetWidthForHeight(struct dkWindow *w, int height);
static int dkWindowGetHeightForWidth(struct dkWindow *w, int width);
static void dkWindowPosition(struct dkWindow *win, int x, int y, int w, int h);
static int dkWindow_doesSaveUnder(void);
static int dkWindow_canFocus(void);
void dkWindow_changeFocus(struct dkWindow *pthis, struct dkWindow *child);
void dkWindow_enable(struct dkWindow *w);
void dkWindow_disable(struct dkWindow *w);

/* Message handlers */
static long DtkWindowPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkWindow_onMap(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkWindow_onMotion(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkWindow_onConfigure(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkWindow_onLeftBtnPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkWindow_onLeftBtnRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkWindow_onKeyPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkWindow_onKeyRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);

static struct dkMapEntry dkWindowMapEntry[] = {
  FXMAPFUNC(SEL_UPDATE, 0, dkWindow_onUpdate),
  FXMAPFUNC(SEL_PAINT, 0, DtkWindowPaint),
  FXMAPFUNC(SEL_MOTION, 0, dkWindow_onMotion),
  FXMAPFUNC(SEL_CONFIGURE, 0, dkWindow_onConfigure),
  FXMAPFUNC(SEL_MAP, 0, dkWindow_onMap),
  FXMAPFUNC(SEL_ENTER, 0, dkWindow_onEnter),
  FXMAPFUNC(SEL_LEAVE, 0, dkWindow_onLeave),
  FXMAPFUNC(SEL_FOCUSIN, 0, dkWindow_onFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT, 0, dkWindow_onFocusOut),
  FXMAPFUNC(SEL_SELECTION_LOST, 0, dkWindow_onSelectionLost),
  FXMAPFUNC(SEL_SELECTION_GAINED, 0, dkWindow_onSelectionGained),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, dkWindow_onLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, dkWindow_onLeftBtnRelease),
  FXMAPFUNC(SEL_KEYPRESS, 0, dkWindow_onKeyPress),
  FXMAPFUNC(SEL_KEYRELEASE, 0, dkWindow_onKeyRelease),
  FXMAPFUNC(SEL_FOCUS_SELF, 0, dkWindow_onFocusSelf)
};

static struct dkMetaClass dkWindowMetaClass = {
  "dkWindow", dkWindowMapEntry, sizeof(dkWindowMapEntry) / sizeof(dkWindowMapEntry[0]), sizeof(struct dkMapEntry)
};

/* Drag type atoms; first widget to need it should register the type */
DKDragType dkdeleteType = 0;
DKDragType dktextType = 0;
DKDragType dkcolorType = 0;
DKDragType dkurilistType = 0;
DKDragType dkutf8Type = 0;
DKDragType dkoctetType = 0;

/* The string type is predefined and hardwired */
#ifndef WIN32
DKDragType dkstringType = XA_STRING;
#else
DKDragType dkstringType = CF_TEXT;
#endif

static void DtkDrawableResize(struct dkWindow *d, int w, int h);

/* Initialize nicely */
void
DtkDrawableInit(struct dkWindow *d, App *app, int w, int h)
{
	/* Base class */
	dkObjectInit((struct dkObject *)d);
	d->app = app;
	d->xid = 0;

	/* Setup vtbl */
	d->resize = DtkDrawableResize;

	/* Initialize members */
	d->visual = NULL;
	d->width = FXMAX(w, 1);
	d->height = FXMAX(h, 1);
}

static void
DtkDrawableResize(struct dkWindow *d, int w, int h)
{
	d->width = FXMAX(w, 1);
	d->height = FXMAX(h, 1);
}

/* The image type is predefined and hardwired */
#ifndef WIN32
DKDragType dkimageType = XA_PIXMAP;
#else
DKDragType dkimageType = CF_DIB;
#endif

/* The UTF-16 string type is predefined and hardwired */
#ifndef WIN32
DKDragType dkutf16Type = 0;
#else
DKDragType dkutf16Type = CF_UNICODETEXT;
#endif

void
DtkWindowHashNew(void)
{
  dtkWindowHash = DtkHashNew();
}

void
DtkWindowHashInsert(void *key, void *value)
{
	DtkHashInsert(dtkWindowHash, key, value);
}

/* Handle message */
long
dkWindow_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	struct dkMapEntry *me;

	me = DKMetaClassSearch(&dkWindowMetaClass, DKSEL(selhi, sello));
	return me ? me->func(pthis, obj, selhi, sello, data) : dkObject_handle(pthis, obj, selhi, sello, data);
}

static void dkWindow_setftable(struct dkWindow *win)
{
  /* Setup vtbl */
  ((struct dkObject *)win)->handle = dkWindow_handle;
  win->create = DtkCreateWindow;
  win->show = DtkWindowShow;
  win->layout = dkWindowLayout;
  win->recalc = dkWindowRecalc;
  win->position = dkWindowPosition;
  win->getDefaultWidth = dkWindowGetDefaultWidth;
  win->getDefaultHeight = dkWindowGetDefaultHeight;
  win->getWidthForHeight = dkWindowGetWidthForHeight;
  win->getHeightForWidth = dkWindowGetHeightForWidth;
  win->doesSaveUnder = dkWindow_doesSaveUnder;
  win->canFocus = dkWindow_canFocus;
  win->setFocus = dkWindow_setFocus;
  win->killFocus = dkWindow_killFocus;
  win->changeFocus = dkWindow_changeFocus;
  win->setDefault = dkWindow_setDefault;
  win->enable = dkWindow_enable;
  win->disable = dkWindow_disable;
}

/* This constructor is used for shell windows */
void
DtkWindowShellCtor(struct dkWindow *win, struct dkApp *app, struct dkWindow *own, DKuint opts, int x, int y, int w, int h)
{
	DtkDrawableInit(win, app, w, h);

	/* Temporary */
	win->classname = "DTKWindow";

	win->parent = (struct dkWindow *)app->root;
	win->owner = own;
	win->visual = app->defaultVisual;
	win->first = win->last = NULL;
	win->prev = win->parent->last;
	win->next = NULL;
	win->parent->last = win;
	if (win->prev) {
		win->wk = win->prev->wk + 1;
		win->prev->next = win;
	} else {
		win->wk = 1;
		win->parent->first = win;
	}
  win->focus = NULL;
  win->composeContext = NULL;
  win->defaultCursor = app->cursor[DEF_ARROW_CURSOR];
  win->dragCursor = app->cursor[DEF_ARROW_CURSOR];
  win->target = NULL;
	win->message = 0;
	win->xpos = x;
	win->ypos = y;
	win->backColor = app->baseColor;
	win->flags = FLAG_DIRTY | FLAG_UPDATE | FLAG_RECALC | FLAG_SHELL;
	win->options = opts;

  dkWindow_setftable(win);
}

/* Only used for the root window */
void
DtkWindowRootInit(struct dkWindow *w, struct dkApp *app, struct FXVisual *v)
{
  DtkDrawableInit(w, app, 1, 1);

  w->visual = app->defaultVisual;
  w->parent = NULL;
  w->owner = NULL;
  w->first = w->last = NULL;
  w->next = w->prev = NULL;
  w->focus = NULL;
  w->composeContext = NULL;
  w->defaultCursor = app->cursor[DEF_ARROW_CURSOR];
  w->dragCursor = app->cursor[DEF_ARROW_CURSOR];
  w->classname = "DTKWindow";
  w->target = NULL;
  w->message = 0;
  w->xpos = 0;
  w->ypos = 0;
  w->backColor = 0;
  w->flags = FLAG_DIRTY | FLAG_SHOWN | FLAG_UPDATE | FLAG_RECALC;
  w->options = LAYOUT_FIX_X | LAYOUT_FIX_Y | LAYOUT_FIX_WIDTH | LAYOUT_FIX_HEIGHT;
  w->wk = 1;

  dkWindow_setftable(w);
}

/* This constructor is used for all child windows */
void dkWindowChildInit(struct dkWindow *pthis, struct dkWindow *comp, DKuint opts, int x, int y, int w, int h)
{
  DtkDrawableInit(pthis, comp->app, w, h);

  /* Temporary */
  pthis->classname = "DTKWindow";

	pthis->parent = comp;
	pthis->owner = pthis->parent;

	pthis->visual = pthis->parent->visual;
	pthis->first = pthis->last = NULL;
	pthis->prev = pthis->parent->last;
	pthis->next = NULL;
	pthis->parent->last = pthis;
	if (pthis->prev) {
		pthis->wk = pthis->prev->wk + 1;
		pthis->prev->next = pthis;
	}
	else {
		pthis->wk = 1;
		pthis->parent->first = pthis;
	}
  pthis->focus = NULL;
  pthis->composeContext = NULL;
  pthis->defaultCursor = comp->app->cursor[DEF_ARROW_CURSOR];
  pthis->dragCursor = comp->app->cursor[DEF_ARROW_CURSOR];
	//accelTable=NULL;
	pthis->target = NULL;
	pthis->message = 0;
	pthis->xpos = x;
	pthis->ypos = y;
	pthis->backColor = pthis->app->baseColor;
	pthis->flags = FLAG_DIRTY | FLAG_UPDATE | FLAG_RECALC;
	pthis->options = opts;

  dkWindow_setftable(pthis);
}

static void dkWindowPosition(struct dkWindow *win, int x, int y, int w, int h)
{
  int ow = win->width;
  int oh = win->height;

	if (w < 0) w = 0;
	if (h < 0) h = 0;
	if ((win->flags & FLAG_DIRTY) || (x != win->xpos) || (y != win->ypos) || (w != ow) || (h != oh)) {
    win->xpos = x;
    win->ypos = y;
    win->width = w;
    win->height = h;
    if (win->xid) {
			// Alas, we have to generate some protocol here even if the placement
			// as recorded in the widget hasn't actually changed.  This is because
			// there are ways to change the placement w/o going through
			// position()!
#ifndef WIN32
			if (0 < w && 0 < h) {
				if((win->flags & FLAG_SHOWN) && (ow <= 0 || oh <= 0)) {
					XMapWindow(win->app->display, win->xid);
				}
				XMoveResizeWindow(win->app->display, win->xid, x, y, w, h);
			}
			else if (0 < ow && 0 < oh) {
				XUnmapWindow(win->app->display, win->xid);
			}
#else
			SetWindowPos((HWND)win->xid, NULL, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
			// We don't have to layout the interior of this widget unless
			// the size has changed or it was marked as dirty:- this is
			// a very good optimization as it's applied recursively!
			if ((win->flags & FLAG_DIRTY) || (w != ow) || (h != oh))
				win->layout(win);
		}
	}
}

static int dkWindow_canFocus(void)
{
  return 0;
}

/* Has window the focus */
int dkWindowHasFocus(struct dkWindow *w)
{
  return (w->flags & FLAG_FOCUSED) != 0;
}

/* Has this window the selection */
int dkWindowHasSelection(struct dkWindow *w)
{
  return (w->app->selectionWindow == w);
}

/* Acquire the selection.
 * We always generate SEL_SELECTION_LOST and SEL_SELECTION_GAINED
 * because we assume the selection types may have changed, and want
 * to give target opportunity to allocate the new data for these types. */
int dkWindowAcquireSelection(struct dkWindow *w, DKDragType *types, DKuint numtypes)
{
  struct dkApp *app;
  if (!types || !numtypes) { dkerror("%s::acquireSelection: should have at least one type to select.\n", ((struct dkObject *)w)->meta->className); }

  app = w->app;
  if (app->selectionWindow) {
    ((struct dkObject *)app->selectionWindow)->handle(app->selectionWindow, (struct dkObject *)app, SEL_SELECTION_LOST, 0, &app->event);
    app->selectionWindow = NULL;
    free(app->xselTypeList);
    app->xselTypeList = NULL;
    app->xselNumTypes = 0;
  }
  if (w->xid) {
#ifndef WIN32
      XSetSelectionOwner(app->display, XA_PRIMARY, w->xid, app->event.time);
      if (XGetSelectionOwner(app->display, XA_PRIMARY) != w->xid) return 0;
#endif
  }
  if (!app->selectionWindow) {
    app->selectionWindow = w;
    ((struct dkObject *)app->selectionWindow)->handle(app->selectionWindow, (struct dkObject *)app, SEL_SELECTION_GAINED, 0, &app->event);
    fx_resize((void**)&app->xselTypeList, sizeof(DKDragType) * numtypes);
    memcpy(app->xselTypeList, types, sizeof(DKDragType) * numtypes);
    app->xselNumTypes = numtypes;
  }
  return 1;
}

static int dkWindow_doesSaveUnder(void)
{
  return 0;
}

struct dkWindow *dkWindowGetRoot(struct dkWindow *pthis)
{
  struct dkWindow *win = pthis;
  while (win->parent) win = win->parent;
  return win;
}

/* Search widget tree for default window */
struct dkWindow * dkWindowFindDefault(struct dkWindow *window)
{
  struct dkWindow *win, *def;

  if (window->flags & FLAG_DEFAULT) return window;
  for (win = window->first; win; win = win->next) {
    if ((def = dkWindowFindDefault(win)) != NULL) return def;
  }
  return NULL;
}

/* Search widget tree for initial window */
struct dkWindow *dkWindowFindInitial(struct dkWindow *window)
{
  struct dkWindow *win, *ini;

  if (window->flags & FLAG_INITIAL) return window;
  for (win = window->first; win; win = win->next) {
    if ((ini = dkWindowFindInitial(win)) != NULL) return ini;
  }
  return NULL;
}


/* Make widget drawn as default */
void dkWindow_setDefault(struct dkWindow *w, DKbool enable)
{
  struct dkWindow *win;

  switch (enable) {
  case FALSE:
    w->flags &= ~FLAG_DEFAULT;
    break;
  case TRUE:
    if (!(w->flags & FLAG_DEFAULT)) {
      win = dkWindowFindDefault(DtkWindowGetShell(w));
      if (win) win->setDefault(win, FALSE);
      w->flags |= FLAG_DEFAULT;
    }
    break;
  case MAYBE:
    if (w->flags & FLAG_DEFAULT) {
      w->flags &= ~FLAG_DEFAULT;
      win = dkWindowFindInitial(DtkWindowGetShell(w));
      if (win) win->setDefault(win, TRUE);
    }
    break;
  }
}

/* Perform layout immediately */
void dkWindowLayout(struct dkWindow *pthis)
{
  pthis->flags &= ~FLAG_DIRTY;
}

/* Mark this window's layout as dirty for later layout */
void
dkWindowRecalc(struct dkWindow *pthis)
{
  if (pthis->parent)
    pthis->parent->recalc(pthis->parent);
  pthis->flags |= FLAG_DIRTY;
}

void
DtkCreateWindow(void *pthis)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  if (w->xid)
    return;

  if (!w->app->display_opened)
    return;

  DtkDrvCreateWindow(w);
  w->flags |= FLAG_OWNED;
}

/* Find window from id */
struct dkWindow *
DtkFindWindowWithId(DKID xid)
{
  return DtkHashFind(dtkWindowHash, (void *)xid);
}

/* Return a pointer to the shell window */
struct dkWindow *
DtkWindowGetShell(struct dkWindow *w)
{
  struct dkWindow *win = (struct dkWindow *)w;
  struct dkWindow *p;

  while ((p = win->parent) != NULL && p->parent)
    win = p;
  return win;
}

/* Return true if this window contains child in its subtree */
int dkWindowContainsChild(struct dkWindow *pthis, struct dkWindow *child)
{
  while (child) {
    if (child == pthis) return 1;
    child = child->parent;
  }
  return 0;
}

/* Check if logically shown */
int
dkWindowIsShown(struct dkWindow *w)
{
  return (w->flags & FLAG_SHOWN) != 0;
}

/* Return true if widget is drawn as default */
int dkWindowIsDefault(struct dkWindow *w)
{
  return (w->flags & FLAG_DEFAULT) != 0;
}

/* Enable the window */
void dkWindow_enable(struct dkWindow *w)
{
  if (!(w->flags & FLAG_ENABLED)) {
    w->flags |= FLAG_ENABLED;
    if (w->xid) {
#ifndef WIN32
      DKuint events = BASIC_EVENT_MASK | ENABLED_EVENT_MASK;
      if (w->flags & FLAG_SHELL) events |= SHELL_EVENT_MASK;
      XSelectInput(w->app->display, w->xid, events);
#else
      EnableWindow((HWND)w->xid, TRUE);
#endif
    }
  }
}

/* Disable the window */
void dkWindow_disable(struct dkWindow *w)
{
  w->killFocus(w);
  if (w->flags & FLAG_ENABLED) {
    w->flags &= ~FLAG_ENABLED;
    if (w->xid) {
#ifndef WIN32
      DKuint events = BASIC_EVENT_MASK;
      if (w->flags & FLAG_SHELL) events |= SHELL_EVENT_MASK;
      XSelectInput(w->app->display, w->xid, events);
      if (w->app->mouseGrabWindow == w) {
        XUngrabPointer(w->app->display, CurrentTime);
        XFlush(w->app->display);
        ((struct dkObject *)w)->handle(w, (struct dkObject *)w, SEL_UNGRABBED, 0, &w->app->event);
        w->app->mouseGrabWindow = NULL;
      }
      if (w->app->keyboardGrabWindow == w) {
        XUngrabKeyboard(w->app->display, w->app->event.time);
        XFlush(w->app->display);
        w->app->keyboardGrabWindow = NULL;
      }
#else
      EnableWindow((HWND)w->xid, FALSE);
      if (w->app->mouseGrabWindow == w) {
        ReleaseCapture();
        SetCursor((HCURSOR)w->defaultCursor->xid);
        ((struct dkObject *)w)->handle(w, (struct dkObject *)w, SEL_UNGRABBED, 0, &w->app->event);
        w->app->mouseGrabWindow = NULL;
      }
      if (w->app->keyboardGrabWindow == w) {
        w->app->keyboardGrabWindow = NULL;
      }
#endif
    }
  }
}

/* Is window enabled */
int dkWindowIsEnabled(struct dkWindow *w)
{
  return (w->flags & FLAG_ENABLED) != 0;
}

/* Raise (but do not activate!) */
void dkWindow_raise(struct dkWindow *w)
{
  if (w->xid) {
#ifndef WIN32
    XRaiseWindow(w->app->display, w->xid);
#else
    SetWindowPos((HWND)w->xid, HWND_TOP, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
#endif
  }
}

/* Get layout hints */
DKuint
dkWindowGetLayoutHints(struct dkWindow *w)
{
	return (w->options & LAYOUT_MASK);
}

#ifndef WIN32
// Scroll rectangle x,y,w,h by a shift of dx,dy
void dkWindow_scroll(struct dkWindow *win, int x, int y, int w, int h, int dx, int dy)
{
  if (win->xid && 0 < w && 0 < h && (dx || dy)) {
    /* No overlap:- repaint the whole thing */
    if ((w <= FXABS(dx)) || (h <= FXABS(dy))) {
      dkAppAddRepaint(win->app, win->xid, x, y, w, h, 1);
    }
    /* Has overlap, so blit contents and repaint the exposed parts */
    else {
      int tx, ty, fx, fy, ex, ey, ew, eh;
      XEvent event;

      /* Force server to catch up */
      XSync(win->app->display, False);

      /* Pull any outstanding repaint events into our own repaint rectangle list */
      while (XCheckWindowEvent(win->app->display, win->xid, ExposureMask, &event)) {
        if (event.xany.type == NoExpose) continue;
        dkAppAddRepaint(win->app, win->xid, event.xexpose.x,
            event.xexpose.y, event.xexpose.width, event.xexpose.height, 0);
        if (event.xgraphicsexpose.count == 0) break;
      }

      /* Scroll all repaint rectangles of this window by the dx, dy */
      dkAppScrollRepaints(win->app, win->xid, dx, dy);

      /* Compute blitted area */
      if (dx > 0){             /* Content shifted right */
        fx = x;
        tx = x + dx;
        ex = x;
        ew = dx;
      } else {                 /* Content shifted left */
        fx = x - dx;
        tx = x;
        ex = x + w + dx;
        ew = -dx;
      } if (dy > 0) {          /* Content shifted down */
        fy = y;
        ty = y + dy;
        ey = y;
        eh = dy;
      } else {                 /* Content shifted up */
        fy = y - dy;
        ty = y;
        ey = y + h + dy;
        eh = -dy;
      }

      /* BLIT the contents */
      XCopyArea(win->app->display, win->xid,
          win->xid, (GC)win->visual->scrollgc,
          fx, fy, w - ew, h - eh, tx, ty);

      /* Post additional rectangles for the uncovered areas */
      if (dy) {
        dkAppAddRepaint(win->app, win->xid, x, ey, w, eh, 1);
      }
      if (dx) {
        dkAppAddRepaint(win->app, win->xid, ex, y, ew, h, 1);
      }
    }
  }
}

#else
void dkWindow_scroll(struct dkWindow *win, int x, int y, int w, int h, int dx, int dy)
{
  if (win->xid && w > 0 && h > 0 && (dx || dy)) {
    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + w;
    rect.bottom = y + h;
    ScrollWindowEx((HWND)win->xid, dx, dy, &rect, &rect,
        NULL, NULL, SW_INVALIDATE);
  }
}
#endif

void
DtkWindowShow(struct dkWindow *w)
{
  if (!(w->flags & FLAG_SHOWN)) {
    w->flags |= FLAG_SHOWN;
    if (w->xid) {
#ifndef WIN32
      DtkDrvWindowShow(w);
#else
      ShowWindow((HWND)w->xid, SW_SHOWNOACTIVATE);
#endif
    }
  }
}

void dkWindowHide(struct dkWindow *w)
{
  DKTRACE((160, "%s::hide %p\n", ((struct dkObject *)w)->meta->className, w));
  if (w->flags & FLAG_SHOWN) {
    w->killFocus(w);
    w->flags &= ~FLAG_SHOWN;
    if (w->xid) {
#ifndef WIN32
      if (w->app->mouseGrabWindow == w) {
        XUngrabPointer(w->app->display, CurrentTime);
        XFlush(w->app->display);
        ((struct dkObject *)w)->handle(w, (struct dkObject *)w, SEL_UNGRABBED, 0, &w->app->event);
        w->app->mouseGrabWindow = NULL;
      }
      if (w->app->keyboardGrabWindow == w) {
        XUngrabKeyboard(w->app->display, w->app->event.time);
        XFlush(w->app->display);
        w->app->keyboardGrabWindow = NULL;
      }
      XUnmapWindow(w->app->display, w->xid);
#else
      if (w->app->mouseGrabWindow == w) {
        ReleaseCapture();
        SetCursor((HCURSOR)w->defaultCursor->xid);
        ((struct dkObject *)w)->handle(w, (struct dkObject *)w, SEL_UNGRABBED, 0, &w->app->event);
        w->app->mouseGrabWindow = NULL;
      }
      if (w->app->keyboardGrabWindow == w) {
        w->app->keyboardGrabWindow = NULL;
      }
      ShowWindow((HWND)w->xid, SW_HIDE);
#endif
    }
  }
}

/* Update dirty rectangle */
void dkWindowUpdateRect(struct dkWindow *win, int x, int y, int w, int h)
{
  if (win->xid) {

    /* We toss out rectangles outside the visible area */
    if (x >= win->width || y >= win->height || x + w <= 0 || y + h <= 0) return;

    /* Intersect with the window */
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > win->width) { w = win->width - x; }
    if (y + h > win->height) { h = win->width - y; }

    /* Append the rectangle; it is a synthetic expose event!! */
    if (w > 0 && h > 0) {
#ifndef WIN32
      dkAppAddRepaint(win->app, win->xid, x, y, w, h, 1);
#else
      RECT r;
      r.left = x;
      r.top = y;
      r.right = x + w;
      r.bottom = y + h;
      InvalidateRect((HWND)win->xid, &r, TRUE);
#endif
    }
  }
}

/* Update dirty window */
void dkWindowUpdate(struct dkWindow *win)
{
  dkWindowUpdateRect(win, 0, 0, win->width, win->height);
}

static long
DtkWindowPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dtkDC dc;
  struct dkWindow *w = (struct dkWindow *)pthis;

  dtkDrvDCEventSetup(&dc, w, (struct dkEvent *)data);
  dtkDrvDCSetForeground(&dc, w->backColor);
  dtkDrvDCFillRectangle(&dc, 0, 0, w->width, w->height);
  dkDCEnd(&dc);

  return 1;
}

/* Get coordinates from another window (for symmetry) */
void dkWindowTranslateCoordinatesFrom(struct dkWindow *pthis, int *tox, int *toy, struct dkWindow *fromwindow, int fromx, int fromy)
{
  if (fromwindow == NULL) { printf("%s: from-window is NULL.\n", __func__); }
  if (pthis->xid && fromwindow->xid) {
#ifndef WIN32
    Window tmp;
    XTranslateCoordinates(pthis->app->display, fromwindow->xid, pthis->xid, fromx, fromy, tox, toy, &tmp);
#else
    POINT pt;
    pt.x = fromx;
    pt.y = fromy;
    ClientToScreen((HWND)fromwindow->xid, &pt);
    ScreenToClient((HWND)pthis->xid, &pt);
    *tox = pt.x;
    *toy = pt.y;
#endif
  }
}

/* Get coordinates to another window (for symmetry) */
void dkWindowTranslateCoordinatesTo(struct dkWindow *pthis, int *tox, int *toy, struct dkWindow *towindow, int fromx, int fromy)
{
  if (towindow == NULL) { printf("%s to-window is NULL.\n", __func__); }
  if (pthis->xid && towindow->xid) {
#ifndef WIN32
    Window tmp;
    XTranslateCoordinates(pthis->app->display, pthis->xid, towindow->xid, fromx, fromy, tox, toy, &tmp);
#else
    POINT pt;
    pt.x = fromx;
    pt.y = fromy;
    ClientToScreen((HWND)pthis->xid, &pt);
    ScreenToClient((HWND)towindow->xid, &pt);
    *tox = pt.x;
    *toy = pt.y;
#endif
  }
}

/* Acquire grab; also switches to the drag cursor */
void dkWindowGrab(struct dkWindow *win)
{
  struct dkApp *app = win->app;

  if (win->xid) {
    if (win->dragCursor->xid == 0) {
      printf("%s: Cursor has not been created yet.\n", __func__);
    }
    if (!(win->flags & FLAG_SHOWN)) {
      printf("%s: Window is not visible.\n", __func__);
    }
#ifndef WIN32
    if (GrabSuccess != XGrabPointer(app->display, win->xid, FALSE, GRAB_EVENT_MASK, GrabModeAsync, GrabModeAsync, None, win->dragCursor->xid, app->event.time)) {
      XGrabPointer(app->display, win->xid, FALSE, GRAB_EVENT_MASK, GrabModeAsync, GrabModeAsync, None, win->dragCursor->xid, CurrentTime);
    }
#else
    SetCapture((HWND)win->xid);
    if (GetCapture() != (HWND)win->xid) {
      SetCapture((HWND)win->xid);
    }
    SetCursor((HCURSOR)win->dragCursor->xid);
#endif
    app->mouseGrabWindow = win;
  }
}

/* Release grab; also switches back to the normal cursor */
void dkWindowUngrab(struct dkWindow *win)
{
  struct dkApp *app = win->app;

  if (win->xid) {
    app->mouseGrabWindow = NULL;
#ifndef WIN32
    XUngrabPointer(app->display, app->event.time);
    XFlush(app->display);
#else
    ReleaseCapture();
    SetCursor((HCURSOR)win->defaultCursor->xid);
#endif
  }
}

/* Find common ancestor between window a and b */
struct dkWindow * dkWindowCommonAncestor(struct dkWindow *a, struct dkWindow *b)
{
  struct dkWindow *p1, *p2;

  if (a || b) {
    if (!a) return dkWindowGetRoot(b);
    if (!b) return dkWindowGetRoot(a);
    p1 = a;
    while (p1) {
      p2 = b;
      while (p2) {
        if (p2 == p1) return p1;
        p2 = p2->parent;
      }
      p1 = p1->parent;
    }
  }
  return NULL;
}

/* Update this widget by sending SEL_UPDATE to its target.
 * If there is no target, onUpdate returns 0 on behalf of widgets
 * which have the autogray feature enabled.  If there is a target
 * but we're not updating because the user is manipulating the
 * widget, then onUpdate returns 1 to prevent it from graying
 * out during manipulation when the autogray feature is enabled.
 * Otherwise, onUpdate returns the value returned by the SEL_UPDATE
 * callback to the target. */
long dkWindow_onUpdate(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  return w->target && (!(w->flags & FLAG_UPDATE) || w->target->handle(w->target, pthis, SEL_UPDATE, w->message, NULL));
}

/* Window was mapped to screen */
static long dkWindow_onMap(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;

  DKTRACE((250, "%s::onMap %p\n", ((struct dkObject *)w)->meta->className, pthis));
  return w->target && w->target->handle(w->target, pthis, SEL_MAP, w->message, ptr);
}

static long dkWindow_onMotion(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  return dkWindowIsEnabled(w) && w->target && w->target->handle(w->target, pthis, SEL_MOTION, w->message, ptr);
}

/* Handle configure notify */
static long dkWindow_onConfigure(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  DKTRACE((250, "%s::onConfigure %p\n", ((struct dkObject *)w)->meta->className, pthis));
  return w->target && w->target->handle(w->target, pthis, SEL_CONFIGURE, w->message, ptr);
}

/* Left button pressed */
static long dkWindow_onLeftBtnPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;

  w->flags &= ~FLAG_TIP;
  ((struct dkObject *)w)->handle(w, pthis, SEL_FOCUS_SELF, 0, ptr);
  if (dkWindowIsEnabled(w)) {
    dkWindowGrab(w);
    if (w->target && w->target->handle(w->target, pthis, SEL_LEFTBUTTONPRESS, w->message, ptr))
      return 1;
  }
  return 0;
}

/* Left button released */
static long dkWindow_onLeftBtnRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;

  if (dkWindowIsEnabled(w)) {
    dkWindowUngrab(w);
    if (w->target && w->target->handle(w->target, pthis, SEL_LEFTBUTTONRELEASE, w->message, ptr))
      return 1;
  }
  return 0;
}

long dkWindow_onEnter(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  struct dkEvent *event = (struct dkEvent *)ptr;

  //FXTRACE((150,"%s::onEnter %p (%s)\n",getClassName(),this, (event->code==CROSSINGNORMAL) ? "CROSSINGNORMAL" : (event->code==CROSSINGGRAB) ? "CROSSINGGRAB" : (event->code==CROSSINGUNGRAB)? "CROSSINGUNGRAB" : "?"));
  if (event->code != CROSSINGGRAB) {
    if (!(event->state & (SHIFTMASK | CONTROLMASK | METAMASK | LEFTBUTTONMASK | MIDDLEBUTTONMASK | RIGHTBUTTONMASK))) w->flags |= FLAG_TIP;
    w->flags|=FLAG_HELP;
  }
  if (dkWindowIsEnabled(w) && w->target) { w->target->handle(w->target, pthis, SEL_ENTER, w->message, ptr); }
  return 1;
}

long dkWindow_onLeave(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  struct dkEvent *event = (struct dkEvent *)ptr;

  //FXTRACE((150,"%s::onLeave %p (%s)\n",getClassName(),this, (event->code==CROSSINGNORMAL) ? "CROSSINGNORMAL" : (event->code==CROSSINGGRAB) ? "CROSSINGGRAB" : (event->code==CROSSINGUNGRAB)? "CROSSINGUNGRAB" : "?"));
  if (event->code != CROSSINGUNGRAB) {
    w->flags &= ~(FLAG_TIP | FLAG_HELP);
  }
  if (dkWindowIsEnabled(w) && w->target) { w->target->handle(w->target, pthis, SEL_LEAVE, w->message, ptr); }
  return 1;
}

/* Gained focus */
long dkWindow_onFocusIn(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  w->flags |= FLAG_FOCUSED;
  if (w->target) { w->target->handle(w->target, pthis, SEL_FOCUSIN, w->message, data); }
  if (w->composeContext) { /*composeContext->focusIn(); */  printf("XXX: Handle composeContext focusIn\n"); }
  if (w->focus) { ((struct dkObject *)w->focus)->handle(w->focus, (struct dkObject *)w->focus, SEL_FOCUSIN, 0, NULL); }
  return 1;
}

/* Lost focus */
long dkWindow_onFocusOut(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  if (w->focus) { ((struct dkObject *)w->focus)->handle(w->focus, (struct dkObject *)w->focus, SEL_FOCUSOUT, 0, NULL); }
  if (w->composeContext) { /*composeContext->focusOut(); */  printf("XXX: Handle composeContext focusOut\n"); }
  if (w->target) { w->target->handle(w->target, pthis, SEL_FOCUSOUT, w->message, data); }
  w->flags &= ~FLAG_FOCUSED;
  return 1;
}

/* Focus on widget itself, if its enabled */
long dkWindow_onFocusSelf(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;

  DKTRACE((150,"%s::onFocusSelf %p\n", ((struct dkObject *)w)->meta->className, pthis));
  if (dkWindowIsEnabled(w) && w->canFocus()) { w->setFocus(w); return 1; }
  return 0;
}

/* Set focus to this widget.
 * The chain of focus from shell down to a control is changed.
 * Widgets now in the chain may or may not gain real focus,
 * depending on whether parent window already had a real focus!
 * Setting the focus to a composite will cause descendants to loose it. */
void dkWindow_setFocus(struct dkWindow *pthis)
{
  DKTRACE((140,"%s::setFocus %p\n", ((struct dkObject *)pthis)->meta->className, pthis));
  if (pthis->parent && pthis->parent->focus != pthis) {
    if (pthis->parent->focus) pthis->parent->focus->killFocus(pthis->parent->focus); else pthis->parent->setFocus(pthis->parent);
    pthis->parent->changeFocus(pthis->parent, pthis);
    if (dkWindowHasFocus(pthis->parent)) ((struct dkObject *)pthis)->handle(pthis, (struct dkObject *)pthis, SEL_FOCUSIN, 0, NULL);
  }
  pthis->flags |= FLAG_HELP;
}

/* Kill focus to this widget. */
void dkWindow_killFocus(struct dkWindow *pthis)
{
  DKTRACE((140, "%s::killFocus %p\n", ((struct dkObject *)pthis)->meta->className, pthis));
  if (pthis->parent && pthis->parent->focus == pthis) {
    if (pthis->focus) pthis->focus->killFocus(pthis->focus);
    if (dkWindowHasFocus(pthis)) ((struct dkObject *)pthis)->handle(pthis, (struct dkObject *)pthis, SEL_FOCUSOUT, 0, NULL);
    pthis->parent->changeFocus(pthis->parent, NULL);
  }
  pthis->flags &= ~FLAG_HELP;
}

/* Notification that focus moved to new child */
void dkWindow_changeFocus(struct dkWindow *pthis, struct dkWindow *child)
{
  DKTRACE((140,"%s::changeFocus: from %p to %p\n", ((struct dkObject *)pthis)->meta->className, pthis->focus, child));
  pthis->focus = child;
}

static int dkWindowGetDefaultWidth(struct dkWindow *w)
{
	return 1;
}

static int dkWindowGetDefaultHeight(struct dkWindow *w)
{
	return 1;
}

/* Return width for given height */
static int dkWindowGetWidthForHeight(struct dkWindow *w, int height)
{
  return w->getDefaultWidth(w);
}

/* Return height for given width */
static int dkWindowGetHeightForWidth(struct dkWindow *w, int weight)
{
  return w->getDefaultHeight(w);
}

/* Get pointer locations (in windows coordinates) */
int dkWindowGetCursorPosition(struct dkWindow *w, int *x, int *y, DKuint *buttons)
{
  if (w->xid) {
#ifndef WIN32
    Window dum;
    int rx, ry;
    return XQueryPointer(w->app->display, w->xid, &dum, &dum, &rx, &ry, x, y, buttons);
#else
    POINT pt;
    GetCursorPos(&pt);
    ScreenToClient((HWND)w->xid, &pt);
    *x = pt.x; *y = pt.y;
    *buttons = dkmodifierkeys();
    return TRUE;
#endif
  }
  return FALSE;
}

#ifdef WIN32
void
DtkDrvCreateWindow(struct dkWindow *w)
{
	DWORD dwStyle;
	DWORD dwExStyle;
	HWND hParent;

	/* Gotta have a parent already created! */
	if (!w->parent->xid) {
		printf("Error: %s: trying to create window before creating parent window.\n", __func__);
	}

	/* If window has owner, owner should have been created already */
	if (w->owner && !w->owner->xid) {
		printf("Error: %s trying to create window before creating owner window.\n", __func__);
	}

	/* Got to have a visual */
	if (!w->visual) {
		printf("Error: %s trying to create window without a visual.\n", __func__);
	}

  /* Initialize visual */
  DtkCreateVisual(w->visual);

  /* Create default cursor */
  if (w->defaultCursor) FxCursorCreate(w->defaultCursor);

  /* Create drag cursor */
  if (w->dragCursor) FxCursorCreate(w->dragCursor);

	/* Most windows use these style bits */
	dwStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
	dwExStyle = 0;
	dwExStyle |= WS_EX_NOPARENTNOTIFY;
	hParent = (HWND)w->parent->xid;
	if (w->flags & FLAG_SHELL) {
		if (strcmp(w->classname, "DTKTopWindow") == 0) {
			dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		} else if (0) {
			/* Never run */
		} else {
			/* Other top-level shell windows (like dialogs) */
			dwStyle = WS_POPUP | WS_CAPTION | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
			dwExStyle = WS_EX_DLGMODALFRAME | WS_EX_TOOLWINDOW;
		}
		if (w->owner)
			hParent = (HWND)w->owner->xid;
	}

	/* Create this window */
	w->xid = CreateWindowExA(dwExStyle, w->classname, NULL, dwStyle,
			w->xpos, w->ypos, DTKMAX(w->width, 1),
			DTKMAX(w->height, 1), hParent, NULL,
			(HINSTANCE)w->app->display, w);

	/* Uh-oh, we failed */
	if(!w->xid) {
		printf("unable to create window.\n");
	}

	/* Store the xid to object mapping */
	DtkWindowHashInsert((void *)w->xid, w);

	/* We put the XdndAware property on all toplevel windows, so that
	 * when dragging, we need to search no further than the toplevel
	 * window. */
	if (w->flags & FLAG_SHELL) {
		//HANDLE propdata = (HANDLE)
	}
}

#else

// Basic events
#define BASIC_EVENT_MASK (StructureNotifyMask|ExposureMask|PropertyChangeMask|EnterWindowMask|LeaveWindowMask|KeyPressMask|KeyReleaseMask|KeymapStateMask)

// Additional events for shell widget events
#define SHELL_EVENT_MASK   (FocusChangeMask|StructureNotifyMask)

// Additional events for enabled widgets
#define ENABLED_EVENT_MASK (ButtonPressMask|ButtonReleaseMask|PointerMotionMask)

// These events are grabbed for mouse grabs
#define GRAB_EVENT_MASK  (ButtonPressMask|ButtonReleaseMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask)

// Do not propagate mask
#define NOT_PROPAGATE_MASK  (KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ButtonMotionMask)

void
DtkDrvCreateWindow(struct dkWindow *w)
{
	XSetWindowAttributes wattr;
	XClassHint hint;
	unsigned long mask;
	Display *disp;

	/* Gotta have a parent already created! */
	if (!w->parent->xid) {
		printf("Error: %s: trying to create window before creating parent window.\n", __func__);
	}

	/* If window has owner, owner should have been created already */
	if (w->owner && !w->owner->xid) {
		printf("Error: %s trying to create window before creating owner window.\n", __func__);
	}

	/* Got to have a visual */
	if (!w->visual) {
		printf("Error: %s trying to create window without a visual.\n", __func__);
	}

	/* Initialize visual */
	DtkCreateVisual(w->visual);

  /* Create default cursor */
  if (w->defaultCursor) FxCursorCreate(w->defaultCursor);

  /* Create drag cursor */
  if(w->dragCursor) FxCursorCreate(w->dragCursor);

	/* Fill in the attributes */
	mask = CWBackPixmap | CWWinGravity | CWBitGravity | CWBorderPixel | CWEventMask |
	    CWDontPropagate | CWCursor | CWOverrideRedirect | CWSaveUnder | CWColormap;

	/* Events for normal windows */
	wattr.event_mask = BASIC_EVENT_MASK;

	/* Events for shell windows */
	if (w->flags & FLAG_SHELL) wattr.event_mask |= SHELL_EVENT_MASK;

	/* If enabled, turn on some more events */
	if (w->flags & FLAG_ENABLED) wattr.event_mask |= ENABLED_EVENT_MASK;

	/* FOX will not propagate events to ancestor windows */
	wattr.do_not_propagate_mask = NOT_PROPAGATE_MASK;

	/* Obtain colormap */
	wattr.colormap = w->visual->colormap;

	/* This is needed for OpenGL */
	wattr.border_pixel = 0;

	/* Background */
	wattr.background_pixmap = None;
	//wattr.background_pixel=visual->getPixel(backColor);

	/* Preserving content during resize will be faster:- not turned
	 * on yet as we will have to recode all widgets to decide when to
	 * repaint or not to repaint the display when resized... */
	wattr.bit_gravity = NorthWestGravity;
	wattr.bit_gravity = ForgetGravity;

	/* The window gravity is NorthWestGravity, which means
	 * if a child keeps same position relative to top/left
	 * of its parent window, nothing extra work is
	 * incurred. */
	wattr.win_gravity = NorthWestGravity;

	/* Determine override redirect */
	wattr.override_redirect = 0;

	/* Determine save-unders */
	wattr.save_under = 0;

	/* Set cursor */
	wattr.cursor = w->defaultCursor->xid;

	disp = w->app->display;

	/* Finally, create the window */
	w->xid = XCreateWindow(disp,
	    w->parent->xid, w->xpos, w->ypos,
	    FXMAX(w->width, 1),
	    FXMAX(w->height, 1), 0,
	    w->visual->depth,
	    InputOutput, (Visual*)w->visual->visual,
	    mask, &wattr);

	/* Uh-oh, we failed */
	if(!w->xid){ printf("unable to create window.\n"); }

	/* Store the xid to object mapping */
	DtkWindowHashInsert((void *)w->xid, w);

	/* Set resource and class name for toplevel windows.
	 * In a perfect world this would be set in FXTopWindow, but for some strange reasons
	 * some window-managers (e.g. fvwm) this will be too late and they will not recognize
	 * them. */
	if (w->flags & FLAG_SHELL) {
		hint.res_name = "DTKApp"; //(char*)getApp()->getAppName().text();
		hint.res_class = "DtkWindow"; //(char*)getApp()->getVendorName().text();
		XSetClassHint(disp, w->xid, &hint);
	}

	/* We put the XdndAware property on all toplevel windows, so that
	 * when dragging, we need to search no further than the toplevel window. */
	if (w->flags & FLAG_SHELL) {
		Atom propdata = (Atom)XDND_PROTOCOL_VERSION;
		XChangeProperty(disp, w->xid, w->app->xdndAware, XA_ATOM, 32, PropModeReplace, (unsigned char*)&propdata, 1);
	}

	/* If window is a shell and it has an owner, make it stay on top of the owner */
	if ((w->flags & FLAG_SHELL) && w->owner) {
		XSetTransientForHint(disp, w->xid, w->owner->xid);
	}

	/* If colormap different, set WM_COLORMAP_WINDOWS property properly */
	if (w->visual->colormap != DefaultColormap(disp, DefaultScreen(disp))) {
		printf("%s::create: TODO! adding to WM_COLORMAP_WINDOWS\n", __func__);
		//addColormapWindows();
	}

	/* Show if it was supposed to be */
	if ((w->flags & FLAG_SHOWN) && 0 < w->width && 0 < w->height)
		XMapWindow(disp, w->xid);
}

void
DtkDrvWindowShow(struct dkWindow *w)
{
  if (w->width > 0 && w->height > 0)
    XMapWindow(w->app->display, w->xid);
}
#endif

int dkWindowIsOwnerOf(struct dkWindow *pthis, struct dkWindow *window)
{
  while (window) {
    if (window == pthis) return 1;
    window = window->owner;
  }
  return 0;
}

int dkWindow_releaseSelection(struct dkWindow *pthis)
{
  struct dkApp *app;

  app = pthis->app;
  if (app->selectionWindow == pthis) {
    ((struct dkObject *)app->selectionWindow)->handle(app->selectionWindow, (struct dkObject *)app, SEL_SELECTION_LOST, 0, &app->event);
    app->selectionWindow = NULL;
    free(app->xselTypeList);
    app->xselTypeList = NULL;
    app->xselNumTypes = 0;
    if (pthis->xid) {
#ifndef WIN32
      XSetSelectionOwner(app->display, XA_PRIMARY, None, app->event.time);
#endif
    }
    return 1;
  }
  return 0;
}

/* Keyboard press */
static long dkWindow_onKeyPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  struct dkEvent *ev = (struct dkEvent *)ptr;
  DKTRACE((200, "%s::onKeyPress %p keysym=0x%04x state=%04x\n", ((struct dkObject *)w)->meta->className, ev->code, ev->state));
  return dkWindowIsEnabled(w) && w->target && w->target->handle(w->target, pthis, SEL_KEYPRESS, w->message, ptr);
}

/* Keyboard release */
static long dkWindow_onKeyRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  struct dkEvent *ev = (struct dkEvent *)ptr;
  DKTRACE((200, "%s::onKeyRelease %p keysym=0x%04x state=%04x\n", ((struct dkObject *)w)->meta->className, ev->code, ev->state));
  return dkWindowIsEnabled(w) && w->target && w->target->handle(w->target, pthis, SEL_KEYRELEASE, w->message, ptr);
}

long dkWindow_onSelectionLost(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  DKTRACE((100, "%s::onSelectionLost %p\n", ((struct dkObject *)w)->meta->className, pthis));
  return w->target && w->target->handle(w->target, pthis, SEL_SELECTION_LOST, w->message, ptr);
}

long dkWindow_onSelectionGained(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *w = (struct dkWindow *)pthis;
  DKTRACE((100, "%s::onSelectionGained %p\n", ((struct dkObject *)w)->meta->className, pthis));
  return w->target && w->target->handle(w->target, pthis, SEL_SELECTION_GAINED, w->message, ptr);
}

long dkWindowCallWindowProc(dkHandleProc proc, struct dkWindow *win, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  return proc(win, obj, selhi, sello, data);
}

dkHandleProc dkWindowGetHandleProc(struct dkWindow *win)
{
  return ((struct dkObject *)win)->handle;
}

void dkWindowSetHandleProc(struct dkWindow *win, dkHandleProc proc)
{
  ((struct dkObject *)win)->handle = proc;
}

void dkWindowSetActionCallback(struct dkWindow *win, dkActionCallback cb, void *udata)
{
  win->actionCb = cb;
  win->cbextra = udata;
}
