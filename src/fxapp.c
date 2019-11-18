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
 * $Id: FXApp.cpp,v 1.617.2.8 2008/05/08 12:54:16 fox Exp $                   *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>
#else /* X11 */
#include "config.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#ifdef HAVE_XRANDR_H
#include <X11/extensions/Xrandr.h>
#endif /* X11 */
#endif /* WIN32 */

#include "fxapp.h"
#include "fxdc.h"
#include "fxdrv.h"
#include "fxfont.h"
#include "fxkeys.h"
#include "fxrootwindow.h"
#include "fxthread.h"
#include "fxutils.h"

/*
  Notes:

  - Should not be able to DND drop into some windows while modal window is up.

  - Refresh() now forces total GUI updating; flush() updates display only.

  - Sender is the FXApp, but in case of DND ops, we might have a special stand-in object
    as the sender (e.g. so we can send it messages).

  - Application should have a target to be notified of certain app-wide events,
    such as activation of [a window of] the application etc.

  - Need way to specify visual on command line (X11).

  - Need to be able to run event loop w/o display connection (just I/O, and timers).

  - Need to be able to ``detach'' from GUI more cleanly.

  - FXApp::exit() should be called by AFTER returning from FXApp::run(); also,
    FXApp::exit() should not call the global exit():- just because we're done doing
    GUI things does not mean we're done with the program!

  - When timer, signal, I/O, chore callback fires, need to go once round the event
    loop, because these callbacks may set a flag to break out of the event loop.
    This is done by letting getNextEvent() return FALSE if it returns with NO
    event [The alternative would be to dispatch these events via dispatchEvent()
    which is currently a bit difficult].

  - Event logging (journalling) and playback.  We need some basic capabilities for
    journalling and playback of user-inputs [basically, mouse and keyboard events].
    How exactly this is going to work is not entirely clear, but there are a couple
    of issues:

      - Mapping window ID's to something we can save on the file. FXWindow now
        generates a window-key which can identify each window by means of a kind of
        Dewey Decimal system.
      - Which events need logging? Of course, mouse buttons, motion, and keyboard, but
        how about enter/leave? The real mouse may be moving around too while playing
        back!
      - And what should be recorded.
      - Plus, some difficulty with event dispatch on Windows.

  - Pre- and post-dispatch hooks. In the pre-dispatch hook, the event is presented to
    some user-defined function (or maybe message handler) and can be inspected prior to
    dispatch.  Returning a TRUE or FALSE from the pre-dispatch hook function will cause
    the actual dispatch to be blocked or not.

    Thus, the predispatch hook can act as an event filter, and events which are passed are
    dispatched normally.

    The post-dispatch hook is presented with the event AFTER it was dispatched.
    Either we present the event to the post-dispatch hook only when it was
    actually handled somewhere in the GUI, or we always present it and pass
    a flag that says whether it has been handled by some widget or not. The chief purpose
    of a post-dispatch hook is for event logging purposes.

  - Make sure keyboard gets dispatched to modal window [dialog or popup or whatever].

  - FXInvocation into a class to its dtor will be called [just in case someone tries to
    throw an exception].   Having a dtor allows clean up without using a try-catch
    construct.

  - Modal modes for FXInvocation:

      - Non-modal for unconstrained model loops, like e.g. toplevel loop.
      - Modal for window, typically a dialog but generally modal for any
        window and its inferiors.  Clicking outside the modal window will
        cause a beep.
      - Application modal, i.e. always beep no matter which window.  This
        is useful for complete blocking of user-events while still performing
        layouts and repaints and so on.
      - Popup modal.  Very similar to Modal for a window, except when clicking
        outside the popup stack is closed instead of issuing a beep.

*/

#ifndef WIN32

/* 17 stipple patterns which match up exactly with the 4x4 dither kernel */
static const unsigned char stipple_patterns[17][8] = {
  {0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00},   // 0 (white)
  {0x00,0x00,0x00,0x88, 0x00,0x00,0x00,0x88},
  {0x00,0x22,0x00,0x88, 0x00,0x22,0x00,0x88},
  {0x00,0x22,0x00,0xaa, 0x00,0x22,0x00,0xaa},
  {0x00,0xaa,0x00,0xaa, 0x00,0xaa,0x00,0xaa},
  {0x00,0xaa,0x44,0xaa, 0x00,0xaa,0x44,0xaa},
  {0x11,0xaa,0x44,0xaa, 0x11,0xaa,0x44,0xaa},
  {0x11,0xaa,0x55,0xaa, 0x11,0xaa,0x55,0xaa},
  {0x55,0xaa,0x55,0xaa, 0x55,0xaa,0x55,0xaa},   // 8 (50% grey)
  {0x55,0xaa,0x55,0xee, 0x55,0xaa,0x55,0xee},
  {0x55,0xbb,0x55,0xee, 0x55,0xbb,0x55,0xee},
  {0x55,0xbb,0x55,0xff, 0x55,0xbb,0x55,0xff},
  {0x55,0xff,0x55,0xff, 0x55,0xff,0x55,0xff},
  {0x55,0xff,0xdd,0xff, 0x55,0xff,0xdd,0xff},
  {0x77,0xff,0xdd,0xff, 0x77,0xff,0xdd,0xff},
  {0x77,0xff,0xff,0xff, 0x77,0xff,0xff,0xff},
  {0xff,0xff,0xff,0xff, 0xff,0xff,0xff,0xff}    // 16 (black)
};

/* Standard-issue cross hatch pattern */
static const unsigned char cross_bits[] = {
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0xff, 0xff, 0xff,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0xff, 0xff, 0xff, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20
};

/* Standard-issue diagonal cross hatch pattern */
static const unsigned char crossdiag_bits[] = {
  0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14, 0x22, 0x22, 0x41, 0x41,
  0x80, 0x80, 0x41, 0x41, 0x22, 0x22, 0x14, 0x14, 0x08, 0x08, 0x14, 0x14,
  0x22, 0x22, 0x41, 0x41, 0x80, 0x80, 0x41, 0x41
};

/* Standard-issue diagonal hatch pattern */
static const unsigned char diag_bits[] = {
  0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04, 0x02, 0x02, 0x01, 0x01,
  0x80, 0x80, 0x40, 0x40, 0x20, 0x20, 0x10, 0x10, 0x08, 0x08, 0x04, 0x04,
  0x02, 0x02, 0x01, 0x01, 0x80, 0x80, 0x40, 0x40
};

/* Standard-issue horizontal hatch pattern */
static const unsigned char hor_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

/* Standard-issue reverse diagonal hatch pattern */
static const unsigned char revdiag_bits[] = {
  0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10, 0x20, 0x20, 0x40, 0x40,
  0x80, 0x80, 0x01, 0x01, 0x02, 0x02, 0x04, 0x04, 0x08, 0x08, 0x10, 0x10,
  0x20, 0x20, 0x40, 0x40, 0x80, 0x80, 0x01, 0x01
};

/* Standard-issue vertical hatch pattern */
static const unsigned char ver_bits[] = {
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20,
  0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20, 0x08, 0x82, 0x20
};

#else

// 17 stipple patterns which match up exactly with the 4x4 dither kernel
// Note that each scan line must be word-aligned so we pad to the right
// with zeroes.
static const BYTE stipple_patterns[17][16]={
  {0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00, 0xff,0x00,0xff,0x00,0xff,0x00,0xff,0x00},   // 0 (white)
  {0xff,0x00,0xff,0x00,0xff,0x00,0x77,0x00, 0xff,0x00,0xff,0x00,0xff,0x00,0x77,0x00},
  {0xff,0x00,0xdd,0x00,0xff,0x00,0x77,0x00, 0xff,0x00,0xdd,0x00,0xff,0x00,0x77,0x00},
  {0xff,0x00,0xdd,0x00,0xff,0x00,0x55,0x00, 0xff,0x00,0xdd,0x00,0xff,0x00,0x55,0x00},
  {0xff,0x00,0x55,0x00,0xff,0x00,0x55,0x00, 0xff,0x00,0x55,0x00,0xff,0x00,0x55,0x00},
  {0xff,0x00,0x55,0x00,0xbb,0x00,0x55,0x00, 0xff,0x00,0x55,0x00,0xbb,0x00,0x55,0x00},
  {0xee,0x00,0x55,0x00,0xbb,0x00,0x55,0x00, 0xee,0x00,0x55,0x00,0xbb,0x00,0x55,0x00},
  {0xee,0x00,0x55,0x00,0xaa,0x00,0x55,0x00, 0xee,0x00,0x55,0x00,0xaa,0x00,0x55,0x00},
  {0xaa,0x00,0x55,0x00,0xaa,0x00,0x55,0x00, 0xaa,0x00,0x55,0x00,0xaa,0x00,0x55,0x00},   // 8 (50% grey)
  {0xaa,0x00,0x55,0x00,0xaa,0x00,0x11,0x00, 0xaa,0x00,0x55,0x00,0xaa,0x00,0x11,0x00},
  {0xaa,0x00,0x44,0x00,0xaa,0x00,0x11,0x00, 0xaa,0x00,0x44,0x00,0xaa,0x00,0x11,0x00},
  {0xaa,0x00,0x44,0x00,0xaa,0x00,0x00,0x00, 0xaa,0x00,0x44,0x00,0xaa,0x00,0x00,0x00},
  {0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00, 0xaa,0x00,0x00,0x00,0xaa,0x00,0x00,0x00},
  {0xaa,0x00,0x00,0x00,0x22,0x00,0x00,0x00, 0xaa,0x00,0x00,0x00,0x22,0x00,0x00,0x00},
  {0x88,0x00,0x00,0x00,0x22,0x00,0x00,0x00, 0x88,0x00,0x00,0x00,0x22,0x00,0x00,0x00},
  {0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x88,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}    // 16 (black)
};

#endif

/* Timer record */
struct fxTimer {
	struct fxTimer       *next;              // Next timeout in list
	struct dkObject      *target;            // Receiver object
	void                 *data;              // User data
	DKSelector            message;           // Message sent to receiver
	DKlong                due;               // When timer is due (ns)
};


/* THIS NEEDS TO GO IN A HEADER FILE */
#ifdef WIN32
extern unsigned int dkmodifierkeys();
extern DKuint wkbMapKeyCode(UINT msg, WPARAM wParam, LPARAM lParam);
#endif

/* Idle record */
struct dkChore {
  struct dkChore       *next;              // Next chore in list
  struct dkObject      *target;            // Receiver object
  void                 *data;              // User data
  DKSelector            message;           // Message sent to receiver
};

/* A repaint event record */
struct dkRepaint {
  struct dkRepaint      *next;              // Next repaint in list
  DKID                   window;            // Window ID of the dirty window
  struct dtkRectangle    rect;              // Dirty rectangle
  int                    hint;              // Hint for compositing
  DKbool                 synth;             // Synthetic expose event or real one?
};

struct fx_invocation {
  struct fx_invocation **invocation;
  struct fx_invocation *upper;
  struct dkWindow *window;
  int modality;
  int code;
  int done;
};

#ifdef WIN32
static void getSystemFont(struct dkFontDesc *fontdesc);
#endif
struct dkWindow *dkAppFindWindowAt(struct dkApp *app, int rx, int ry, DKID window);
struct dkWindow *dkAppGetFocusWindow(struct dkApp *app);

/* Define message target functions */
long dkApp_onCmdHover(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
long dkApp_onCmdQuit(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

static struct dkMapEntry dkAppMapEntry[] = {
  FXMAPFUNC(SEL_TIMEOUT, ID_HOVER, dkApp_onCmdHover),
  FXMAPFUNC(SEL_TIMEOUT, ID_QUIT, dkApp_onCmdQuit),
  FXMAPFUNC(SEL_COMMAND, ID_QUIT, dkApp_onCmdQuit)
};

static struct dkMetaClass dkAppMetaClass = {
	"dkApp", dkAppMapEntry, sizeof(dkAppMapEntry) / sizeof(dkAppMapEntry[0]), sizeof(struct dkMapEntry)
};

static void
fx_invocation_setup(struct fx_invocation *inv, struct fx_invocation **pinv,
    int mode, struct dkWindow *win)
{
	inv->invocation = pinv;
	inv->upper = *pinv;
	inv->window = win;
	inv->modality = mode;
	inv->done = 0;
	inv->code = 0;

	*(inv->invocation) = inv;
}

static void
fx_invocation_destroy(struct fx_invocation *inv)
{
	*(inv->invocation) = inv->upper;
}


long dkApp_onCmdHover(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  int x, y;
  DKuint buttons;
  struct dkWindow *window;
  struct dkApp *app = (struct dkApp *)pthis;

  if (!app->mouseGrabWindow && app->cursorWindow && app->cursorWindow != (struct dkWindow *)app->root) {
    dkWindowGetCursorPosition((struct dkWindow *)app->root, &x, &y, &buttons);
    if ((window = dkAppFindWindowAt(app, x, y, 0)) == NULL || !dkWindowContainsChild(DtkWindowGetShell(window), app->cursorWindow)) {
      app->event.type = SEL_LEAVE;
      app->event.root_x = x;
      app->event.root_y = y;
      dkAppLeaveWindow(app, app->cursorWindow, (struct dkWindow *)app->root);
      return 0;
    }
  }
  fxAppAddTimeout(app, (struct dkObject *)app, ID_HOVER, 200, NULL);
  return 0;
}

void dkAppExit(struct dkApp *app, int code)
{
  DKTRACE((100,"dkApp::exit\n"));

  /* TODO Write the registry */

  /* Exit the program */
  dkAppStop(app, code);
}

/* Handle quit */
long
dkApp_onCmdQuit(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkApp *app = (struct dkApp *)pthis;
  dkAppExit(app, 0);
  return 1;
}

long
dkApp_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;
  DKSelector sel = DKSEL(selhi, sello);

  me = DKMetaClassSearch(&dkAppMetaClass, sel);
  if (me != NULL)
    return me->func(pthis, obj, selhi, sello, data);
  return dkObject_handle(pthis, obj, selhi, sello, data);
}

struct dkApp *
dkAppNew(void)
{
	struct dkApp *ret;

	/* allocate and send off to base class */
	ret = calloc(1, sizeof(struct dkApp));
  dkObjectInit((struct dkObject *)ret);
  ((struct dkObject *)ret)->meta = &dkAppMetaClass;
  ((struct dkObject *)ret)->handle = dkApp_handle;

  dkMutexInit(&ret->appMutex, 0);

  ret->display = NULL;      /* Display connection */
  ret->dpy = ":0.0";        /* Initial display guess */
  ret->activeWindow = NULL; /* Active toplevel window */
  ret->cursorWindow = NULL; /* Window under the cursor */
  ret->mouseGrabWindow = NULL; /* Window which grabbed mouse */
  ret->keyboardGrabWindow = NULL; /* Window which grabbed keyboard */
  ret->keyWindow = NULL;          /* Window in which keyboard key was pressed */
  ret->selectionWindow = NULL;    /* Window which has the selection */

  ret->refresher = NULL;                      /* GUI refresher pointer */
  ret->refresherstop = NULL;                  /* GUI refresher end pointer */
  ret->timers = NULL;                         /* No timers present */
  ret->chores = NULL;                         /* No chores present */
  ret->repaints = NULL;                       /* No outstanding repaints */
  ret->timerrecs = NULL;                      /* No timer records */
  ret->chorerecs = NULL;                      /* No chore records */
  ret->repaintrecs = NULL;                    /* No repaint records */

  ret->ninputs = 8;                           /* Number of these */
  ret->maxinput = -1;                         /* Maximum input number */

  /* Clear event structure */
  ret->event.type = 0;
  ret->event.time = 0;
  ret->event.win_x = 0;
  ret->event.win_y = 0;
  ret->event.rect.x = 0;
  ret->event.rect.y = 0;
  ret->event.rect.w = 0;
  ret->event.rect.h = 0;
  ret->event.synthetic = 0;
  dstr_init(&ret->event.text);

  /* Clear sticky mod state */
  ret->stickyMods = 0;

  /* Default visual */
  ret->defaultVisual = DtkNewVisual(ret, VISUAL_DEFAULT);

  ret->cursor[DEF_ARROW_CURSOR] = FxCursorNew(ret, CURSOR_ARROW);
  ret->cursor[DEF_TEXT_CURSOR] = FxCursorNew(ret, CURSOR_IBEAM);

  ret->root = DtkNewRootWindow(ret, ret->defaultVisual);

  /* X Window specific inits */
#ifndef WIN32
  /* Window Manager Stuff */
  ret->wmMotifHints = 0;
  ret->wmState = 0;

  /* Extended Window Manager Stuff */
  ret->wmNetState = 0;

  /* SELECTION */
  ret->xselTypeList = NULL;            /* List of primary selection types */
  ret->xselNumTypes = 0;               /* How many types in list */

  /* XDND */
  ret->xdndAware = 0;                  /* XDND awareness atom */

  ret->xrreventbase = 0;               /* XRR support */
  ret->r_fds = calloc(sizeof(fd_set), 1);        /* Read File Descriptor set */
  ret->w_fds = calloc(sizeof(fd_set), 1);        /* Write File Descriptor set */
  ret->e_fds = calloc(sizeof(fd_set), 1);        /* Except File Descriptor set */
/* MS-Windows specific inits */
#else
  /* SELECTION */
  ret->xselTypeList = NULL;            /* List of primary selection types */
  ret->xselNumTypes = 0;               /* How many types in list */

  ret->handles = calloc(sizeof(void *), ret->ninputs);   /* Same size as inputs array */
#endif

  /* Other settings */
  ret->clickSpeed = 400;
  ret->blinkSpeed = 500;
  ret->dragDelta = 6;
  ret->scrollBarSize = 15;

  // Make font
#ifdef HAVE_XFT_H
  ret->normalFont = dkFontNew(ret, "Sans,90");
#else
  ret->normalFont = dkFontNew(ret, "helvetica,90");
#endif

	/* Init colors */
	ret->borderColor = FXRGB(0, 0, 0);
	ret->baseColor = FXRGB(212, 208, 200);
	ret->hiliteColor = makeHiliteColor(ret->baseColor);
	ret->shadowColor = makeShadowColor(ret->baseColor);
	ret->backColor = FXRGB(255,255,255);
	ret->foreColor = FXRGB(0,0,0);
	ret->selforeColor = FXRGB(255,255,255);
	ret->selbackColor = FXRGB(10,36,106);
	ret->tipforeColor = FXRGB(0,0,0);
	ret->tipbackColor = FXRGB(255,255,225);
	ret->selMenuTextColor = FXRGB(255,255,255);
	ret->selMenuBackColor = FXRGB(10,36,106);

	return ret;
}

static void
dtk_app_opendisplay(App *app)
{
  if (app->display_opened == 1)
    return;

  /* What's going on */
  DKTRACE((100, "dkApp: opening display.\n"));

	DtkWindowHashNew();
	dtkDrvOpenDisplay(app, app->dpy);
#ifndef WIN32
    app->wmMotifHints = XInternAtom(app->display, "_MOTIF_WM_HINTS", 0);
#endif

  /* Clear sticky mod state */
  app->stickyMods = 0;

  /* Lock the global mutex */
  dkMutexLock(&app->appMutex);

  /* We have been initialized */
  app->display_opened = 1;
}

void dkAppInit(App *app, int argc, char *argv[])
{
  int i, j;
#ifdef WIN32
  struct dkFontDesc fontdesc;
#else
  char *d;
#endif

	/* Stub */
#ifndef WIN32
  /* Try locate display */
  if((d = getenv("DISPLAY")) != NULL) app->dpy = d;
#endif

  /* Parse out DK args */
  i = j = 1;
  while (j < argc) {

    /* Set trace level */
    if (strcmp(argv[j], "-tracelevel") == 0) {
      if (++j >= argc) {
        printf("dkAppInit: missing argument for -tracelevel.\n");
        exit(1);
      }
      dkTraceLevel = strtol(argv[j++], NULL, 10);
      continue;
    }

    /* Copy program arguments */
    argv[i++] = argv[j++];
  }

  /* Adjust argument count */
  argv[i] = NULL;
  argc = i;

  /* Remember arguments */
  app->appArgv = argv;
  app->appArgc = argc;

  // Log message
  DKTRACE((100,"%s::init\n", ((struct dkObject *)app)->meta->className));

#ifdef WIN32
  getSystemFont(&fontdesc);
  dkFontSetDesc(app->normalFont, &fontdesc);
#endif

  dtk_app_opendisplay(app);
}

void dkAppCreate(App *app)
{
  DKTRACE((100, "dkApp::create\n"));

  /* Create visuals */
  DtkCreateVisual(app->defaultVisual);

  /* Create default font */
  dkFontCreate(app->normalFont);

  /* Create stock cursors */
  FxCursorCreate(app->cursor[DEF_ARROW_CURSOR]);
  FxCursorCreate(app->cursor[DEF_TEXT_CURSOR]);

  /* Create the root window which will create all the other windows */
  app->root->create(app->root);
}

/* Schedule a future refresh; if we were in the middle of
 * one, we continue with the current cycle until we wrap
 * around to the current widget about to be updated. */
void
fxAppRefresh(App *app)
{
  if (app->refresher == NULL)
    app->refresher = app->root;

  app->refresherstop = app->refresher;
}

void
dkAppDel(struct dkApp *app)
{
  free(app);
}

int dkAppRun(App *app)
{
  struct fx_invocation inv;

  fx_invocation_setup(&inv, &app->invocation, MODAL_FOR_NONE, NULL);
  DKTRACE((100, "Start run\n"));
  while (!inv.done) {
    dtkDrvRunOneEvent(app, 1);
  }
  DKTRACE((100, "End run\n"));
  fx_invocation_destroy(&inv);
  return inv.code;
}

/* Break out of topmost event loop, closing all nested loops also */
void dkAppStop(struct dkApp *app, int value)
{
  struct fx_invocation* inv;
  for (inv = app->invocation; inv; inv = inv->upper) {
    inv->done = TRUE;
    inv->code = 0;
    if (inv->upper == NULL) {
      inv->code = value;
      return;
    }
  }
}

/* Add timeout, sorted by time */
void
fxAppAddTimeout(struct dkApp *app, struct dkObject *tgt, FXSelector sel, DKuint ms, void *ptr)
{
	DKlong milliseconds = 1000000;
	DKlong nsec = ms * milliseconds;
	struct fxTimer *t, **tt;

	for (tt = &app->timers; (t = *tt) != NULL; tt = &t->next) {
    /* If we already have this timeout message, update it */
		if (t->target == tgt && t->message == sel){ *tt = t->next; goto a; }
	}

	if (app->timerrecs) {
		t = app->timerrecs;
		app->timerrecs = t->next;
	} else {
		t = malloc(sizeof(struct fxTimer));
	}

a:
  t->data = ptr;
	t->target = tgt;
	t->due = dkThreadTime() + nsec;
	t->message = sel;

  /* Place the timer into the list after any items that are due before it */
  for (tt = &app->timers; *tt && ((*tt)->due < t->due); tt = &(*tt)->next);

	t->next = *tt;
	*tt = t;
}

/* Remove timeout identified by tgt and sel from the list */
void
fxAppRemoveTimeout(struct dkApp *app, struct dkObject *tgt, DKSelector sel)
{
	struct fxTimer *t, **tt;

	for (tt = &app->timers; (t = *tt) != NULL; tt = &t->next) {
		if (t->target == tgt && t->message == sel) {
			*tt = t->next; t->next = app->timerrecs; app->timerrecs = t;
			break;
		}
	}
}

/* Handle any outstanding timeouts */
void
fxAppHandleTimeouts(struct dkApp *app)
{
  DKlong now;
  struct fxTimer *t;

  now = dkThreadTime();
  while (app->timers) {
    if (now < app->timers->due) break;
    t = app->timers;
    app->timers = t->next;
    if (t->target && t->target->handle(t->target, (struct dkObject *)app, SEL_TIMEOUT, t->message, t->data))
      fxAppRefresh(app);
    t->next = app->timerrecs;
    app->timerrecs = t;
  }
}

/* Add chore to the END of the list */
void dkAppAddChore(struct dkApp *app, struct dkObject *tgt, DKSelector sel, void *ptr)
{
	struct dkChore *c, **cc;
	for (cc = &app->chores; (c = *cc) != NULL; cc = &c->next) {
		if (c->target == tgt && c->message == sel) {
			*cc = c->next; goto a;
		}
	}

	if (app->chorerecs) {
		c = app->chorerecs;
		app->chorerecs = c->next;
	} else {
		c = malloc(sizeof(struct dkChore));
	}
a:	c->data = ptr;
	c->target = tgt;
	c->message = sel;
	for (cc = &app->chores; *cc; cc = &(*cc)->next);
	c->next = NULL;
	*cc = c;
}

/* Remove chore identified by tgt and sel from the list */
void dkAppRemoveChore(struct dkApp *app, struct dkObject* tgt, DKSelector sel)
{
	struct dkChore *c, **cc;

	for (cc = &app->chores; (c = *cc) != NULL; cc = &c->next) {
		if (c->target == tgt && c->message == sel) {
			*cc = c->next; c->next = app->chorerecs; app->chorerecs = c;
			break;
		}
	}
}

/* Find window from root x, y, starting from given window */
struct dkWindow *dkAppFindWindowAt(struct dkApp *app, int rx, int ry, DKID window)
{
  if (app->display_opened) {
#ifndef WIN32
    Window rootwin, child;
    int wx, wy;

    rootwin = XDefaultRootWindow((Display *)app->display);
    if (!window) window = rootwin;
    while (1) {
      if (!XTranslateCoordinates((Display*)app->display, rootwin, window, rx, ry, &wx, &wy, &child)) return NULL;
      if (child == None) break;
      window = child;
    }
#else
    POINT point;
    point.x = rx;
    point.y = ry;
    window = WindowFromPoint(point);  /* FIXME this finds only enabled/visible windows */
#endif
    return DtkFindWindowWithId(window);
  }
  return NULL;
}

/*******************************************************************************/

/* Generate SEL_LEAVE */
void dkAppLeaveWindow(struct dkApp *app, struct dkWindow *window, struct dkWindow *ancestor)
{
  if (window && window->parent && window != ancestor) {
    app->event.type = SEL_LEAVE;
    dkWindowTranslateCoordinatesFrom(window, &app->event.win_x, &app->event.win_y, (struct dkWindow *)app->root, app->event.root_x, app->event.root_y);
    if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_LEAVE, 0, &app->event))
        fxAppRefresh(app);
    app->cursorWindow = window->parent;
    dkAppLeaveWindow(app, window->parent, ancestor);
  }
}

/* Generate SEL_ENTER */
void dkAppEnterWindow(struct dkApp *app, struct dkWindow *window, struct dkWindow *ancestor)
{
  if (window && window->parent && window != ancestor) {
    dkAppEnterWindow(app, window->parent, ancestor);
    app->event.type = SEL_ENTER;
    dkWindowTranslateCoordinatesFrom(window, &app->event.win_x, &app->event.win_y, (struct dkWindow *)app->root, app->event.root_x, app->event.root_y);
    app->cursorWindow = window;
    if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_ENTER, 0, &app->event))
        fxAppRefresh(app);
  }
}

/*******************************************************************************/


void dkAppBeep(struct dkApp *app)
{
  if(app->display_opened) {
    printf("Beep\n");
#ifndef WIN32
    XBell((Display*)app->display, 0);
#else
    MessageBeep(0);
#endif
  }
}

struct dkWindow *dkAppGetFocusWindow(struct dkApp *app)
{
  struct dkWindow *result = app->activeWindow;
  if (result) {
    while (result->focus) {
      result = result->focus;
    }
  }
  return result;
}

#ifdef WIN32

/* Translate to string on KeyPress */
void translateKeyEvent(struct dstr *str, unsigned int wParam, long lParam)
{
  DKnchar buffer[20];
  BYTE keystate[256];
  int n;

  GetKeyboardState(keystate);
  n = ToUnicodeEx(wParam, HIWORD(lParam) & (KF_EXTENDED | KF_UP | 0xFF), keystate, buffer, 20, 0, GetKeyboardLayout(0));
  if (n <= 0) n = 0;
  dstr_initn2(str, buffer, n);
}

static void getSystemFont(struct dkFontDesc *fontdesc)
{
  NONCLIENTMETRICS ncm;
  HDC hDC;

  ncm.cbSize = sizeof(NONCLIENTMETRICS);
  SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
#ifdef UNICODE
#error "no unicode"
#else
  strncpy(fontdesc->face, ncm.lfMenuFont.lfFaceName, sizeof(fontdesc->face) - 1);
#endif
  fontdesc->face[sizeof(fontdesc->face) - 1] = '\0';
  hDC = CreateCompatibleDC(NULL);
  fontdesc->size = -10 * MulDiv(ncm.lfMenuFont.lfHeight, 72, GetDeviceCaps(hDC, LOGPIXELSY));
  DeleteDC(hDC);
  fontdesc->weight = ncm.lfMenuFont.lfWeight / 10;
  fontdesc->slant = ncm.lfMenuFont.lfItalic ? dkFont_Italic : dkFont_Straight;
  fontdesc->encoding = FONTENCODING_DEFAULT;
  fontdesc->setwidth = 0;
  fontdesc->flags = 0;
}

LRESULT CALLBACK dtkDrvWndProc(DKID hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

/* Trick to find module handle of DTK library */
static HINSTANCE
GetOwnModuleHandle()
{
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery((const void*)GetOwnModuleHandle, &mbi, sizeof(mbi));
	return (HINSTANCE)mbi.AllocationBase;
}

LRESULT CALLBACK
dtkDrvWndProc(DKID hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  struct dkWindow *window, *ancestor;
  static HWND lastmovehwnd = 0;
  static LPARAM lastmovelParam = 0;
  POINT ptRoot, pt;
  DWORD dwpts;
  PAINTSTRUCT ps;
  DKuint state;
  struct dkApp *app;

  if (!IsWindow((HWND)hwnd))
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  /* Get window */
  window = DtkFindWindowWithId(hwnd);

  if (window == 0 && msg != WM_CREATE)
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  if (window != NULL)
    app = window->app;
  else
    app = NULL;

  /* Translate Win32 message to DTK message type */
  switch (msg) {

  /* Repaint event */
  case WM_PAINT:
    app->event.type = SEL_PAINT;
    app->event.synthetic = 1;  /* FIXME when is it non-synthetic ? */
    BeginPaint((HWND)hwnd, &ps);
    app->event.rect.x = (short)ps.rcPaint.left;
    app->event.rect.y = (short)ps.rcPaint.top;
    app->event.rect.w = (short)(ps.rcPaint.right - ps.rcPaint.left);
    app->event.rect.h = (short)(ps.rcPaint.bottom - ps.rcPaint.top);
    ((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_PAINT, 0, &app->event);
    EndPaint((HWND)hwnd, &ps);
    return 0;

  /* Keyboard */
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
  case WM_KEYUP:
  case WM_SYSKEYUP:
    DKTRACE((100, "%s virtkey=%c [0x%04x] hi=0x%04x rc=%d\n", msg == WM_KEYDOWN ? "WM_KEYDOWN": msg == WM_KEYUP ? "WM_KEYUP": msg == WM_SYSKEYDOWN ? "WM_SYSKEYDOWN" : "WM_SYSKEYUP", wParam, wParam, HIWORD(lParam), LOWORD(lParam)));
    app->event.type = ((msg == WM_KEYUP)||(msg == WM_SYSKEYUP)) ? SEL_KEYRELEASE : SEL_KEYPRESS;
    app->event.time = GetMessageTime();
    dwpts = GetMessagePos();
    app->event.root_x = pt.x = ((int)(short)LOWORD(dwpts));
    app->event.root_y = pt.y = ((int)(short)HIWORD(dwpts));
    ScreenToClient((HWND)hwnd, &pt);
    app->event.win_x = pt.x;
    app->event.win_y = pt.y;

    app->event.state = dkmodifierkeys();

    /* Translate to keysym */
    app->event.code = wkbMapKeyCode(msg, wParam, lParam); /* FIXME not all codes match with those of X11 */

    /* Translate to string on KeyPress */
    if (app->event.type == SEL_KEYPRESS) {
      translateKeyEvent(&app->event.text, wParam, lParam);
    }

    /* Clear string on KeyRelease */
    else {
      dstr_setlength(&app->event.text, 0);
    }

    /* Alt key seems to repeat. */

    /* Fix modifier state */
    if (app->event.type == SEL_KEYPRESS) {
      if (app->event.code == KEY_Shift_L) app->event.state |= SHIFTMASK;
      else if (app->event.code == KEY_Shift_R) app->event.state |= SHIFTMASK;
      else if (app->event.code == KEY_Control_L) app->event.state |= CONTROLMASK;
      else if (app->event.code == KEY_Control_R) app->event.state |= CONTROLMASK;
      else if (app->event.code == KEY_F13) app->event.state |= METAMASK; /* Key between Ctrl and Alt (on most keyboards) */
      else if (app->event.code == KEY_Alt_L) app->event.state |= ALTMASK;
      else if (app->event.code == KEY_Alt_R) app->event.state |= ALTMASK; /* FIXME do we need ALTGR flag instead/in addition? */
      else if (app->event.code == KEY_Num_Lock) app->event.state |= NUMLOCKMASK;
      else if (app->event.code == KEY_Caps_Lock) app->event.state |= CAPSLOCKMASK;
      else if (app->event.code == KEY_Scroll_Lock) app->event.state |= SCROLLLOCKMASK;
      else if (app->event.code == KEY_Super_L) app->event.state |= METAMASK;
      else if (app->event.code == KEY_Super_R) app->event.state |= METAMASK;
      else { app->stickyMods = app->event.state & (SHIFTMASK | CONTROLMASK | METAMASK | ALTMASK); }
    }
    else {
      if (app->event.code == KEY_Shift_L) app->event.state &= ~SHIFTMASK;
      else if (app->event.code == KEY_Shift_R) app->event.state &= ~SHIFTMASK;
      else if (app->event.code == KEY_Control_L) app->event.state &= ~CONTROLMASK;
      else if (app->event.code == KEY_Control_R) app->event.state &= ~CONTROLMASK;
      else if (app->event.code == KEY_F13) app->event.state &= ~METAMASK; /* Key between Ctrl and Alt (on most keyboards) */
      else if (app->event.code == KEY_Alt_L) app->event.state &= ~ALTMASK;
      else if (app->event.code == KEY_Alt_R) app->event.state &= ~ALTMASK; /* FIXME do we need ALTGR flag instead/in addition? */
      else if (app->event.code == KEY_Num_Lock) app->event.state &= ~NUMLOCKMASK;
      else if (app->event.code == KEY_Caps_Lock) app->event.state &= ~CAPSLOCKMASK;
      else if (app->event.code == KEY_Scroll_Lock) app->event.state &= ~SCROLLLOCKMASK;
      else if (app->event.code == KEY_Super_L) app->event.state &= ~METAMASK;
      else if (app->event.code == KEY_Super_R) app->event.state &= ~METAMASK;
      else { app->event.state |= app->stickyMods; app->stickyMods = 0; }
    }

    DKTRACE((100, "%s code=%04x state=%04x stickyModes=%04x text=\"%s\"\n", (app->event.type == SEL_KEYPRESS) ? "SEL_KEYPRESS" : "SEL_KEYRELEASE", app->event.code, app->event.state, app->stickyMods, app->event.text.str));

    /* Keyboard grabbed by specific window */
    if (app->keyboardGrabWindow) {
      if (((struct dkObject *)app->keyboardGrabWindow)->handle(app->keyboardGrabWindow, (struct dkObject *)app, app->event.type, 0, &app->event)) {
        fxAppRefresh(app);
      }
      return 0;
    }

    /* Remember window for later */
    if (app->event.type == SEL_KEYPRESS) app->keyWindow = app->activeWindow;

    /* Dispatch to key window */
    if (app->keyWindow) {

      /* FIXME doesSaveUnder test should go away */
      /* Dispatch if not in a modal loop or in a modal loop for a window containing the focus window */
      if (!app->invocation || app->invocation->modality == MODAL_FOR_NONE ||
        (app->invocation->window && dkWindowIsOwnerOf(app->invocation->window, app->keyWindow)) ||
        (DtkWindowGetShell(app->keyWindow)->doesSaveUnder())) {
        if (((struct dkObject *)app->keyWindow)->handle(app->keyWindow, (struct dkObject *)app, app->event.type, 0, &app->event))
          fxAppRefresh(app);
        return 0;
      }

      /* Beek if outside modal */
      // TODO: beep
    }
    return 0;

  /* The grab might be broken; in FOX, we ignore this!! */
  case WM_CANCELMODE:
    printf("Cancel mode\n");
    return 0;

  /* Capture changed */
  case WM_CAPTURECHANGED:
    return 0;

  /* Motion */
  case WM_MOUSEMOVE:
    app->event.time = GetMessageTime();
    pt.x = ptRoot.x = (int)((short)LOWORD(lParam));
    pt.y = ptRoot.y = (int)((short)HIWORD(lParam));
    ClientToScreen((HWND)hwnd, &ptRoot);
    app->event.root_x = ptRoot.x;
    app->event.root_y = ptRoot.y;
    app->event.state = dkmodifierkeys();

    /* Reset hover timer */
    fxAppAddTimeout(app, (struct dkObject *)app, ID_HOVER, 200, NULL);

    /* Set moved flag */
    if ((FXABS(app->event.root_x - app->event.rootclick_x) >= app->dragDelta) ||
        (FXABS(app->event.root_y - app->event.rootclick_y) >= app->dragDelta))
      app->event.moved = 1;

    /* Was grabbed */
    if (app->mouseGrabWindow) {

      /* Translate to grab window's coordinate system */
      dkWindowTranslateCoordinatesTo((struct dkWindow *)app->root, &app->event.win_x, &app->event.win_y, app->mouseGrabWindow, app->event.root_x, app->event.root_y);

      /* Moved out of/into rectangle of grabbed window */
      if (0 <= app->event.win_x &&
			    app->event.win_x < app->mouseGrabWindow->width &&
          0 <= app->event.win_y &&
			    app->event.win_y < app->mouseGrabWindow->height) {
        window = app->mouseGrabWindow;
      } else {
        window = app->mouseGrabWindow->parent;
      }
    }

    /* Switched windows */
    if (app->cursorWindow != window) {
      ancestor = dkWindowCommonAncestor(window, app->cursorWindow);
      app->event.code = CROSSINGNORMAL;
      dkAppLeaveWindow(app, app->cursorWindow, ancestor);
      dkAppEnterWindow(app, window, ancestor);
      fxAppRefresh(app);
    }

    /* Suppress spurious `tickling' motion events */
    if (hwnd == lastmovehwnd && lParam == lastmovelParam) return 0;

    /* Was still grabbed, but possibly new grab window! */
    if (app->mouseGrabWindow) {

      /* Translate to grab window's coordinate system */
      dkWindowTranslateCoordinatesTo((struct dkWindow *)app->root, &app->event.win_x, &app->event.win_y, app->mouseGrabWindow, app->event.root_x, app->event.root_y);

      /* Set event data */
      app->event.type = SEL_MOTION;
      app->event.code = 0;

      /* Dispatch to grab-window */
      if (((struct dkObject *)app->mouseGrabWindow)->handle(app->mouseGrabWindow, (struct dkObject *)app, SEL_MOTION, 0, &app->event))
        fxAppRefresh(app);
    }

    /* FIXME Does window still exist? */

    /* Was not grabbed */
    else if (!app->invocation || app->invocation->modality == MODAL_FOR_NONE ||
        (app->invocation->window && dkWindowIsOwnerOf(app->invocation->window, window)) ||
        (DtkWindowGetShell(window)->doesSaveUnder())) {
      /* FIXME doesSaveUnder test should go away */

      /* Set event data */
      app->event.type = SEL_MOTION;
      app->event.code = 0;
      app->event.win_x = pt.x;
      app->event.win_y = pt.y;

      /* Dispatch to window under cursor */
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_MOTION, 0, &app->event))
        fxAppRefresh(app);
    }

    /* Update most recent mouse position */
    app->event.last_x = pt.x;
    app->event.last_y = pt.y;

    /* Rember this for tickling test */
    lastmovehwnd = (HWND)hwnd;
    lastmovelParam = lParam;

    return 0;

  case WM_LBUTTONDOWN:
  case WM_MBUTTONDOWN:
  case WM_RBUTTONDOWN:
  case WM_LBUTTONUP:
  case WM_MBUTTONUP:
  case WM_RBUTTONUP:
    app->event.time = GetMessageTime();
    app->event.win_x = pt.x = (int)((short)LOWORD(lParam));
    app->event.win_y = pt.y = (int)((short)HIWORD(lParam));
    ClientToScreen((HWND)hwnd, &pt);
    app->event.root_x = pt.x;
    app->event.root_y = pt.y;
    app->event.state = dkmodifierkeys(); /* Get the state of the modifier keys and mouse buttons */
    if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN) {
      if (msg == WM_LBUTTONDOWN) { app->event.type = SEL_LEFTBUTTONPRESS; app->event.code = LEFTBUTTON; }
      if (msg == WM_MBUTTONDOWN) { app->event.type = SEL_MIDDLEBUTTONPRESS; app->event.code = MIDDLEBUTTON; }
      if (msg == WM_RBUTTONDOWN) { app->event.type = SEL_RIGHTBUTTONPRESS; app->event.code = RIGHTBUTTON; }
      if (!app->event.moved && (app->event.time - app->event.click_time < app->clickSpeed) && (app->event.code == (int)app->event.click_button)) {
        app->event.click_count++;
        app->event.click_time = app->event.time;
      } else {
        app->event.click_count = 1;
        app->event.click_x = app->event.win_x;
        app->event.click_y = app->event.win_y;
        app->event.rootclick_x = app->event.root_x;
        app->event.rootclick_y = app->event.root_y;
        app->event.click_button = app->event.code;
        app->event.click_time = app->event.time;
      }
      state = app->event.state & (LEFTBUTTONMASK | MIDDLEBUTTONMASK | RIGHTBUTTONMASK);
      if ((state == LEFTBUTTONMASK) || (state == MIDDLEBUTTONMASK) || (state == RIGHTBUTTONMASK)) app->event.moved = 0;
    } else {
      if (msg == WM_LBUTTONUP) { app->event.type = SEL_LEFTBUTTONRELEASE; app->event.code = LEFTBUTTON; }
      if (msg == WM_MBUTTONUP) { app->event.type = SEL_MIDDLEBUTTONRELEASE; app->event.code = MIDDLEBUTTON; }
      if (msg == WM_RBUTTONUP) { app->event.type = SEL_RIGHTBUTTONRELEASE; app->event.code = RIGHTBUTTON; }
    }
    if (app->mouseGrabWindow) {
      dkWindowTranslateCoordinatesTo(window, &app->event.win_x, &app->event.win_y, app->mouseGrabWindow, app->event.win_x, app->event.win_y);
      if (((struct dkObject *)app->mouseGrabWindow)->handle(app->mouseGrabWindow, (struct dkObject *)app, app->event.type, 0, &app->event))
        fxAppRefresh(app);
    }
    /* FIXME doesSaveUnder test should go away */
    else if (!app->invocation || app->invocation->modality == MODAL_FOR_NONE ||
        (app->invocation->window && dkWindowIsOwnerOf(app->invocation->window, window)) ||
        (DtkWindowGetShell(window)->doesSaveUnder())) {
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, app->event.type, 0, &app->event))
        fxAppRefresh(app);
    } else if (msg == WM_LBUTTONDOWN || msg == WM_MBUTTONDOWN || msg == WM_RBUTTONDOWN) {
      dkAppBeep(app);
    }
    app->event.last_x = app->event.win_x;
    app->event.last_y = app->event.win_y;
    return 0;

  /* Focus */
  case WM_SETFOCUS:
    SendMessage((HWND)window, WM_NCACTIVATE, 1, 123456); /* Suggestion from: Frank De prins <fdp@MCS.BE> */
  case WM_KILLFOCUS:
    window = DtkWindowGetShell(window);
    if (msg == WM_KILLFOCUS && app->activeWindow == window) {
      app->event.type = SEL_FOCUSOUT;
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_FOCUSOUT, 0, &app->event))
        fxAppRefresh(app);
      app->activeWindow = NULL;
    }
    if (msg == WM_SETFOCUS && app->activeWindow != window) {
      app->event.type = SEL_FOCUSIN;
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_FOCUSIN, 0, &app->event))
        fxAppRefresh(app);
      app->activeWindow = window;
    }
    return 0;

  case WM_CREATE:
		window = (struct dkWindow *)(((LPCREATESTRUCT)lParam)->lpCreateParams);
		DtkWindowHashInsert((void *)hwnd, window);
		app = window->app;
		app->event.type = SEL_CREATE;
		if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_CREATE, 0, &app->event))
			fxAppRefresh(app);
		return 0;

  /* Map or Unmap */
  case WM_SHOWWINDOW:
    if (wParam) {
      app->event.type = SEL_MAP;
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_MAP, 0, &app->event))
        fxAppRefresh(app);
    }
    else {
      app->event.type = SEL_UNMAP;
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_UNMAP, 0, &app->event))
        fxAppRefresh(app);
    }
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  /* Configure (size) */
  case WM_SIZE:
    if (wParam == SIZE_MINIMIZED) return 0;
    app->event.type = SEL_CONFIGURE;
    app->event.rect.x = window->xpos;
    app->event.rect.y = window->ypos;
    app->event.rect.w = LOWORD(lParam);
    app->event.rect.h = HIWORD(lParam);
    if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_CONFIGURE, 0, &app->event))
      fxAppRefresh(app);
    return 0;

  /* Configure (move) */
  case WM_MOVE:
    app->event.type = SEL_CONFIGURE;
    app->event.rect.x = (short)LOWORD(lParam);
    app->event.rect.y = (short)HIWORD(lParam);
    app->event.rect.w = window->width;
    app->event.rect.h = window->height;
    if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_CONFIGURE, 0, &app->event))
      fxAppRefresh(app);
    return 0;

  /* Configure position and size */
  case WM_WINDOWPOSCHANGED:
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  /* Change the cursor based on the window */
  case WM_SETCURSOR:
    if (!app->mouseGrabWindow && window->defaultCursor && (LOWORD(lParam) == HTCLIENT)) {
      SetCursor((HCURSOR)window->defaultCursor->xid);  /* Show default cursor */
      return 0;
    }
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  case WM_STYLECHANGING:
  case WM_SIZING:
  case WM_MOVING:
  case WM_ERASEBKGND:     /* Do nothing, erasing background causes flashing */
    return 0;

  case WM_ACTIVATE:
    if ((strcmp(window->classname, "DTKTopWindow") == 0) && app->activeWindow && app->activeWindow != window) {  /* Suggestion from: Frank De prins <fdp@MCS.BE> */
      SendMessage((HWND)app->activeWindow->xid, WM_NCACTIVATE, 0, 123456);
    }
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  case WM_NCACTIVATE:   /* Suggestion from: Frank De prins <fdp@MCS.BE> */
    if (lParam != 123456) wParam = 1;
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);

  case WM_ACTIVATEAPP:  /* Suggestion from: Frank De prins <fdp@MCS.BE> */
    SendMessage((HWND)hwnd, WM_NCACTIVATE, wParam, 123456);
    return DefWindowProc((HWND)hwnd, msg, wParam, lParam);
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

/* Mimics FOX's openDisplay */
int
dtkDrvOpenDisplay(App *app, char *dpyname)
{
  WNDCLASSEX wndclass;

  app->display = GetOwnModuleHandle();

  /* Standard stipples */
  app->stipples[STIPPLE_0] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_0]);
  app->stipples[STIPPLE_1] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_1]);
  app->stipples[STIPPLE_2] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_2]);
  app->stipples[STIPPLE_3] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_3]);
  app->stipples[STIPPLE_4] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_4]);
  app->stipples[STIPPLE_5] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_5]);
  app->stipples[STIPPLE_6] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_6]);
  app->stipples[STIPPLE_7] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_7]);
  app->stipples[STIPPLE_8] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_8]);
  app->stipples[STIPPLE_9] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_9]);
  app->stipples[STIPPLE_10] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_10]);
  app->stipples[STIPPLE_11] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_11]);
  app->stipples[STIPPLE_12] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_12]);
  app->stipples[STIPPLE_13] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_13]);
  app->stipples[STIPPLE_14] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_14]);
  app->stipples[STIPPLE_15] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_15]);
  app->stipples[STIPPLE_16] = CreateBitmap(8, 8, 1, 1, stipple_patterns[STIPPLE_16]);


  /* Register our child window classes */

	/* Child window */
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;  /* Set to 0 for bit_gravity */
	wndclass.lpfnWndProc = (WNDPROC)dtkDrvWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(struct dkWindow *);
	wndclass.hInstance = (HINSTANCE)app->display;
	wndclass.hIcon = NULL;
	wndclass.hIconSm = NULL;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = TEXT("DTKWindow");
	RegisterClassEx(&wndclass);

	/* Top window class */
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = (WNDPROC)dtkDrvWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(struct dkWindow *);
	wndclass.hInstance = (HINSTANCE)app->display;
	wndclass.hIcon = LoadIcon((HINSTANCE)app->display, IDI_APPLICATION);
	if (wndclass.hIcon == NULL)
		wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hIconSm = (HICON)LoadImage((HINSTANCE)app->display, IDI_APPLICATION, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	if (wndclass.hIconSm == NULL)
		wndclass.hIconSm = wndclass.hIcon;
	wndclass.hCursor = NULL;
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = TEXT("DTKTopWindow");
	RegisterClassEx(&wndclass);

	/* Someday we should add OpenGL */

	/* Popup window class */
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;  /* Do save-unders */
	wndclass.lpfnWndProc = (WNDPROC)dtkDrvWndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(struct dkWindow *);
	wndclass.hInstance = (HINSTANCE)app->display;
	wndclass.hIcon = NULL;
	wndclass.hIconSm = NULL;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = TEXT("DTKPopup");
	RegisterClassEx(&wndclass);

	/* This should prevent the Abort/Retry/Ignore message
	 * when switching to a drive w/no media mounted in it... */
	SetErrorMode(SEM_FAILCRITICALERRORS);

	return 1;
}

int
dtkDrvDispatchEvent(MSG *msg)
{
	/* This pushes the messages into the window proc */
	DispatchMessage(msg);
	/* FIXME should return TRUE only when handled in DTK */
	return 1;
}

int
dtkDrvGetNextEvent(App *app, MSG *msg, int blocking)
{
  int allinputs;
  DWORD signalled;

  /* Set to no-op just in case */
  msg->message = 0;

  /* Handle all past due timers */
  if (app->timers)
    dkAppHandleTimeouts(app);

  /* Check non-immediate signals that may have fired */
  /* TODO: Handle this */

	/* Peek for messages; this marks the message queue as unsignalled, i.e.
	 * MsgWaitForMultipleObjects would block even if there are unhandled
	 * events; the fix is to call MsgWaitForMultipleObjects only AFTER
	 * having ascertained that there are NO unhandled events queued up. */
	if (PeekMessage(msg, NULL, 0, 0, PM_REMOVE)) return 1;

	/* Poll to see if any waitable objects are signalled */
	allinputs = app->maxinput + 1;
	signalled = MsgWaitForMultipleObjects(allinputs, app->handles, FALSE,
	    0, QS_ALLINPUT);

	/* No objects were signalled, so perform background tasks now */
	if (signalled == WAIT_TIMEOUT) {

    /* Do our chores :-) */
    if (app->chores) {
      struct dkChore *c = app->chores;
      app->chores = c->next;
      if (c->target && c->target->handle(c->target, (struct dkObject *)app, SEL_CHORE, c->message, c->data))
        fxAppRefresh(app);
      c->next = app->chorerecs;
      app->chorerecs = c;
    }

    /* GUI updating:- walk the whole widget tree. */
    if (app->refresher) {
      ((struct dkObject *)app->refresher)->handle(app->refresher, (struct dkObject *)app, SEL_UPDATE, 0, NULL);
      if (app->refresher->first) {
        app->refresher = app->refresher->first;
      } else {
        while (app->refresher->parent) {
          if (app->refresher->next) {
            app->refresher = app->refresher->next;
            break;
          }
          app->refresher = app->refresher->parent;
        }
      }
      if (app->refresher != app->refresherstop) return 0;
      app->refresher = app->refresherstop = NULL;
    }

    /* There are more chores to do */
    if (app->chores) return 0;

    /* No updates or chores pending, so return at this point if
     * not blocking */
    if (!blocking) return 0;

		/* One more call to PeekMessage here because the preceding
		 * idle processing may have caused some more messages to be
		 * posted to our message queue:- a call to
		 * MsgWaitForMultipleObjects when there are events already
		 * in the queue would not immediately fall through but block
		 * until the next event comes in. */
		if (PeekMessage(msg, NULL, 0, 0, PM_REMOVE))
			return 1;

    /* If there are timers, block only a little time */
    allinputs = app->maxinput + 1;
    if (app->timers) {
      DKlong interval;
      DWORD delta;

      /* All that testing above may have taken some time... */
      interval = app->timers->due - dkThreadTime();

      /* Some timers are already due; do them right away! */
      if (interval <= 0) return 0;

      delta = (DWORD)(interval/1000000);

      /* Exit critical section */
      dkMutexUnlock(&app->appMutex);

      /* Now we will block */
      signalled = MsgWaitForMultipleObjects(allinputs,
          app->handles, FALSE, delta, QS_ALLINPUT);

      /* Enter critical section */
      dkMutexLock(&app->appMutex);


    }
    /* No timers, so block indefinitely */
    else {
      /* Exit critical section */
      dkMutexUnlock(&app->appMutex);

      signalled = MsgWaitForMultipleObjects(allinputs,
          app->handles, FALSE, INFINITE, QS_ALLINPUT);

      /* Enter critical section */
      dkMutexLock(&app->appMutex);
    }
	}

	/* Timed out, so do timeouts */
	if (signalled == WAIT_TIMEOUT) return 0;

	/* Got message from the GUI? */
	if (signalled != WAIT_OBJECT_0+allinputs) return 0;

	/* Get the event; this used to be GetMessage(msg, NULL, 0, 0),
	 * but for some reason, this occasionally blocks even though we have
	 * tried to make sure an event was indeed available.  The new code
	 * will always fall trhough, with an event if there is one, or without
	 * one if despite our efforts above there wasn't.  Thanks to Hodju
	 * Petri <phodju@cc.hut.fi> for this suggestion. */
	return PeekMessage(msg, NULL, 0, 0, PM_REMOVE) != 0;
}

int
dtkDrvRunOneEvent(App *app, int blocking)
{
	MSG msg;

	if (dtkDrvGetNextEvent(app, &msg, blocking)) {
		dtkDrvDispatchEvent(&msg);
		return 1;
	}
	return 0;
}

#else

/* Translate key code to utf8 text */
void translateKeyEvent(struct dstr *str, XEvent *event)
{
  char buffer[40]; KeySym sym; DKwchar w;
  XLookupString(&event->xkey, buffer, sizeof(buffer), &sym, NULL);
  w = dkkeysym2ucs(sym);
  dstr_initw2(str, &w, 1);
}

static int dtkDrvDispatchEvent(App *app, XEvent *ev);

/* Smart rectangle compositing algorithm */
void dkAppAddRepaint(struct dkApp *app, DKID win, int x, int y, int w, int h, DKbool synth)
{
  int px, py, pw, ph, hint, area;
  struct dkRepaint *r, **pr;

  hint = w * h;
  w += x;
  h += y;
  do {

    /* Find overlap with outstanding rectangles */
    for (r = app->repaints, pr = &app->repaints; r; pr = &r->next, r = r->next) {
      if (r->window == win) {
        /* Tentatively conglomerate rectangles */
        px = FXMIN(x, r->rect.x);
        py = FXMIN(y, r->rect.y);
        pw = FXMAX(w, r->rect.w);
        ph = FXMAX(h, r->rect.h);
        area = (pw - px) * (ph - py);

        /* New area MUCH bigger than sum; forget about it */
        if (area > (hint + r->hint) * 2) continue;

        /* Take old paintrect out of the list */
        *pr = r->next;
        r->next = app->repaintrecs;
        app->repaintrecs = r;

        /* New rectangle */
        synth |= r->synth;        /* Synthethic is preserved! */
        hint = area;
        x = px;
        y = py;
        w = pw;
        h = ph;
        break;
      }
    }
  } while (r);

  /* Get rectangle, recycled if possible */
  if (app->repaintrecs) {
    r = app->repaintrecs;
    app->repaintrecs = r->next;
  } else {
    r = malloc(sizeof(struct dkRepaint));
  }

  /* Fill it */
  r->window = win;
  r->rect.x = x;
  r->rect.y = y;
  r->rect.w = w;
  r->rect.h = h;
  r->hint = hint;
  r->synth = synth;
  r->next = NULL;
  *pr = r;
}

/* Remove repaints by dispatching them */
void dkAppRemoveRepaints(struct dkApp *app, DKID win, int x, int y, int w, int h)
{
  struct dkRepaint *r, **rr;
  XEvent ev;

  w += x;
  h += y;

  /* Flush the buffer and wait till the X server catches up;
   * resulting events, if any, are buffered in the client. */
  XSync((Display*)app->display, FALSE);

  /* Fish out the expose events and compound them */
  while (XCheckMaskEvent((Display*)app->display, ExposureMask, &ev)) {
    if (ev.xany.type == NoExpose) continue;
    dkAppAddRepaint(app, ev.xexpose.window, ev.xexpose.x, ev.xexpose.y, ev.xexpose.width, ev.xexpose.height, 0);
  }

  /* Then process events pertaining to window win and overlapping
   * with the given rectangle; other events are left in the queue. */
  rr = &app->repaints;
  while((r = *rr) != NULL) {
    if (!win || (win == r->window && x < r->rect.w && y < r->rect.h && r->rect.x < w && r->rect.y < h)) {
      *rr = r->next;
      ev.xany.type = Expose;
      ev.xexpose.window = r->window;
      ev.xexpose.x = r->rect.x;
      ev.xexpose.y = r->rect.y;
      ev.xexpose.width = r->rect.w - r->rect.x;
      ev.xexpose.height = r->rect.h-r->rect.y;
      r->next = app->repaintrecs;
      app->repaintrecs = r;
      dtkDrvDispatchEvent(app, &ev);
      continue;
    }
    rr = &r->next;
  }

  /* Flush the buffer again */
  XFlush((Display*)app->display);
}

/* Scroll repaint rectangles; some slight trickyness here:- the
 * rectangles don't just move, they stretch in the scroll direction
 * This means the original dirty area will remain part of the area to
 * be painted. */
void dkAppScrollRepaints(struct dkApp *app, DKID win, int dx, int dy)
{
  struct dkRepaint *r;
  for (r = app->repaints; r; r = r->next) {
    if (r->window == win) {
      if (dx > 0) r->rect.w += dx; else r->rect.x += dx;
      if (dy > 0) r->rect.h += dy; else r->rect.y += dy;
    }
  }
}

static int
xerrorhandler(Display *dpy, XErrorEvent *eev)
{
	printf("An error occured\n");
	return 1;
}

/* Fatal error (e.g. lost connection) */
static int
xfatalerrorhandler(Display *dpy)
{
	printf("X Fatal error.\n");
	return 1;
}

/* Mimics FOX's openDisplay */
int
dtkDrvOpenDisplay(App *app, char *dpyname)
{
#ifdef HAVE_XRANDR_H
	int errorbase;
#endif

	/* Set the error handler */
	XSetErrorHandler(xerrorhandler);

	/* Set fatal handler */
	XSetIOErrorHandler(xfatalerrorhandler);

	/* Revert to default */
	if (dpyname == NULL) dpyname = app->dpy;
	/* Open display */

	app->display = XOpenDisplay(dpyname);
	if(app->display == NULL)
		return 0;

	/* Check for X Rotation and Reflection support */
#ifdef HAVE_XRANDR_H
	if (XRRQueryExtension((Display*)app->display, &app->xrreventbase, &errorbase)) {
		XRRSelectInput((Display*)app->display, XDefaultRootWindow((Display*)app->display), True);
		DKTRACE((100, "X RandR available\n"));
	}
#endif

	app->wmState = XInternAtom((Display*)app->display, "WM_STATE", 0);

	/* Extended Window Manager support */
	app->wmNetState = XInternAtom((Display*)app->display, "_NET_WM_STATE", 0);

	/* XDND protocol awareness */
	app->xdndAware = (DKID)XInternAtom(app->display, "XdndAware", 0);

  /* Standard stipples */
  app->stipples[STIPPLE_0] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_0], 8, 8);
  app->stipples[STIPPLE_1] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_1], 8, 8);
  app->stipples[STIPPLE_2] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_2], 8, 8);
  app->stipples[STIPPLE_3] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_3], 8, 8);
  app->stipples[STIPPLE_4] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_4], 8, 8);
  app->stipples[STIPPLE_5] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_5], 8, 8);
  app->stipples[STIPPLE_6] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_6], 8, 8);
  app->stipples[STIPPLE_7] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_7], 8, 8);
  app->stipples[STIPPLE_8] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_8], 8, 8);
  app->stipples[STIPPLE_9] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_9], 8, 8);
  app->stipples[STIPPLE_10] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_10], 8, 8);
  app->stipples[STIPPLE_11] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_11], 8, 8);
  app->stipples[STIPPLE_12] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_12], 8, 8);
  app->stipples[STIPPLE_13] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_13], 8, 8);
  app->stipples[STIPPLE_14] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_14], 8, 8);
  app->stipples[STIPPLE_15] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_15], 8, 8);
  app->stipples[STIPPLE_16] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)stipple_patterns[STIPPLE_16], 8, 8);

  /* Hatch patterns */
  app->stipples[STIPPLE_HORZ] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)hor_bits, 24, 24);
  app->stipples[STIPPLE_VERT] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)ver_bits, 24, 24);
  app->stipples[STIPPLE_CROSS] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)cross_bits, 24, 24);
  app->stipples[STIPPLE_DIAG] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)diag_bits, 16, 16);
  app->stipples[STIPPLE_REVDIAG] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)revdiag_bits, 16, 16);
  app->stipples[STIPPLE_CROSSDIAG] = (DKID)XCreateBitmapFromData((Display*)app->display, XDefaultRootWindow((Display*)app->display), (char*)crossdiag_bits, 16, 16);

  return 0;
}

/* Get keysym; interprets the modifiers! */
static DKuint keysym(XEvent *event)
{
  KeySym sym = KEY_VoidSymbol;
  char buffer[40];
  XLookupString(&event->xkey, buffer, sizeof(buffer), &sym, NULL);
  return sym;
}

static int dtkDrvDispatchEvent(App *app, XEvent *ev)
{
  int        tmp_x, tmp_y;
  Window     tmp;
  struct dkWindow *window, *ancestor;

  /* Get window */
  window = DtkFindWindowWithId(ev->xany.window);

  /* Was one of our windows, so dispatch */
  if (window) {
    switch (ev->xany.type) {
    /* Repaint event */
    case GraphicsExpose:
    case Expose:
      app->event.type = SEL_PAINT;
      app->event.rect.x = ev->xexpose.x;
      app->event.rect.y = ev->xexpose.y;
      app->event.rect.w = ev->xexpose.width;
      app->event.rect.h = ev->xexpose.height;
      app->event.synthetic = ev->xexpose.send_event;
      ((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_PAINT, 0, &app->event);
    case NoExpose:
      return 1;

    /* Keymap Notify */
    case KeymapNotify:
      return 1;

    /* Keyboard */
    case KeyPress:
    case KeyRelease:
      app->event.type = SEL_KEYPRESS + ev->xkey.type - KeyPress;
      app->event.time = ev->xkey.time;
      app->event.win_x = ev->xkey.x;
      app->event.win_y = ev->xkey.y;
      app->event.root_x = ev->xkey.x_root;
      app->event.root_y = ev->xkey.y_root;

      /* Translate to keysym; must interpret modifiers! */
      app->event.code = keysym(ev);

      /* Translate to string on KeyPress */
      if (ev->xkey.type == KeyPress) {
        struct dkWindow *focus = dkAppGetFocusWindow(app);
          if (focus && focus->composeContext)
            printf("XXX: TODO!! composeContext\n");  //event.text=getFocusWindow()->getComposeContext()->translateEvent(ev);
          else
            translateKeyEvent(&app->event.text, ev);
      }
      /* Clear string on KeyRelease */
      else {
        dstr_setlength(&app->event.text, 0);
      }

      /* Mouse buttons and modifiers but no wheel buttons */
      app->event.state = ev->xkey.state &~(Button4Mask | Button5Mask);

      /* Fix modifier state */
      if (ev->xkey.type == KeyPress) {
        if (app->event.code == KEY_Shift_L) app->event.state |= SHIFTMASK;
        else if (app->event.code == KEY_Shift_R) app->event.state |= SHIFTMASK;
        else if (app->event.code == KEY_Control_L) app->event.state |= CONTROLMASK;
        else if (app->event.code == KEY_Control_R) app->event.state |= CONTROLMASK;
        else if (app->event.code == KEY_F13) app->event.state |= METAMASK;     /* Key between Ctrl and Alt (on most keyboards) */
        else if (app->event.code == KEY_Alt_L) app->event.state |= ALTMASK;
        else if (app->event.code == KEY_Alt_R) app->event.state |= ALTMASK;    /* FIXME do we need ALTGR flag instead/in addition? */
        else if (app->event.code == KEY_Num_Lock) app->event.state |= NUMLOCKMASK;
        else if (app->event.code == KEY_Caps_Lock) app->event.state |= CAPSLOCKMASK;
        else if (app->event.code == KEY_Scroll_Lock) app->event.state |= SCROLLLOCKMASK;
        else if (app->event.code == KEY_Super_L) app->event.state |= METAMASK;
        else if (app->event.code == KEY_Super_R) app->event.state |= METAMASK;
        else { app->stickyMods = app->event.state & (SHIFTMASK | CONTROLMASK | METAMASK | ALTMASK); }
      }
      else {
        if (app->event.code == KEY_Shift_L) app->event.state &= ~SHIFTMASK;
        else if (app->event.code == KEY_Shift_R) app->event.state &= ~SHIFTMASK;
        else if (app->event.code == KEY_Control_L) app->event.state &= ~CONTROLMASK;
        else if (app->event.code == KEY_Control_R) app->event.state &= ~CONTROLMASK;
        else if (app->event.code == KEY_F13) app->event.state &= ~METAMASK;    /* Key between Ctrl and Alt (on most keyboards) */
        else if (app->event.code == KEY_Alt_L) app->event.state &= ~ALTMASK;
        else if (app->event.code == KEY_Alt_R) app->event.state &= ~ALTMASK;   /* FIXME do we need ALTGR flag instead/in addition? */
        else if (app->event.code == KEY_Num_Lock) app->event.state &= ~NUMLOCKMASK;
        else if (app->event.code == KEY_Caps_Lock) app->event.state &= ~CAPSLOCKMASK;
        else if (app->event.code == KEY_Scroll_Lock) app->event.state &= ~SCROLLLOCKMASK;
        else if (app->event.code == KEY_Super_L) app->event.state &= ~METAMASK;
        else if (app->event.code == KEY_Super_R) app->event.state &= ~METAMASK;
        else { app->event.state |= app->stickyMods; app->stickyMods = 0; }
      }

      DKTRACE((100, "%s code=%04x state=%04x stickyMods=%04x text=\"%s\"\n", (app->event.type == SEL_KEYPRESS) ? "SEL_KEYPRESS" : "SEL_KEYRELEASE", app->event.code, app->event.state, app->stickyMods, app->event.text.str));

      /* Keyboard grabbed by specific window */
      if (app->keyboardGrabWindow) {
        if (((struct dkObject *)app->keyboardGrabWindow)->handle(app->keyboardGrabWindow, (struct dkObject *)app, app->event.type, 0, &app->event)) {
          fxAppRefresh(app);
        }
        return 1;
      }

      /* Remember window for later */
      if (ev->xkey.type == KeyPress) app->keyWindow = app->activeWindow;

      /* Dispatch to key window */
      if (app->keyWindow) {

        /* FIXME doesSaveUnder test should go away */
        /* Dispatch if not in a modal loop or in a modal loop for a window containing the focus window */
        if (!app->invocation || app->invocation->modality == MODAL_FOR_NONE ||
          (app->invocation->window && dkWindowIsOwnerOf(app->invocation->window, app->keyWindow)) ||
          (DtkWindowGetShell(app->keyWindow)->doesSaveUnder())) {
          if (((struct dkObject *)app->keyWindow)->handle(app->keyWindow, (struct dkObject *)app, app->event.type, 0, &app->event))
            fxAppRefresh(app);
          return 0;
        }

        /* Beep if outside modal */
        if (ev->xany.type == KeyPress) dkAppBeep(app);
      }
      return 1;

    /* Motion */
    case MotionNotify:
      app->event.type = SEL_MOTION;
      app->event.time = ev->xmotion.time;
      app->event.win_x = ev->xmotion.x;
      app->event.win_y = ev->xmotion.y;
      app->event.root_x = ev->xmotion.x_root;
      app->event.root_y = ev->xmotion.y_root;
      app->event.code = 0;

      /* Mouse buttons and modifiers but no wheel buttons */
      app->event.state = (ev->xmotion.state &~(Button4Mask|Button5Mask)) | app->stickyMods;

      /* Moved more that delta */
      if ((FXABS(app->event.root_x - app->event.rootclick_x) >= app->dragDelta) ||
          (FXABS(app->event.root_y - app->event.rootclick_y) >= app->dragDelta)) app->event.moved = 1;

      /* Dispatch to grab window */
      if (app->mouseGrabWindow) {
        dkWindowTranslateCoordinatesTo(window, &app->event.win_x, &app->event.win_y, app->mouseGrabWindow, app->event.win_x, app->event.win_y);
        if (((struct dkObject *)app->mouseGrabWindow)->handle(app->mouseGrabWindow, (struct dkObject *)app, SEL_MOTION, 0, &app->event))
          fxAppRefresh(app);
      }

      /* FIXME doesSaveUnder test should go away */
      /* Dispatch if inside model window */
      else if (!app->invocation || app->invocation->modality == MODAL_FOR_NONE ||
        (app->invocation->window && dkWindowIsOwnerOf(app->invocation->window, window)) ||
        (DtkWindowGetShell(window)->doesSaveUnder())) {

        if (((struct dkObject *)window)->handle(window, (struct dkObject *)app,
              SEL_MOTION, 0, &app->event))
          fxAppRefresh(app);
      }

      /* Remember last mouse */
      app->event.last_x = app->event.win_x;
      app->event.last_y = app->event.win_y;
      return 1;

    /* Button */
    case ButtonPress:
    case ButtonRelease:
      app->event.time = ev->xbutton.time;
      app->event.win_x = ev->xbutton.x;
      app->event.win_y = ev->xbutton.y;
      app->event.root_x = ev->xbutton.x_root;
      app->event.root_y = ev->xbutton.y_root;

      /* Mouse buttons and modifiers but no wheel buttons */
      app->event.state = (ev->xmotion.state &~(Button4Mask|Button5Mask)) | app->stickyMods;

      /* Mouse Wheel */
      if (ev->xbutton.button == Button4 || ev->xbutton.button == Button5) {
        /* TODO: XXX Finish mouse wheel */
        printf("MOUSEWHEEL!!\n");
        return 1;
      }

      /* Mouse Button */
      app->event.code = ev->xbutton.button;
      if (ev->xbutton.type == ButtonPress) {                               /* Mouse button press */
        if (ev->xbutton.button == Button1){ app->event.type = SEL_LEFTBUTTONPRESS; app->event.state |= LEFTBUTTONMASK; }
        if (ev->xbutton.button == Button2){ app->event.type = SEL_MIDDLEBUTTONPRESS; app->event.state |= MIDDLEBUTTONMASK; }
        if (ev->xbutton.button == Button3){ app->event.type = SEL_RIGHTBUTTONPRESS; app->event.state |= RIGHTBUTTONMASK; }
        if (!app->event.moved && (app->event.time - app->event.click_time < app->clickSpeed) && (app->event.code == (int)app->event.click_button)) {
          app->event.click_count++;
          app->event.click_time = app->event.time;
        } else {
          app->event.click_count = 1;
          app->event.click_x = app->event.win_x;
          app->event.click_y = app->event.win_y;
          app->event.rootclick_x = app->event.root_x;
          app->event.rootclick_y = app->event.root_y;
          app->event.click_button = app->event.code;
          app->event.click_time = app->event.time;
        }
        if (!(ev->xbutton.state & (Button1Mask | Button2Mask | Button3Mask))) app->event.moved = 0;
      } else {                                                           /* Mouse button release */
        if (ev->xbutton.button == Button1) { app->event.type = SEL_LEFTBUTTONRELEASE; app->event.state &= ~LEFTBUTTONMASK; }
        if (ev->xbutton.button == Button2) { app->event.type = SEL_MIDDLEBUTTONRELEASE; app->event.state &= ~MIDDLEBUTTONMASK; }
        if (ev->xbutton.button == Button3) { app->event.type = SEL_RIGHTBUTTONRELEASE; app->event.state &= ~RIGHTBUTTONMASK; }
      }

      /* Dispatch to grab window */
      if (app->mouseGrabWindow) {
        dkWindowTranslateCoordinatesTo(window, &app->event.win_x, &app->event.win_y, app->mouseGrabWindow, app->event.win_x, app->event.win_y);
        if (((struct dkObject *)app->mouseGrabWindow)->handle(app->mouseGrabWindow, (struct dkObject *)app, app->event.type, 0, &app->event))
          fxAppRefresh(app);
      }
      /* FIXME doesSaveUnder test should go away */
      /* Dispatch if inside model window */
      else if (!app->invocation || app->invocation->modality == MODAL_FOR_NONE ||
        (app->invocation->window && dkWindowIsOwnerOf(app->invocation->window, window)) ||
        (DtkWindowGetShell(window)->doesSaveUnder())) {
        if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, app->event.type, 0, &app->event))
          fxAppRefresh(app);
      }
      /* Beep if outside modal window */
      else {
        if (ev->xany.type == ButtonPress) dkAppBeep(app);
      }

      /* Remember last mouse */
      app->event.last_x = app->event.win_x;
      app->event.last_y = app->event.win_y;
      return 1;

    /* Crossing */
    case EnterNotify:
      app->event.time = ev->xcrossing.time;
      if (app->cursorWindow != window) {
        if (ev->xcrossing.mode == NotifyGrab || ev->xcrossing.mode == NotifyUngrab ||
          (ev->xcrossing.mode == NotifyNormal && ev->xcrossing.detail != NotifyInferior)) {
          ancestor = dkWindowCommonAncestor(window, app->cursorWindow);
          app->event.root_x = ev->xcrossing.x_root;
          app->event.root_y = ev->xcrossing.y_root;
          app->event.code = ev->xcrossing.mode;
          dkAppLeaveWindow(app, app->cursorWindow, ancestor);
          dkAppEnterWindow(app, window, ancestor);
        }
      }
      return 1;

    /* Crossing */
    case LeaveNotify:
      app->event.time = ev->xcrossing.time;
      if (app->cursorWindow == window) {
        if (ev->xcrossing.mode == NotifyGrab || ev->xcrossing.mode == NotifyUngrab ||
          (ev->xcrossing.mode == NotifyNormal && ev->xcrossing.detail != NotifyInferior)) {
          app->event.root_x = ev->xcrossing.x_root;
          app->event.root_y = ev->xcrossing.y_root;
          app->event.code = ev->xcrossing.mode;
          //FXASSERT(cursorWindow==window);
          dkAppLeaveWindow(app, window, window->parent);
        }
      }
      return 1;

    /* Focus change on shell window */
    case FocusIn:
    case FocusOut:
      window = DtkWindowGetShell(window);
      if (ev->xfocus.type == FocusOut && app->activeWindow == window) {
        app->event.type = SEL_FOCUSOUT;
        if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_FOCUSOUT, 0, &app->event))
          fxAppRefresh(app);
        app->activeWindow = NULL;
      }
      if (ev->xfocus.type == FocusIn && app->activeWindow != window) {
        app->event.type = SEL_FOCUSIN;
        if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_FOCUSIN, 0, &app->event))
          fxAppRefresh(app);
        app->activeWindow = window;
      }
      return 1;

    /* Map */
    case MapNotify:
      app->event.type = SEL_MAP;
      /* Only FXPopup handles MapNotify which we haven't built yet */
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_MAP, 0, &app->event))
        fxAppRefresh(app);
      return 1;

		/* Property change */
		case PropertyNotify:
			app->event.time = ev->xproperty.time;

			/* Update window position after minimize/maximize/restore whatever */
			if (ev->xproperty.atom == app->wmState || ev->xproperty.atom == app->wmNetState) {
				app->event.type = SEL_CONFIGURE;
				XTranslateCoordinates((Display*)app->display, ev->xproperty.window,
						XDefaultRootWindow((Display*)app->display), 0, 0,
						&tmp_x, &tmp_y, &tmp);
				app->event.rect.x = tmp_x;
				app->event.rect.y = tmp_y;
				app->event.rect.w = window->width;
				app->event.rect.h = window->height;
				app->event.synthetic = ev->xproperty.send_event;
				if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_CONFIGURE, 0, &app->event))
					fxAppRefresh(window->app);
			}
			return 1;

    case CreateNotify:
      app->event.type = SEL_CREATE;
      if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_CREATE, 0, &app->event)) fxAppRefresh(app);
      return 1;

		case ConfigureNotify:
			app->event.type = SEL_CONFIGURE;
			/* According to the ICCCM, if its synthetic, the coordinates are relative
			 * to root window; otherwise, they're relative to the parent; so we use
			 * the old coordinates if its not a synthetic configure notify */
			if (DtkWindowGetShell(window) == window && !ev->xconfigure.send_event) {
				ev->xconfigure.x = window->xpos;
				ev->xconfigure.y = window->ypos;
			}
			app->event.rect.x = ev->xconfigure.x;
			app->event.rect.y = ev->xconfigure.y;
			app->event.rect.w = ev->xconfigure.width;
			app->event.rect.h = ev->xconfigure.height;
			app->event.synthetic = ev->xconfigure.send_event;
			if (((struct dkObject *)window)->handle(window, (struct dkObject *)app, SEL_CONFIGURE, 0, &app->event))
				fxAppRefresh(window->app);
			return 1;
    /* Client message */
    case ClientMessage:
      printf("ClientMessage\n");
      return 1;

    /* Keyboard mapping */
    case MappingNotify:
      printf("MappingNotify\n");
      if (ev->xmapping.request != MappingPointer) XRefreshKeyboardMapping(&ev->xmapping);
      return 1;

    /* Other events */
    default:
#ifdef HAVE_XRANDR_H
      if (ev->type == app->xrreventbase + RRScreenChangeNotify) {
        printf("TODO: r&r\n");
      }
#endif
      return 1;
    }
  }

  return 0;
}

int dtkDrvGetNextEvent(App *app, XEvent *ev, int blocking)
{
  XEvent e;

  /* Set to no-op just in case */
  ev->xany.type=0;

  /* Handle all past due timers */
  if (app->timers)
    fxAppHandleTimeouts(app);

  /* Check non-immediate signals that may have fired */
  /* TODO: Finish */

	/* Are there no events already queued up? */
	if (!app->display_opened || !XEventsQueued(app->display, QueuedAfterFlush)) {
		struct timeval delta;
		fd_set readfds;
		fd_set writefds;
		fd_set exceptfds;
		int    maxfds;
		int    nfds;

		/* Prepare fd's to check */
		maxfds = app->maxinput;
		readfds = *((fd_set*)app->r_fds);
		writefds = *((fd_set*)app->w_fds);
		exceptfds = *((fd_set*)app->e_fds);

		/* Add connection to display if its open */
		if (app->display_opened) {
			FD_SET(ConnectionNumber((Display*)app->display), &readfds);
			if (ConnectionNumber((Display*)app->display) > maxfds)
				maxfds = ConnectionNumber((Display*)app->display);
		}

		delta.tv_usec = 0;
		delta.tv_sec = 0;

		/* Do a quick poll for any ready events or inputs */
		nfds = select(maxfds + 1, &readfds, &writefds, &exceptfds, &delta);

    /* Nothing to do, so perform idle processing */
    if (nfds == 0) {

      /* Release the expose events */
      if (app->repaints) {
        struct dkRepaint *r = app->repaints;
        ev->xany.type = Expose;
        ev->xexpose.window = r->window;
        ev->xexpose.send_event = r->synth;
        ev->xexpose.x = r->rect.x;
        ev->xexpose.y = r->rect.y;
        ev->xexpose.width = r->rect.w - r->rect.x;
        ev->xexpose.height = r->rect.h - r->rect.y;
        app->repaints = r->next;
        r->next = app->repaintrecs;
        app->repaintrecs = r;
        return 1;
      }

			/* Do our chores :-) */
			if (app->chores) {
				struct dkChore *c = app->chores;
				app->chores = c->next;
				if (c->target && c->target->handle(c->target, (struct dkObject *)app, SEL_CHORE, c->message, c->data))
					fxAppRefresh(app);
				c->next = app->chorerecs;
				app->chorerecs = c;
			}

			/* GUI updating:- walk the whole widget tree. */
			if (app->refresher) {
				((struct dkObject *)app->refresher)->handle(app->refresher, (struct dkObject *)app, SEL_UPDATE, 0, NULL);
				if (app->refresher->first) {
					app->refresher = app->refresher->first;
				} else {
					while (app->refresher->parent) {
						if (app->refresher->next) {
							app->refresher = app->refresher->next;
							break;
						}
						app->refresher = app->refresher->parent;
					}
				}

				if (app->refresher != app->refresherstop) return 0;
				app->refresher = app->refresherstop = NULL;

			}

      /* There are more chores to do */
      if (app->chores) return 0;

      /* We're not blocking */
      if (!blocking) return 0;

			/* Now, block till timeout, i/o, or event */
			maxfds = app->maxinput;
			readfds = *((fd_set*)app->r_fds);
			writefds = *((fd_set*)app->w_fds);
			exceptfds = *((fd_set*)app->e_fds);

			/* Add connection to display if its open */
			if (app->display_opened) {
				FD_SET(ConnectionNumber((Display*)app->display), &readfds);
				if (ConnectionNumber((Display*)app->display) > maxfds)
					maxfds = ConnectionNumber((Display*)app->display);
			}

      /* If there are timers, we block only for a little while. */
      if (app->timers) {
        DKlong interval;

        /* All that testing above may have taken some time... */
        interval = app->timers->due - dkThreadTime();

        /* Some timers are already due; do them right away! */
        if (interval <= 0) return 0;

        /* Compute how long to wait */
        delta.tv_usec = (interval / 1000) % 1000000;
        delta.tv_sec = interval / 1000000000;

        /* Exit critical section */
        dkMutexUnlock(&app->appMutex);

        /* Block till timer or event or interrupt */
        nfds = select(maxfds + 1, &readfds, &writefds, &exceptfds, &delta);

        /* Enter critical section */
        dkMutexLock(&app->appMutex);
      }
      /* If no timers, we block till event or interrupt */
      else {
        /* Exit critical section */
        dkMutexUnlock(&app->appMutex);

        /* Block until something happens */
        nfds = select(maxfds + 1, &readfds, &writefds, &exceptfds, NULL);

        /* Enter critical section */
        dkMutexLock(&app->appMutex);
      }
    }

		/* Timed out or interrupted */
		if (nfds <= 0) {
			if (nfds < 0 && errno != EAGAIN && errno != EINTR) {
				printf("Application terminated: interrupt or lost connection errno=%d\n", errno);
			}
			return 0;
		}

		/* Any other file descriptors set? */
		if (app->maxinput >= 0) {
			/* We need to handle this */
			printf("We need to handle this\n");
		}

		/* If there is no event, we're done */
		if (!app->display_opened || !FD_ISSET(ConnectionNumber((Display*)app->display), &readfds) ||
		    !XEventsQueued((Display*)app->display, QueuedAfterReading))
			return 0;

	}

	/* Get an event */
	XNextEvent(app->display, ev);

	/* Compress some common events */
	/* Compress motion events */
	if (ev->xany.type == MotionNotify) {
		while (XPending((Display*)app->display)) {
			XPeekEvent((Display*)app->display, &e);
			if ((e.xany.type != MotionNotify) || (ev->xmotion.window != e.xmotion.window) ||
			    (ev->xmotion.state != e.xmotion.state)) break;
			XNextEvent((Display*)app->display, ev);
		}
	}

	/* Compress wheel events */
	else if ((ev->xany.type == ButtonPress) && (ev->xbutton.button == Button4 || ev->xbutton.button == Button5)) {
		int ticks = 1;
		while (XPending((Display*)app->display)) {
			XPeekEvent((Display*)app->display, &e);
			if ((e.xany.type != ButtonPress && e.xany.type != ButtonRelease) ||
			    (ev->xany.window != e.xany.window) || (ev->xbutton.button != e.xbutton.button)) break;
			ticks += (e.xany.type == ButtonPress);
			XNextEvent((Display*)app->display, ev);
		}
		ev->xbutton.subwindow = (Window)ticks;   // Stick it here for later
	}

	/* Compress configure events */
	else if (ev->xany.type == ConfigureNotify) {
		while (XCheckTypedWindowEvent((Display*)app->display, ev->xconfigure.window, ConfigureNotify, &e)) {
			ev->xconfigure.width = e.xconfigure.width;
			ev->xconfigure.height = e.xconfigure.height;
			if (e.xconfigure.send_event) {
				ev->xconfigure.x = e.xconfigure.x;
				ev->xconfigure.y = e.xconfigure.y;
			}
		}
	}

	/* Regular event */
	return 1;
}

int
dtkDrvRunOneEvent(App *app, int blocking)
{
	XEvent event;

	if (dtkDrvGetNextEvent(app, &event, blocking)) {
		dtkDrvDispatchEvent(app, &event);
		return 1;
	}
	return 0;
}
#endif
