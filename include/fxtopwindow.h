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
 * $Id: FXTopWindow.h,v 1.62 2006/01/22 17:58:11 fox Exp $                    *
 *****************************************************************************/

#ifndef FX_TOPWINDOW_H
#define FX_TOPWINDOW_H

#include "fxapp.h"
#include "fxdefs.h"

#include "fxshell.h"

/// Title and border decorations
enum {
  DECOR_NONE        = 0,                                  /// Borderless window
  DECOR_TITLE       = 0x00020000,                         /// Window title
  DECOR_MINIMIZE    = 0x00040000,                         /// Minimize button
  DECOR_MAXIMIZE    = 0x00080000,                         /// Maximize button
  DECOR_CLOSE       = 0x00100000,                         /// Close button
  DECOR_BORDER      = 0x00200000,                         /// Border
  DECOR_SHRINKABLE  = 0x00400000,                         /// Window can become smaller
  DECOR_STRETCHABLE = 0x00800000,                         /// Window can become larger
  DECOR_RESIZE      = DECOR_SHRINKABLE|DECOR_STRETCHABLE, /// Resize handles
  DECOR_MENU        = 0x01000000,                         /// Window menu
  DECOR_ALL         = (DECOR_TITLE|DECOR_MINIMIZE|DECOR_MAXIMIZE|DECOR_CLOSE|DECOR_BORDER|DECOR_SHRINKABLE|DECOR_STRETCHABLE|DECOR_MENU)
};

/* Initial window placement */
enum {
  PLACEMENT_DEFAULT,    /* Place it at he default size and location */
  PLACEMENT_VISIBLE,    /* Place window to be fully visible */
  PLACEMENT_CURSOR,     /* Place it under the cursor position */
  PLACEMENT_OWNER,      /* Place it centered on its owner */
  PLACEMENT_SCREEN,     /* Place it centered on the screen */
  PLACEMENT_MAXIMIZED   /* Place it maximized on the screen size */
};

struct FXVisual;

/* Inherits from dkShell */
struct dkTopWindow {
  struct dkWindow base;

  /* dkTopWindow fields */
  char *title;
  int padtop;
  int padbottom;
  int padleft;
  int padright;
  int hspacing;                 /* Horizontal child spacing */
  int vspacing;                 /* Vertical child spacing */
};

/* Constructor for dkTopWindow */
void DtkTopWindowCtor(struct dkTopWindow *pthis, struct dkApp *app, char *title, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs);
void DtkCreateTopWindow(void *pthis);

void dkTopWindowLayout(struct dkWindow *w);

#endif /* FX_TOPWINDOW_H */
