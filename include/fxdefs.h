/******************************************************************************
 *                                                                            *
 *                  FOX Definitions, Types, and Macros                        *
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
 * $Id: fxdefs.h,v 1.178.2.2 2006/11/09 23:21:43 fox Exp $                    *
 *****************************************************************************/

#ifndef FXDEFS_H
#define FXDEFS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>

/******************************  Definitions  ********************************/

/* Truth values */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef MAYBE
#define MAYBE 2
#endif
#ifndef NULL
#define NULL 0
#endif

/* Streamable types; these are fixed size! */
typedef unsigned char          DKuchar;
typedef DKuchar                DKbool;

/******************************* Macros **********************************/

/* Absolute value */
#define FXABS(val) (((val)>=0)?(val):-(val))

/* Return the maximum of a or b */
#define FXMAX(a,b) (((a)>(b))?(a):(b))

/* Return the minimum of a or b */
#define FXMIN(a,b) (((a)>(b))?(b):(a))

/* Clamp value x to range [lo..hi] */
#define DKCLAMP(lo,x,hi) ((x)<(lo)?(lo):((x)>(hi)?(hi):(x)))

/* Test if character c is at the start of a utf8 sequence */
#define DKISUTF(c) (((c)&0xC0)!=0x80)

/* Get red value from RGBA color */
#define FXREDVAL(rgba)     ((unsigned char)(((rgba)>>24)&0xff))

/* Get green value from RGBA color */
#define FXGREENVAL(rgba)   ((unsigned char)(((rgba)>>16)&0xff))

/* Get blue value from RGBA color */
#define FXBLUEVAL(rgba)    ((unsigned char)(((rgba)>>8)&0xff))

/* Get alpha value from RGBA color */
#define FXALPHAVAL(rgba)   ((unsigned char)((rgba)&0xff))

/* Make RGB color */
#define FXRGB(r,g,b)       (((unsigned int)(unsigned char)(r)<<24) | ((unsigned int)(unsigned char)(g)<<16) | ((unsigned int)(unsigned char)(b)<<8) | 0x000000ff)

#ifdef WIN32
typedef unsigned int           DKwchar;
#if defined(_MSC_VER) && !defined(_NATIVE_WCHAR_T_DEFINED)
typedef unsigned short         DKnchar;
#elif defined(__WATCOM_INT64__)
typedef long char              DKnchar;
#else
typedef wchar_t                DKnchar;
#endif
#else
typedef wchar_t                DKwchar;
typedef unsigned short         DKnchar;
#endif

/* Integral types large enough to hold value of a pointer */
#if defined(_MSC_VER) && defined(_WIN64)
typedef __int64                Dival;
typedef unsigned __int64       Duval;
#else
typedef long                   Dival;
typedef unsigned long          Duval;
#endif

#if defined(__LP64__) || defined(_LP64) || (_MIPS_SZLONG == 64) || (__WORDSIZE == 64)
typedef unsigned long          DKulong;
typedef long                   DKlong;
#elif defined(_MSC_VER) || (defined(__BCPLUSPLUS__) && __BORLANDC__ > 0x500) || defined(__WATCOM_INT64__)
typedef unsigned __int64       DKulong;
typedef __int64                DKlong;
#elif defined(__GNUG__) || defined(__GNUC__) || defined(__SUNPRO_CC) || defined(__MWERKS__) || defined(__SC__) || defined(_LONGLONG)
typedef unsigned long long     DKulong;
typedef long long              DKlong;
#else
#error "DKlong and DKulong not defined for this architecture!"
#endif

/* Integral types large enough to hold value of a pointer */
#if defined(_MSC_VER) && defined(_WIN64)
typedef __int64                DKival;
typedef unsigned __int64       DKuval;
#else
typedef long                   DKival;
typedef unsigned long          DKuval;
#endif

/* Handle to something in server */
#ifndef WIN32
typedef unsigned long          DKID;
#else
typedef void*                  DKID;
#endif

typedef unsigned short         DKushort;
typedef unsigned int           DKuint;

/* RGBA pixel value */
typedef DKuint                 DKColor;

/* Drag type */
#ifndef WIN32
typedef DKID                   DKDragType;
#else
typedef DKushort               DKDragType;
#endif

/* Number of elements in a static array */
#define ARRAYNUMBER(array)  (sizeof(array)/sizeof(array[0]))

/* Make int out of two shorts */
#define MKUINT(l, h) ((((DKuint)(l))&0xffff) | (((DKuint)(h))<<16))

/* Make selector from message type and message id */
#define DKSEL(type,id) ((((DKuint)(id))&0xffff) | (((DKuint)(type))<<16))

/* Define one function */
#define FXMAPFUNC(type,key,func) {DKSEL(type, key), DKSEL(type, key), &func}

#ifndef NDEBUG
#define DKTRACE(arguments) dktrace arguments
#else
#define DKTRACE(arguments) ((void)0)
#endif

/* dtk typedefs */
typedef struct dkApp dkApp;
typedef struct dkApp App;
typedef struct dtkWindow dtkWindow;
typedef struct dkCursor dkCursor;

/* FOX Keyboard and Button states */
enum {
  SHIFTMASK        = 0x001,           /// Shift key is down
  CAPSLOCKMASK     = 0x002,           /// Caps Lock key is down
  CONTROLMASK      = 0x004,           /// Ctrl key is down
#ifdef __APPLE__
  ALTMASK          = 0x2000,          /// Alt key is down
  METAMASK         = 0x10,            /// Meta key is down
#else
  ALTMASK          = 0x008,           /// Alt key is down
  METAMASK         = 0x040,           /// Meta key is down
#endif
  NUMLOCKMASK      = 0x010,           /// Num Lock key is down
  SCROLLLOCKMASK   = 0x0E0,           /// Scroll Lock key is down (seems to vary)
  LEFTBUTTONMASK   = 0x100,           /// Left mouse button is down
  MIDDLEBUTTONMASK = 0x200,           /// Middle mouse button is down
  RIGHTBUTTONMASK  = 0x400            /// Right mouse button is down
};

/* DTK System Defined Selector Types */
enum DKSelType {
  SEL_NONE,
  SEL_KEYPRESS,                         /// Key pressed
  SEL_KEYRELEASE,                       /// Key released
  SEL_LEFTBUTTONPRESS,                  /// Left mouse button pressed
  SEL_LEFTBUTTONRELEASE,                /// Left mouse button released
  SEL_MIDDLEBUTTONPRESS,                /// Middle mouse button pressed
  SEL_MIDDLEBUTTONRELEASE,              /// Middle mouse button released
  SEL_RIGHTBUTTONPRESS,                 /// Right mouse button pressed
  SEL_RIGHTBUTTONRELEASE,               /// Right mouse button released
  SEL_MOTION,                           /// Mouse motion
  SEL_ENTER,                            /// Mouse entered window
  SEL_LEAVE,                            /// Mouse left window
  SEL_FOCUSIN,                          /// Focus into window
  SEL_FOCUSOUT,                         /// Focus out of window
  SEL_KEYMAP,
  SEL_UNGRABBED,                        /// Lost the grab (Windows)
  SEL_PAINT,                            /// Must repaint window
  SEL_CREATE,
  SEL_DESTROY,
  SEL_UNMAP,                            /// Window was hidden
  SEL_MAP,                              /// Window was shown
  SEL_CONFIGURE,                        /// Resize
  SEL_SELECTION_LOST,                   /// Widget lost selection
  SEL_SELECTION_GAINED,                 /// Widget gained selection
  SEL_SELECTION_REQUEST,                /// Inquire selection data
  SEL_RAISED,                           /// Window to top of stack
  SEL_LOWERED,                          /// Window to bottom of stack
  SEL_CLOSE,                            /// Close window
  SEL_DELETE,                           /// Delete window
  SEL_MINIMIZE,                         /// Iconified
  SEL_RESTORE,                          /// No longer iconified or maximized
  SEL_MAXIMIZE,                         /// Maximized
  SEL_UPDATE,                           /// GUI update
  SEL_COMMAND,                          /// GUI command
  SEL_CLICKED,                          /// Clicked
  SEL_DOUBLECLICKED,                    /// Double-clicked
  SEL_TRIPLECLICKED,                    /// Triple-clicked
  SEL_MOUSEWHEEL,                       /// Mouse wheel
  SEL_CHANGED,                          /// GUI has changed
  SEL_VERIFY,                           /// Verify change
  SEL_DESELECTED,                       /// Deselected
  SEL_SELECTED,                         /// Selected
  SEL_INSERTED,                         /// Inserted
  SEL_REPLACED,                         /// Replaced
  SEL_DELETED,                          /// Deleted
  SEL_OPENED,                           /// Opened
  SEL_CLOSED,                           /// Closed
  SEL_EXPANDED,                         /// Expanded
  SEL_COLLAPSED,                        /// Collapsed
  SEL_BEGINDRAG,                        /// Start a drag
  SEL_ENDDRAG,                          /// End a drag
  SEL_DRAGGED,                          /// Dragged
  SEL_LASSOED,                          /// Lassoed
  SEL_TIMEOUT,                          /// Timeout occurred
  SEL_SIGNAL,                           /// Signal received
  SEL_CLIPBOARD_LOST,                   /// Widget lost clipboard
  SEL_CLIPBOARD_GAINED,                 /// Widget gained clipboard
  SEL_CLIPBOARD_REQUEST,                /// Inquire clipboard data
  SEL_CHORE,                            /// Background chore
  SEL_FOCUS_SELF,                       /// Focus on widget itself
  SEL_FOCUS_RIGHT,                      /// Focus moved right
  SEL_FOCUS_LEFT,                       /// Focus moved left
  SEL_FOCUS_DOWN,                       /// Focus moved down
  SEL_FOCUS_UP,                         /// Focus moved up
  SEL_FOCUS_NEXT,                       /// Focus moved to next widget
  SEL_FOCUS_PREV,                       /// Focus moved to previous widget
  SEL_DND_ENTER,                        /// Drag action entering potential drop target
  SEL_DND_LEAVE,                        /// Drag action leaving potential drop target
  SEL_DND_DROP,                         /// Drop on drop target
  SEL_DND_MOTION,                       /// Drag position changed over potential drop target
  SEL_DND_REQUEST,                      /// Inquire drag and drop data
  SEL_IO_READ,                          /// Read activity on a pipe
  SEL_IO_WRITE,                         /// Write activity on a pipe
  SEL_IO_EXCEPT,                        /// Except activity on a pipe
  SEL_PICKED,                           /// Picked some location
  SEL_QUERY_TIP,                        /// Message inquiring about tooltip
  SEL_QUERY_HELP,                       /// Message inquiring about statusline help
  SEL_DOCKED,                           /// Toolbar docked
  SEL_FLOATED,                          /// Toolbar floated
  SEL_SESSION_NOTIFY,                   /// Session is about to close
  SEL_SESSION_CLOSED,                   /// Session is closed
  SEL_LAST
};

/* DTK Mouse buttons */
enum {
  LEFTBUTTON       = 1,
  MIDDLEBUTTON     = 2,
  RIGHTBUTTON      = 3
};

/* DTK window crossing modes */
enum {
  CROSSINGNORMAL,              /* Normal crossing event */
  CROSSINGGRAB,                /* Crossing due to mouse grab */
  CROSSINGUNGRAB               /* Crossing due to mouse ungrab */
};

/* Search modes for search/replace dialogs */
enum {
  SEARCH_FORWARD         = 0,         /* Search forward (default) */
  SEARCH_BACKWARD        = 1,         /* Search backward */
  SEARCH_NOWRAP          = 0,         /* Don't wrap (default) */
  SEARCH_WRAP            = 2,         /* Wrap around to start */
  SEARCH_EXACT           = 0,         /* Exact match (default) */
  SEARCH_IGNORECASE      = 4,         /* Ignore case */
  SEARCH_REGEX           = 8,         /* Regular expression match */
  SEARCH_PREFIX          = 16         /* Prefix of subject string */
};

//typedef long (*dkWndProc) (void *, struct dkObject *, DKSelector

/* Globals */

/* Error routine */
extern void dkerror(const char *format, ...);

/* Trace printout routine:- usually not called directly but called through DKTRACE */
extern void dktrace(unsigned int level, char *format, ...);

/* Controls tracing level */
extern unsigned int dkTraceLevel;

extern DKwchar dkucs2keysym(DKwchar ucs);
extern DKwchar dkkeysym2ucs(DKwchar sym);

#endif /* FXDEFS_H */
