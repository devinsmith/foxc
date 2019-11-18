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
 * $Id: FXWindow.h,v 1.149 2006/01/22 17:58:12 fox Exp $                      *
 *****************************************************************************/

#ifndef FX_WINDOW_H
#define FX_WINDOW_H

#include "fxdefs.h"

#include "fxapp.h"
#include "fxcursor.h"

/* Window defines */
#define FLAG_SHOWN        0x00000001     /* Is shown */
#define FLAG_ENABLED      0x00000002     /* Able to receive input */
#define FLAG_UPDATE       0x00000004     /* Is subject to GUI update */
#define FLAG_DROPTARGET   0x00000008     /* Drop target */
#define FLAG_FOCUSED      0x00000010     /* Has focus */
#define FLAG_DIRTY        0x00000020     /* Needs layout */
#define FLAG_RECALC       0x00000040     /* Needs recalculation */
#define FLAG_TIP          0x00000080     /* Show tip */
#define FLAG_HELP         0x00000100     /* Show help */
#define FLAG_DEFAULT      0x00000200     /* Default widget */
#define FLAG_INITIAL      0x00000400     /* Initial widget */
#define FLAG_SHELL        0x00000800     /* Shell window */
#define FLAG_ACTIVE       0x00001000     /* Window is active */
#define FLAG_PRESSED      0x00002000     /* Button has been pressed */
#define FLAG_KEY          0x00004000     /* Keyboard key pressed */
#define FLAG_CARET        0x00008000     /* Caret is on */
#define FLAG_CHANGED      0x00010000     /* Window data changed */
#define FLAG_LASSO        0x00020000     /* Lasso mode */
#define FLAG_TRYDRAG      0x00040000     /* Tentative drag mode */
#define FLAG_DODRAG       0x00080000     /* Doing drag mode */
#define FLAG_SCROLLINSIDE 0x00100000     /* Scroll only when inside */
#define FLAG_SCROLLING    0x00200000     /* Right mouse scrolling */
#define FLAG_OWNED        0x00400000

/// Layout hints for child widgets
enum {
  LAYOUT_NORMAL      = 0,                                   /// Default layout mode
  LAYOUT_SIDE_TOP    = 0,                                   /// Pack on top side (default)
  LAYOUT_SIDE_BOTTOM = 0x00000001,                          /// Pack on bottom side
  LAYOUT_SIDE_LEFT   = 0x00000002,                          /// Pack on left side
  LAYOUT_SIDE_RIGHT  = LAYOUT_SIDE_LEFT|LAYOUT_SIDE_BOTTOM, /// Pack on right side
  LAYOUT_FILL_COLUMN = 0x00000001,                          /// Matrix column is stretchable
  LAYOUT_FILL_ROW    = 0x00000002,                          /// Matrix row is stretchable
  LAYOUT_LEFT        = 0,                                   /// Stick on left (default)
  LAYOUT_RIGHT       = 0x00000004,                          /// Stick on right
  LAYOUT_CENTER_X    = 0x00000008,                          /// Center horizontally
  LAYOUT_FIX_X       = LAYOUT_RIGHT|LAYOUT_CENTER_X,        /// X fixed
  LAYOUT_TOP         = 0,                                   /// Stick on top (default)
  LAYOUT_BOTTOM      = 0x00000010,                          /// Stick on bottom
  LAYOUT_CENTER_Y    = 0x00000020,                          /// Center vertically
  LAYOUT_FIX_Y       = LAYOUT_BOTTOM|LAYOUT_CENTER_Y,       /// Y fixed
  LAYOUT_DOCK_SAME   = 0,                                   /// Dock on same galley if it fits
  LAYOUT_DOCK_NEXT   = 0x00000040,                          /// Dock on next galley
  LAYOUT_RESERVED_1  = 0x00000080,
  LAYOUT_FIX_WIDTH   = 0x00000100,                          /// Width fixed
  LAYOUT_FIX_HEIGHT  = 0x00000200,                          /// height fixed
  LAYOUT_MIN_WIDTH   = 0,                                   /// Minimum width is the default
  LAYOUT_MIN_HEIGHT  = 0,                                   /// Minimum height is the default
  LAYOUT_FILL_X      = 0x00000400,                          /// Stretch or shrink horizontally
  LAYOUT_FILL_Y      = 0x00000800,                          /// Stretch or shrink vertically
  LAYOUT_FILL        = LAYOUT_FILL_X|LAYOUT_FILL_Y,         /// Stretch or shrink in both directions
  LAYOUT_EXPLICIT    = LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT   /// Explicit placement
};

/// Packing style (for packers)
enum {
  PACK_NORMAL         = 0,              /// Default is each its own size
  PACK_UNIFORM_HEIGHT = 0x00008000,     /// Uniform height
  PACK_UNIFORM_WIDTH  = 0x00010000      /// Uniform width
};

/* Frame border appearance styles (for subclasses) */
enum {
  FRAME_NONE   = 0,                                     /// Default is no frame
  FRAME_SUNKEN = 0x00001000,                            /// Sunken border
  FRAME_RAISED = 0x00002000,                            /// Raised border
  FRAME_THICK  = 0x00004000,                            /// Thick border
  FRAME_GROOVE = FRAME_THICK,                           /// A groove or etched-in border
  FRAME_RIDGE  = FRAME_THICK|FRAME_RAISED|FRAME_SUNKEN, /// A ridge or embossed border
  FRAME_LINE   = FRAME_RAISED|FRAME_SUNKEN,             /// Simple line border
  FRAME_NORMAL = FRAME_SUNKEN|FRAME_THICK               /// Regular raised/thick border
};

/* Message ID's common to most Windows */
enum {
  WIN_ID_NONE,
  WIN_ID_HIDE,            // ID_HIDE+FALSE
  WIN_ID_SHOW,            // ID_HIDE+TRUE
    ID_TOGGLESHOWN,
    ID_LOWER,
    ID_RAISE,
    ID_DELETE,
    ID_DISABLE,         // ID_DISABLE+FALSE
    ID_ENABLE,          // ID_DISABLE+TRUE
    ID_TOGGLEENABLED,
    ID_UNCHECK,         // ID_UNCHECK+FALSE
    ID_CHECK,           // ID_UNCHECK+TRUE
    ID_UNKNOWN,         // ID_UNCHECK+MAYBE
    ID_UPDATE,
    ID_AUTOSCROLL,
    ID_TIPTIMER,
  WIN_ID_HSCROLLED,
  WIN_ID_VSCROLLED,
    ID_SETVALUE,
    ID_SETINTVALUE,
    ID_SETREALVALUE,
    ID_SETSTRINGVALUE,
    ID_SETICONVALUE,
    ID_SETINTRANGE,
    ID_SETREALRANGE,
    ID_GETINTVALUE,
    ID_GETREALVALUE,
    ID_GETSTRINGVALUE,
    ID_GETICONVALUE,
    ID_GETINTRANGE,
    ID_GETREALRANGE,
    ID_SETHELPSTRING,
    ID_GETHELPSTRING,
    ID_SETTIPSTRING,
    ID_GETTIPSTRING,
    ID_QUERY_MENU,
    ID_HOTKEY,
    ID_ACCEL,
    ID_UNPOST,
    ID_POST,
    ID_MDI_TILEHORIZONTAL,
    ID_MDI_TILEVERTICAL,
    ID_MDI_CASCADE,
    ID_MDI_MAXIMIZE,
    ID_MDI_MINIMIZE,
    ID_MDI_RESTORE,
    ID_MDI_CLOSE,
    ID_MDI_WINDOW,
    ID_MDI_MENUWINDOW,
    ID_MDI_MENUMINIMIZE,
    ID_MDI_MENURESTORE,
    ID_MDI_MENUCLOSE,
    ID_MDI_NEXT,
    ID_MDI_PREV,
    ID_LAST,
    ID_LAYOUT
};

/* Forward declarations */
struct dkEvent;
struct dkDrawable;
struct dkComposite;

DKDragType dkoctetType;          // Raw octet stream
DKDragType dkdeleteType;         // Delete request
DKDragType dktextType;           // Ascii text request
DKDragType dkutf8Type;           // UTF-8 text request
DKDragType dkutf16Type;          // UTF-16 text request
DKDragType dkcolorType;          // Color
DKDragType dkurilistType;        // URI List
DKDragType dkstringType;         // Clipboard text type (pre-registered)
DKDragType dkimageType;          // Clipboard image type (pre-registered)

/* Inherits from dkObject */
struct dkWindow {
	struct dkObject base;

	struct dkApp *app; /* Back link to application object */
	void *data; /* user data */

	DKID xid;

	/* Create resource */
	void (*create) (void *pthis);
#ifdef WIN32
  DKID (*getDC) (struct dkWindow *d);
  int (*releaseDC) (struct dkWindow *d, DKID id);
#endif
  /* Resize drawable to the specified width and height */
  void (*resize) (struct dkWindow *d, int w, int h);

  struct FXVisual     *visual;                 /* Visual for this window */
  int width;                  /* Width */
  int height;                 /* Height */

  /*
  ** dkWindow virtual functions
  */

  int (*canFocus) (void);
  void (*setFocus) (struct dkWindow *w);
  void (*killFocus) (struct dkWindow *w);
  void (*changeFocus) (struct dkWindow *w, struct dkWindow *child);

  void (*setDefault) (struct dkWindow *w, DKbool enable);

  /* Enable the window to receive mouse and keyboard events */
  void (*enable) (struct dkWindow *w);

  /* Disable the window from receiving mouse and keyboard events */
  void (*disable) (struct dkWindow *w);

	/* Show this window */
	void (*show) (struct dkWindow *w);
	void (*layout) (struct dkWindow *w);
	void (*recalc) (struct dkWindow *w);

	/**
	* Move and resize the window immediately, in the parent's coordinate system.
	* Update the server representation as well if the window is realized.
	* Perform layout of the children when necessary.
	*/
	void (*position) (struct dkWindow *win, int x, int y, int w, int h);

	/* Return the default width of this window */
	int (*getDefaultWidth) (struct dkWindow *w);
	/* Return the default height of this window */
	int (*getDefaultHeight) (struct dkWindow *w);
	/* Return width for given height */
	int (*getWidthForHeight) (struct dkWindow *w, int givenheight);
	/* Return height for given width */
	int (*getHeightForWidth) (struct dkWindow *w, int givenwidth);

  int (*doesSaveUnder) (void);
  /* dkWindow fields */

	char *classname;  /* Temporary */

	struct dkWindow *parent;             /* Parent Window */
	struct dkWindow *owner;              /* Owner Window */
	struct dkWindow *first;              /* First Child */
	struct dkWindow *last;               /* Last Child */
	struct dkWindow *next;               /* Next Sibling */
	struct dkWindow *prev;               /* Previous Sibling */
	struct dkWindow *focus;              /* Focus Child */
	unsigned int wk;                     /* Window Key */

  struct dkComposeContext *composeContext;  /* Compose context */
  struct dkCursor *defaultCursor;           /* Normal Cursor */
  struct dkCursor *dragCursor;              /* Cursor during drag */
  struct dkObject *target;             /* Target object */
	DKSelector       message;            /* Message ID */
	int              xpos;               /* Window X Position */
	int              ypos;               /* Window Y Position */
	unsigned int     backColor;          /* Window background color */
	int              flags;              /* Window state flags */
	DKuint           options;            /* Window options */

  void (*actionCb)(struct dkWindow *win, void *udata);
  void *cbextra;
  void *wExtra;             /* Window extra stuff */
	int (*onPaint)(struct dkWindow *win, struct dkEvent *data);
};

typedef void (*dkActionCallback)(struct dkWindow *win, void *udata);

/* Constructor for dkWindow for Root windows only */
void DtkWindowRootInit(struct dkWindow *w, struct dkApp *app, struct FXVisual *v);
void DtkWindowShellCtor(struct dkWindow *pthis, struct dkApp *app, struct dkWindow *win, DKuint opts, int x, int y, int w, int h);
void dkWindowChildInit(struct dkWindow *pthis, struct dkWindow *comp, DKuint opts, int x, int y, int w, int h);

int dkWindowIsShown(struct dkWindow *w);
int dkWindowIsDefault(struct dkWindow *w);
int dkWindowIsEnabled(struct dkWindow *w);
int dkWindowIsOwnerOf(struct dkWindow *pthis, struct dkWindow *window);
int dkWindowHasFocus(struct dkWindow *w);
int dkWindowHasSelection(struct dkWindow *w);
DKuint dkWindowGetLayoutHints(struct dkWindow *w);
struct dkWindow * dkWindowCommonAncestor(struct dkWindow *a, struct dkWindow *b);

long dkWindowCallWindowProc(dkHandleProc proc, struct dkWindow *win, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
dkHandleProc dkWindowGetHandleProc(struct dkWindow *win);
void dkWindowSetHandleProc(struct dkWindow *win, dkHandleProc proc);
void dkWindowSetActionCallback(struct dkWindow *win, dkActionCallback cb, void *udata);

void dkWindowLayout(struct dkWindow *pthis);
void dkWindowRecalc(struct dkWindow *pthis);
void DtkCreateWindow(void *pthis);
long dkWindow_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
struct dkWindow *dkWindowGetRoot(struct dkWindow *pthis);
int dkWindowGetCursorPosition(struct dkWindow *w, int *x, int *y, DKuint *buttons);
void dkWindowTranslateCoordinatesTo(struct dkWindow *pthis, int *tox, int *toy, struct dkWindow *towindow, int fromx, int fromy);
void dkWindowTranslateCoordinatesFrom(struct dkWindow *pthis, int *tox, int *toy, struct dkWindow *fromwindow, int fromx, int fromy);
int dkWindowContainsChild(struct dkWindow *pthis, struct dkWindow *child);
void dkWindowUpdateRect(struct dkWindow *win, int x, int y, int w, int h);
void dkWindowUpdate(struct dkWindow *win);
void dkWindowGrab(struct dkWindow *win);
void dkWindowUngrab(struct dkWindow *win);
int dkWindowAcquireSelection(struct dkWindow *w, DKDragType *types, DKuint numtypes);

/* Message handlers */
long dkWindow_onUpdate(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkWindow_onEnter(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkWindow_onLeave(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkWindow_onFocusIn(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkWindow_onFocusOut(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkWindow_onFocusSelf(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
long dkWindow_onSelectionLost(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
long dkWindow_onSelectionGained(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);

void dkWindow_setDefault(struct dkWindow *w, DKbool enable);
void dkWindow_setFocus(struct dkWindow *win);
void dkWindow_killFocus(struct dkWindow *pthis);
int dkWindow_releaseSelection(struct dkWindow *pthis);
void dkWindow_scroll(struct dkWindow *win, int x, int y, int w, int h, int dx, int dy);
void dkWindow_raise(struct dkWindow *win);
void dkWindowHide(struct dkWindow *win);

#endif /* FX_WINDOW_H */
