/******************************************************************************
 *                                                                            *
 *                  A p p l i c a t i o n   O b j e c t                       *
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
 * $Id: FXApp.h,v 1.230 2006/01/22 17:57:58 fox Exp $                         *
 *****************************************************************************/

#ifndef FX_APP_H
#define FX_APP_H

#include <sys/types.h>

#include "fxdefs.h"

#include "fxcursor.h"
#include "fxobject.h"
#include "fxstring.h"
#include "fxvisual.h"
#include "fxwindow.h"
#include "fxthread.h"

/* Default cursors provided by the application */
enum DtkDefaultCursor {
  DEF_ARROW_CURSOR,                     /* Arrow cursor */
  DEF_RARROW_CURSOR,                    /* Reverse arrow cursor */
  DEF_TEXT_CURSOR                       /* Text cursor */
};

/* Messages */
enum {
	ID_QUIT = 1,    /// Terminate the application normally
	ID_DUMP,      /// Dump the current widget tree
	ID_HOVER
};

/* All ways of being modal */
enum dkModality {
  MODAL_FOR_NONE,         /* Non modal event loop (dispatch normally) */
  MODAL_FOR_WINDOW,       /* Modal dialog (beep if outside of modal dialog) */
  MODAL_FOR_POPUP         /* Modal for popup (always dispatch to popup) */
};

struct dtkRectangle {
	short x;
	short y;
	short w;
	short h;
};

struct dtkHash {
	struct dtkEntry *table;     // Hash table
	unsigned int   total;       // Table size
	unsigned int   used;        // Number of used entries
	unsigned int   free;        // Number of free entries
};

/* dkEvent */
struct dkEvent {
  DKuint            type;           /* Event type */
  DKuint            time;           /* Time of last event */
  int               win_x;          /* Window-relative x-coord */
  int               win_y;          /* Window-relative y-coord */
  int               root_x;         /* Root x-coord */
  int               root_y;         /* Root y-coord */
  int               state;          /* Mouse button and modifier key state */
  int               code;           /* Button, Keysym, or mode; DDE Source */
  struct dstr       text;           /* Text of keyboard event */
  int               last_x;         /* Window-relative x-coord of previous mouse location */
  int               last_y;         /* Window-relative y-coord of previous mouse location */
  int               click_x;        /* Window-relative x-coord of mouse press */
  int               click_y;        /* Window-relative y-coord of mouse press */
  int               rootclick_x;    /* Root-relative x-coord of mouse press */
  int               rootclick_y;    /* Root-relative y-coord of mouse press */
  DKuint            click_time;     /* Time of mouse button press */
  DKuint            click_button;   /* Mouse button pressed */
  int               click_count;    /* Click-count */
  DKbool            moved;          /* Moved cursor since press */

  struct dtkRectangle rect;         /* Rectangle */
  int synthetic;                    /* True if synthetic expose event */
};

/* dkApp inherits from dkObject */
struct dkApp {
  struct dkObject base;

  /* Start of dkApp's public and private members */
  int  appArgc;         /* Argument count */
  char **appArgv;       /* Argument vector */

  int display_opened;
  void *display;        /* Display we're talking to */
  char *dpy;            /* Initial display guess */
  struct dkWindow *activeWindow; /* Active toplevel window */
  struct dkWindow *cursorWindow; /* Window under the cursor */
  struct dkWindow *mouseGrabWindow;     /* Window which grabbed the mouse */
  struct dkWindow *keyboardGrabWindow;  /* Window which grabbed the keyboard */
  struct dkWindow *keyWindow;           /* Window in which keyboard key was pressed */
  struct dkWindow *selectionWindow;     /* Selection window */

  struct fx_invocation  *invocation;
  struct dkFont          *normalFont;          /* Normal font */
  struct dkMutex          appMutex;            /* Application wide mutex */

  struct dkEvent event;
  DKuint   stickyMods;          /* Sticky modifier state */

  FXVisual *defaultVisual;     /* Default [color] visual */

	struct dkWindow        *refresher;           /* GUI refresher pointer */
	struct dkWindow        *refresherstop;       /* GUI refresher end pointer */

	struct dkWindow *root;                       /* Pointer to the root window */
	dkCursor *cursor[DEF_TEXT_CURSOR + 1]; /* Default cursors */
	int ninputs;                  /* Number of inputs */
	int maxinput;                 /* Maximum input number */
  DKuint           clickSpeed;  /* Double click speed */
  DKuint           blinkSpeed;  /* Cursor blink speed */
  int              dragDelta;   /* Minimum distance considered a move */

  int              scrollBarSize;       /* Scrollbar size */
	DKColor          borderColor;         /* Border color */
	DKColor          baseColor;           /* Background color of GUI controls */
	DKColor          hiliteColor;         /* Highlight color of GUI controls */
	DKColor          shadowColor;         /* Shadow color of GUI controls */
	DKColor          backColor;           /* Background color */
	DKColor          foreColor;           /* Foreground color */
	DKColor          selforeColor;        /* Select foreground color */
	DKColor          selbackColor;        /* Select background color */
	DKColor          tipforeColor;        /* Tooltip foreground color */
	DKColor          tipbackColor;        /* Tooltip background color */
	DKColor          selMenuTextColor;    /* Select foreground color in menus */
	DKColor          selMenuBackColor;    /* Select background color in menus */

  struct fxTimer         *timers;              /* List of timers, sorted by time */
  struct dkChore         *chores;              /* List of chores */
  struct dkRepaint       *repaints;            /* Unhandled repaint rectangles */
  struct fxTimer         *timerrecs;           /* List of recycled timer records */
  struct dkChore         *chorerecs;           /* List of recycled chore records */
  struct dkRepaint       *repaintrecs;         /* List of recycled repaint records */

#ifndef WIN32
  DKID             wmMotifHints;        /* Motif hints */
  DKID             wmState;             /* Window state */
  DKID             wmNetState;          /* Extended Window Manager window state */
  DKDragType      *xselTypeList;        /* Selection type list */
  DKuint           xselNumTypes;        /* Selection number of types on list */
  DKID             xdndAware;           /* XDND awareness atom */
  int              xrreventbase;        /* XRR event base */
  DKID             stipples[23];        /* Standard stipple patterns */
  void            *r_fds;               /* Set of file descriptors for read */
  void            *w_fds;               /* Set of file descriptors for write */
  void            *e_fds;               /* Set of file descriptors for exceptions */
#else
  DKDragType      *xselTypeList;        /* Selection type list */
  DKuint           xselNumTypes;        /* Selection number of types on list */
  DKID             stipples[17];        /* Standard stipple bitmaps */
  void           **handles;             /* Waitable object handles */
#endif
};

/* Application startup/init/destroy */
App * dkAppNew(void);
void  dkAppDel(App *app);

void  dkAppInit(App *app, int argc, char *argv[]);
void  dkAppCreate(App *app);
void  dkAppBeep(struct dkApp *app);
int   dkAppRun(App *app);
void  dkAppExit(struct dkApp *app, int value);
void  dkAppStop(struct dkApp *app, int value);
void  DtkAppRefresh(App *app);
void dkAppAddChore(struct dkApp *app, struct dkObject *tgt, DKSelector sel, void *ptr);
void dkAppRemoveChore(struct dkApp *app, struct dkObject* tgt, DKSelector sel);
void fxAppAddTimeout(struct dkApp *app, struct dkObject *tgt, DKSelector sel, DKuint ms, void *ptr);
void fxAppRemoveTimeout(struct dkApp *app, struct dkObject *tgt, DKSelector sel);
void dkAppEnterWindow(struct dkApp *app, struct dkWindow *window, struct dkWindow *ancestor);
void dkAppLeaveWindow(struct dkApp *app, struct dkWindow *window, struct dkWindow *ancestor);
void dkAppAddRepaint(struct dkApp *app, DKID win, int x, int y, int w, int h, DKbool synth);
void dkAppRemoveRepaints(struct dkApp *app, DKID win, int x, int y, int w, int h);
void dkAppScrollRepaints(struct dkApp *app, DKID win, int dx, int dy);

/* composite.c */
dtkWindow * DtkNewCompositeRoot(App *app, FXVisual *v);
void DtkCreateComposite(void *pthis);

/* shell.c */
dtkWindow * DtkNewShellTopWindow(App *app);
void DtkShellWindowShow(struct dkWindow *w);

/* top_window.c */
dtkWindow * DtkNewTopWindow(App *app, char *title);
void dkWindowShowAndPlace(struct dkWindow *w, unsigned int place);
void DtkTopWindowRaise(struct dkWindow *w);

/* window.c */
dtkWindow * DtkNewWindowRoot(App *app, FXVisual *v);
dtkWindow * DtkNewWindowShell(App *app, dtkWindow *own);
void DtkCreateWindow(void *pthis);
void DtkWindowShow(struct dkWindow *w);
void DtkWindowHashNew(void);
struct dkWindow * DtkFindWindowWithId(DKID xid);
void DtkWindowHashInsert(void *key, void *value);
struct dkWindow * DtkWindowGetShell(struct dkWindow *w);

/* visual.c */
FXVisual * DtkNewVisual(App *app, unsigned int flags);
void DtkCreateVisual(FXVisual *v);

dtkWindow * DtkButtonNew(dtkWindow *w, char *title);

/* Memory management (fxobject.c) */
void * fx_alloc(size_t bytes);
int    fx_resize(void **ptr, unsigned long size);

/* Hash functions */
struct dtkHash *DtkHashNew(void);
void DtkHashFree(struct dtkHash *h);
void DtkHashResize(struct dtkHash *h, unsigned int m);
void *DtkHashInsert(struct dtkHash *h, void *key, void *value);
void *DtkHashReplace(struct dtkHash *h, void *key, void *value);
void *DtkHashRemove(struct dtkHash *h, void* key);
void *DtkHashFind(struct dtkHash *h, void *key);
void DtkHashClear(struct dtkHash *h);

#endif /* FX_APP_H */
