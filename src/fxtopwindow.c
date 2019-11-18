/******************************************************************************
 *                                                                            *
 *              T o p - L e v e l   W i n d o w   W i d g e t                 *
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
 * $Id: FXTopWindow.cpp,v 1.175.2.8 2008/05/08 02:00:07 fox Exp $             *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#endif

#include "fxapp.h"
#include "fxpriv.h"
#include "fxdc.h"
#include "fxdrv.h"

#include "fxtopwindow.h"

/*
  Notes:
  - Handle zero width/height case similar to FXWindow.
  - Pass Size Hints to Window Manager as per ICCCM.
  - Add padding options, as this is convenient for FXDialogBox subclasses;
    for FXTopWindow/FXMainWindow, padding should default to 0, for FXDialogBox,
    default to something easthetically pleasing...
  - Now observes LAYOUT_FIX_X and LAYOUT_FIX_Y hints.
  - LAYOUT_FIX_WIDTH and LAYOUT_FIX_HEIGHT take precedence over PACK_UNIFORM_WIDTH and
    PACK_UNIFORM_HEIGHT!
*/

// Definitions for Motif-style WM Hints.
#ifndef WIN32
#define MWM_HINTS_FUNCTIONS	(1L << 0)       // Definitions for FXMotifHints.flags
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_HINTS_INPUT_MODE	(1L << 2)
#define MWM_HINTS_ALL           (MWM_HINTS_FUNCTIONS|MWM_HINTS_DECORATIONS|MWM_HINTS_INPUT_MODE)

#define MWM_FUNC_ALL		(1L << 0)       // Definitions for FXMotifHints.functions
#define MWM_FUNC_RESIZE		(1L << 1)
#define MWM_FUNC_MOVE		(1L << 2)
#define MWM_FUNC_MINIMIZE	(1L << 3)
#define MWM_FUNC_MAXIMIZE	(1L << 4)
#define MWM_FUNC_CLOSE		(1L << 5)

#define MWM_DECOR_ALL		(1L << 0)       // Definitions for FXMotifHints.decorations
#define MWM_DECOR_BORDER	(1L << 1)
#define MWM_DECOR_RESIZEH	(1L << 2)
#define MWM_DECOR_TITLE		(1L << 3)
#define MWM_DECOR_MENU		(1L << 4)
#define MWM_DECOR_MINIMIZE	(1L << 5)
#define MWM_DECOR_MAXIMIZE	(1L << 6)

#define MWM_INPUT_MODELESS		    0   // Values for FXMotifHints.inputmode
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL 1
#define MWM_INPUT_SYSTEM_MODAL		    2
#define MWM_INPUT_FULL_APPLICATION_MODAL    3
#endif

static void DtkTopWindowShow(struct dkWindow *w);
static void DtkTopWindowSetTitle(struct dkTopWindow *w);
static void dkTopWindowSetDecorations(struct dkTopWindow *w);
static void dkTopWindowPosition(struct dkWindow *win, int x, int y, int w, int h);
int dkTopWindowGetDefaultWidth(struct dkWindow *win);
int dkTopWindowGetDefaultHeight(struct dkWindow *win);

/* Map */
static struct dkMapEntry dkTopWindowMapEntry[] = {
};

/* Object implementation */
static struct dkMetaClass dkTopWindowMetaClass = {
  "dkTopWindow", dkTopWindowMapEntry, sizeof(dkTopWindowMapEntry) / sizeof(dkTopWindowMapEntry[0]), sizeof(struct dkMapEntry)
};

void dkTopWindowPlace(struct dkTopWindow *w, DKuint placement);

void
DtkTopWindowCtor(struct dkTopWindow *pthis, struct dkApp *app, char *title, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs)
{
	/* Pass to base class constructor */
	DtkTopWindowShellCtor((struct dkShell *)pthis, app, opts, x, y, w, h);
  ((struct dkObject *)pthis)->meta = &dkTopWindowMetaClass;

  /* Setup the vtbl */
  ((struct dkWindow *)pthis)->create = DtkCreateTopWindow;
  ((struct dkWindow *)pthis)->show = DtkTopWindowShow;
  ((struct dkWindow *)pthis)->layout = dkTopWindowLayout;
  ((struct dkWindow *)pthis)->position = dkTopWindowPosition;
  ((struct dkWindow *)pthis)->getDefaultWidth = dkTopWindowGetDefaultWidth;
  ((struct dkWindow *)pthis)->getDefaultHeight = dkTopWindowGetDefaultHeight;

  /* Store fields/members */
  ((struct dkWindow *)pthis)->classname = "DTKTopWindow";
	pthis->title = title;
	pthis->padtop = pt;
	pthis->padbottom = pb;
	pthis->padleft = pl;
	pthis->padright = pr;
	pthis->hspacing = hs;
	pthis->vspacing = vs;
}

void
DtkCreateTopWindow(void *pthis)
{
	struct dkTopWindow *w = (struct dkTopWindow *)pthis;
	DtkCreateShellWindow((struct dkShell *)w);

	/* Create icons */
	if (((struct dkWindow *)w)->xid) {
		if (((struct dkWindow *)w)->app->display_opened) {
			/* Set Title */
			DtkTopWindowSetTitle(w);

			/* Set decorations */
			dkTopWindowSetDecorations(w);
		}
	}
}

/* Request for toplevel window reposition */
static void dkTopWindowPosition(struct dkWindow *win, int x, int y, int w, int h)
{
  struct dkWindow *tw = win;

  if ((tw->flags & FLAG_DIRTY) || (x != tw->xpos) || (y != tw->ypos) ||
	    (w != tw->width) || (h != tw->height)) {
    tw->xpos = x;
    tw->ypos = y;
    tw->width = FXMAX(w, 1);
    tw->height = FXMAX(h, 1);
    if (tw->xid) {
#ifdef WIN32
      RECT rect;
      DWORD dwStyle;
      DWORD dwExStyle;

      SetRect(&rect, tw->xpos, tw->ypos, tw->xpos + tw->width, tw->ypos + tw->height);
      dwStyle = GetWindowLong((HWND)tw->xid, GWL_STYLE);
      dwExStyle = GetWindowLong((HWND)tw->xid, GWL_EXSTYLE);
      AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);  /* Calculate based on *client* rectange */
      SetWindowPos((HWND)tw->xid, NULL, rect.left, rect.top, FXMAX(rect.right - rect.left, 1), FXMAX(rect.bottom - rect.top, 1), SWP_NOZORDER | SWP_NOOWNERZORDER);
#else
      XWindowChanges changes;
      XSizeHints size;
      size.flags = USSize | PSize | PWinGravity | USPosition | PPosition;
      size.x = tw->xpos;
      size.y = tw->ypos;
      size.width = tw->width;
      size.height = tw->height;
      size.min_width = 0;
      size.min_height = 0;
      size.max_width = 0;
      size.max_height = 0;
      size.width_inc = 0;
      size.height_inc = 0;
      size.min_aspect.x = 0;
      size.min_aspect.y = 0;
      size.max_aspect.x = 0;
      size.max_aspect.y = 0;
      size.base_width = 0;
      size.base_height = 0;
      size.win_gravity = NorthWestGravity;                        // Tim Alexeevsky <realtim@mail.ru>
      size.win_gravity = StaticGravity;                           // Account for border (ICCCM)
      if (!(tw->options & DECOR_SHRINKABLE)) {
        if (!(tw->options & DECOR_STRETCHABLE)) {                       // Cannot change at all
          size.flags |= PMinSize | PMaxSize;
          size.min_width = size.max_width = tw->width;
          size.min_height = size.max_height = tw->height;
        } else {                                                   // Cannot get smaller than default
          size.flags |= PMinSize;
          size.min_width = tw->getDefaultWidth((struct dkWindow *)tw);
          size.min_height = tw->getDefaultHeight((struct dkWindow *)tw);
        }
      } else if (!(tw->options & DECOR_STRETCHABLE)) {                    // Cannot get larger than default
        size.flags |= PMaxSize;
        size.max_width = tw->getDefaultWidth((struct dkWindow *)tw);
        size.max_height = tw->getDefaultHeight((struct dkWindow *)tw);
      }
      XSetWMNormalHints(tw->app->display, tw->xid, &size);
      changes.x = tw->xpos;
      changes.y = tw->ypos;
      changes.width = tw->width;
      changes.height = tw->height;
      changes.border_width = 0;
      changes.sibling = None;
      changes.stack_mode = Above;
      XReconfigureWMWindow(tw->app->display, tw->xid,
			    DefaultScreen(tw->app->display), CWX | CWY | CWWidth | CWHeight,
					&changes);
#endif
      tw->layout(tw);
    }
  }
}

/* Compute minimum height based on child layout hints */
int dkTopWindowGetDefaultHeight(struct dkWindow *win)
{
  int h, hcum, hmax, mh;
  struct dkWindow *child;
  struct dkWindow *tw = win;
  DKuint hints;

  hmax = hcum = mh = 0;
  if (tw->options & PACK_UNIFORM_HEIGHT)
    mh = dkCompositeMaxChildHeight(tw);
  for (child = tw->last; child; child = child->prev) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
      else if (tw->options & PACK_UNIFORM_HEIGHT) h = mh;
      else h = child->getDefaultHeight(child);
      if ((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y)) {   // Fixed Y
        h = child->ypos + h;
        if (h > hmax) hmax = h;
      } else if (!(hints & LAYOUT_SIDE_LEFT)) {     /* Top or bottom */
        if (child->next) hcum += ((struct dkTopWindow *)tw)->vspacing;
        hcum += h;
      } else {
        if (h > hcum) hcum = h;
      }
    }
  }
  hcum += ((struct dkTopWindow *)tw)->padtop + ((struct dkTopWindow *)tw)->padbottom;
  return FXMAX(hcum, hmax);
}

/* Compute minimum width based on child layout hints */
int dkTopWindowGetDefaultWidth(struct dkWindow *win)
{
  int w, wcum, wmax, mw;
  struct dkWindow *child;
  struct dkWindow *tw = win;
  DKuint hints;

  wmax = wcum = mw = 0;

  if (tw->options & PACK_UNIFORM_WIDTH)
    mw = dkCompositeMaxChildWidth(tw);

  for (child = tw->last; child; child = child->prev) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      if (hints & LAYOUT_FIX_WIDTH) w = child->width;
      else if (tw->options & PACK_UNIFORM_WIDTH) w = mw;
      else w = child->getDefaultWidth(child);
      if ((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X)) {    // Fixed X
        w = child->xpos + w;
        if (w > wmax) wmax = w;
      } else if (hints & LAYOUT_SIDE_LEFT) {                // Left or right
        if (child->next) wcum += ((struct dkTopWindow *)tw)->hspacing;
        wcum += w;
      } else {
        if (w > wcum) wcum = w;
      }
    }
  }
  wcum += ((struct dkTopWindow *)tw)->padleft + ((struct dkTopWindow *)tw)->padright;
  return FXMAX(wcum, wmax);
}

static void dkTopWindowSetDecorations(struct dkTopWindow *w)
{
#ifdef WIN32
	/* Get old style */
	DWORD dwStyle = GetWindowLong((HWND)((struct dkWindow *)w)->xid, GWL_STYLE);
	RECT rect;

	/* Moved here just in case the size changes behind our backs */
	SetRect(&rect, 0, 0, ((struct dkWindow *)w)->width,
	    ((struct dkWindow *)w)->height);

	/* Change style setting; note, under Windows, if we want a minimize,
	 * maximize, or close button, we als need a window menu style as well.
	 * Also, if you wan a title, you will need a border. */
	if (((struct dkWindow *)w)->options & DECOR_BORDER) dwStyle |= WS_BORDER; else dwStyle &= ~WS_BORDER;
	if (((struct dkWindow *)w)->options & DECOR_TITLE) dwStyle |= WS_CAPTION; else dwStyle &= ~WS_DLGFRAME;
	if (((struct dkWindow *)w)->options & DECOR_RESIZE) dwStyle |= WS_THICKFRAME; else dwStyle &= ~WS_THICKFRAME;
	if (((struct dkWindow *)w)->options & DECOR_MENU) dwStyle |= WS_SYSMENU; else dwStyle &= ~WS_SYSMENU;
	if (((struct dkWindow *)w)->options & DECOR_CLOSE) dwStyle |= WS_SYSMENU;
	if (((struct dkWindow *)w)->options & DECOR_MINIMIZE) dwStyle |= (WS_MINIMIZEBOX | WS_SYSMENU); else dwStyle &= ~WS_MINIMIZEBOX;
	if (((struct dkWindow *)w)->options & DECOR_MAXIMIZE) dwStyle |= (WS_MAXIMIZEBOX | WS_SYSMENU); else dwStyle &= ~WS_MAXIMIZEBOX;

	/* Set new style */
	SetWindowLong((HWND)((struct dkWindow *)w)->xid, GWL_STYLE, dwStyle);

	/* Patch from Stephane Ancelot <sancelot@wanadoo.fr> and Sander Jansen <sander@knology.net> */
	HMENU sysmenu = GetSystemMenu((HWND)((struct dkWindow *)w)->xid, FALSE);
	if (sysmenu) {
		if (((struct dkWindow *)w)->options & DECOR_CLOSE)
			EnableMenuItem(sysmenu, SC_CLOSE, MF_ENABLED);
		else
			EnableMenuItem(sysmenu, SC_CLOSE, MF_GRAYED);
	}

	/* Moved here just in case SetWindowLong GWL_STYLE has changed
	 * the GWL_EXSTYLE behind the scenes... */
	DWORD dwExStyle = GetWindowLong((HWND)((struct dkWindow *)w)->xid, GWL_EXSTYLE);

	/* Adjust non-client area size based on new style */
	AdjustWindowRectEx(&rect, dwStyle, FALSE, dwExStyle);
	SetWindowPos((HWND)((struct dkWindow *)w)->xid, NULL, 0, 0, FXMAX(rect.right - rect.left, 1), FXMAX(rect.bottom - rect.top, 1), SWP_NOMOVE | SWP_NOZORDER | SWP_NOOWNERZORDER);
	RedrawWindow((HWND)((struct dkWindow *)w)->xid, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
#else
  struct {
    long flags;
    long functions;
    long decorations;
    long inputmode;
  } prop;

  prop.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS | MWM_HINTS_INPUT_MODE;
  prop.decorations = 0;
  prop.functions = MWM_FUNC_MOVE;
  prop.inputmode = MWM_INPUT_MODELESS;
  if (((struct dkWindow *)w)->options & DECOR_TITLE) {
    prop.decorations |= MWM_DECOR_TITLE;
  }
  if (((struct dkWindow *)w)->options & DECOR_MINIMIZE) {
    prop.decorations |= MWM_DECOR_MINIMIZE;
    prop.functions |= MWM_FUNC_MINIMIZE;
  }
  if (((struct dkWindow *)w)->options & DECOR_MAXIMIZE) {
    prop.decorations |= MWM_DECOR_MAXIMIZE;
    prop.functions |= MWM_FUNC_MAXIMIZE;
  }
  if (((struct dkWindow *)w)->options & DECOR_CLOSE) {
    prop.functions |= MWM_FUNC_CLOSE;
  }
  if (((struct dkWindow *)w)->options & DECOR_BORDER) {
    prop.decorations |= MWM_DECOR_BORDER;
  }
  if (((struct dkWindow *)w)->options & (DECOR_SHRINKABLE | DECOR_STRETCHABLE)) {
    if (((struct dkWindow *)w)->options & DECOR_BORDER) prop.decorations |= MWM_DECOR_RESIZEH;       /* Only grips if border */
    prop.functions |= MWM_FUNC_RESIZE;
  }
  if (((struct dkWindow *)w)->options & DECOR_MENU) {
    prop.decorations |= MWM_DECOR_MENU;
    prop.functions |= MWM_FUNC_RESIZE;
  }
  XChangeProperty(((struct dkWindow *)w)->app->display, ((struct dkWindow *)w)->xid, ((struct dkWindow *)w)->app->wmMotifHints, ((struct dkWindow *)w)->app->wmMotifHints, 32, PropModeReplace, (unsigned char*)&prop, 4);
#endif
}

static void
DtkTopWindowSetTitle(struct dkTopWindow *w)
{
	if (w->title != NULL) {
#ifdef WIN32
#ifdef UNICODE
#else
		SetWindowTextA((HWND)((struct dkWindow *)w)->xid, w->title);
#endif
#else
		XTextProperty t;
		if(XStringListToTextProperty((char**)&w->title, 1, &t)){
			XSetWMIconName((Display *)((struct dkWindow *)w)->app->display,
			    ((struct dkWindow *)w)->xid, &t);
			XSetWMName((Display *)((struct dkWindow *)w)->app->display,
			    ((struct dkWindow *)w)->xid, &t);
			XFree(t.value);
		}

	/* Extended window manager hint for true unicode name in title */
//	XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmNetIconName,utf8Type,8,PropModeReplace,(unsigned char*)title.text(),title.length());
//	XChangeProperty(DISPLAY(getApp()),xid,getApp()->wmNetWindowName,utf8Type,8,PropModeReplace,(unsigned char*)title.text(),title.length());
#endif
	}
}

static void
DtkTopWindowShow(struct dkWindow *w)
{
  DtkShellWindowShow(w);
  DtkTopWindowRaise(w);
}

/* Recalculate layout */
void
dkTopWindowLayout(struct dkWindow *win)
{
	struct dkTopWindow *tw = (struct dkTopWindow *)win;
	int left,right,top,bottom,x,y,w,h;
	int mw = 0, mh = 0;
	struct dkWindow *child;
	DKuint hints;

	/* Placement rectangle; right/bottom non-inclusive */
	left = tw->padleft;
	right = ((struct dkWindow *)tw)->width - tw->padright;
	top = tw->padtop;
	bottom = ((struct dkWindow *)tw)->height - tw->padbottom;

	/* Get maximum child size */
	if (win->options & PACK_UNIFORM_WIDTH) mw = dkCompositeMaxChildWidth(win);
	if (win->options & PACK_UNIFORM_HEIGHT) mh = dkCompositeMaxChildHeight(win);

	/* Pack them in the cavity */
	for (child = ((struct dkWindow *)tw)->first; child; child = child->next) {
		if (dkWindowIsShown(child)) {
			hints = dkWindowGetLayoutHints(child);
			x = child->xpos;
			y = child->ypos;

			/* Vertical */
			if (hints & LAYOUT_SIDE_LEFT) {

        /* Height */
        if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
        else if (((struct dkWindow *)tw)->options & PACK_UNIFORM_HEIGHT) h = mh;
        else if (hints & LAYOUT_FILL_Y) h = bottom - top;
        else h = child->getDefaultHeight(child);

				/* Width */
				if (hints & LAYOUT_FIX_WIDTH) w = child->width;
				else if (((struct dkWindow *)tw)->options & PACK_UNIFORM_WIDTH) w = mw;
				else if (hints & LAYOUT_FILL_X) w = right - left;
				else w = child->getWidthForHeight(child, h);   /* Width is a function of height! */

				/* Y */
				if (!((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y))) {
					if (hints & LAYOUT_CENTER_Y) y = top + (bottom - top - h) / 2;
					else if (hints & LAYOUT_BOTTOM) y = bottom - h;
					else y = top;
				}

				/* X */
				if (!((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X))) {
					if (hints & LAYOUT_CENTER_X) x = left + (right - left - w) / 2;
					else if (hints & LAYOUT_SIDE_BOTTOM) {  // Right
						x = right - w;
						right -= (w + tw->hspacing);
					}
					else { // Left
						x = left;
						left += (w + tw->hspacing);
					}
				}
			}
			/* Horizontal */
			else {
				/* Width */
				if (hints & LAYOUT_FIX_WIDTH) w = child->width;
				else if (((struct dkWindow *)tw)->options & PACK_UNIFORM_WIDTH) w = mw;
				else if (hints & LAYOUT_FILL_X) w = right - left;
				else w = child->getDefaultWidth(child);

				/* Height */
				if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
				else if (((struct dkWindow *)tw)->options & PACK_UNIFORM_HEIGHT) h = mh;
				else if (hints & LAYOUT_FILL_Y) h = bottom - top;
				else h = child->getHeightForWidth(child, w);   /* Height is a function of width! */

				/* X */
				if (!((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X))) {
					if (hints & LAYOUT_CENTER_X) x = left + (right - left - w) / 2;
					else if (hints & LAYOUT_RIGHT) x = right - w;
					else x = left;
				}

				/* Y */
				if (!((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y))) {
					if (hints & LAYOUT_CENTER_Y) y = top + (bottom - top - h) / 2;
					else if (hints & LAYOUT_SIDE_BOTTOM) {  /* Bottom */
						y = bottom - h;
						bottom -= (h + tw->vspacing);
					}
					else {                                  /* Top */
						y = top;
						top += (h + tw->vspacing);
					}
				}
			}
			child->position(child, x, y, w, h);
		}
	}
  ((struct dkWindow *)tw)->flags &= ~FLAG_DIRTY;
}

void dkTopWindowPlace(struct dkTopWindow *tw, DKuint placement)
{
  int rx, ry, rw, rh, ox, oy, ow, oh, wx, wy, ww, wh, x, y;
  DKuint state;
  struct dkWindow *over;
#ifdef WIN32
  RECT rect;
  MYMONITORINFO minfo;
  HANDLE monitor;
#endif

  /* Default placement:- leave it where it was */
  wx = ((struct dkWindow *)tw)->xpos;
  wy = ((struct dkWindow *)tw)->ypos;
  ww = ((struct dkWindow *)tw)->width;
  wh = ((struct dkWindow *)tw)->height;

  /* Get root window size */
#ifdef WIN32
  /* Use mouse position to select screen */
  if (placement != PLACEMENT_OWNER) {
    struct dkWindow *root;

    root = dkWindowGetRoot((struct dkWindow *)tw);
    dkWindowGetCursorPosition((struct dkWindow *)root, &x, &y, &state);
    rect.left = x;
    rect.right = x + 1;
    rect.top = y;
    rect.bottom = y + 1;
  }
  /* Use owner to select screen */
  else {
    over = ((struct dkWindow *)tw)->owner ? ((struct dkWindow *)tw)->owner : dkWindowGetRoot((struct dkWindow *)tw);
    ow = over->width;
    oh = over->height;
  }

  /* Get monitor infor if we have this API */
  monitor = dkMonitorFromRect(&rect, MONITOR_DEFAULTTOPRIMARY);
  if (monitor) {
    memset(&minfo, 0, sizeof(minfo));
    minfo.cbSize = sizeof(minfo);
    dkGetMonitorInfo(monitor, &minfo);
    rx = minfo.rcWork.left;
    ry = minfo.rcWork.top;
    rw = minfo.rcWork.right - minfo.rcWork.left;
    rh = minfo.rcWork.bottom - minfo.rcWork.top;
  }
  /* Otherwise use the work-area */
  else {
    SystemParametersInfo(SPI_GETWORKAREA, sizeof(RECT), &rect, 0);
    rx = rect.left;
    ry = rect.top;
    rw = rect.right - rect.left;
    rh = rect.bottom - rect.top;
  }
#else
  rx = dkWindowGetRoot((struct dkWindow *)tw)->xpos;
  ry = dkWindowGetRoot((struct dkWindow *)tw)->ypos;
  rw = dkWindowGetRoot((struct dkWindow *)tw)->width;
  rh = dkWindowGetRoot((struct dkWindow *)tw)->height;
#endif

  switch (placement) {
  case PLACEMENT_CURSOR:
    /* Get dialog location in root coordinates */
    dkWindowTranslateCoordinatesTo((struct dkWindow *)tw, &wx, &wy, dkWindowGetRoot((struct dkWindow *)tw), 0, 0);

    /* Where's the mouse? */
    dkWindowGetCursorPosition(dkWindowGetRoot((struct dkWindow *)tw), &x, &y, &state);

    break;
  /* Place centered on the screen */
  case PLACEMENT_SCREEN:
    /* Adjust position */
    wx = rx + (rw - ww) / 2;
    wy = ry + (rh - wh) / 2;
    break;
  /* Place maximized */
  case PLACEMENT_MAXIMIZED:
    wx = rx;
    wy = ry;
    ww = rw;                /* Yes, I know:- we should substract the borders; */
    wh = rh;                /* trouble is, no way to know how big those are.... */
    break;
  /* Default placement */
  case PLACEMENT_DEFAULT:
  default:
    break;
  }

  /* Place it */
  ((struct dkWindow *)tw)->position((struct dkWindow *)tw, wx, wy, ww, wh);
}

void dkWindowShowAndPlace(struct dkWindow *w, unsigned int place)
{
  /* place */
  dkTopWindowPlace((struct dkTopWindow *)w, place);
  DtkShellWindowShow(w);
  DtkTopWindowRaise(w);
}

void
DtkTopWindowRaise(struct dkWindow *w)
{
	if (w->xid) {
#ifdef WIN32
		SetForegroundWindow((HWND)w->xid);
#endif
	}
}
