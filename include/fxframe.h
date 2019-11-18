/******************************************************************************
 *                                                                            *
 *                    F r a m e   W i n d o w   W i d g e t                   *
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
 * $Id: FXFrame.h,v 1.24 2006/01/22 17:58:02 fox Exp $                        *
 *****************************************************************************/

#ifndef FX_FRAME_H
#define FX_FRAME_H

#include "fxapp.h"
#include "fxcursor.h"
#include "fxdc.h"
#include "fxdefs.h"

#include "fxwindow.h"

/* Justification modes used by certain subclasses */
enum {
  JUSTIFY_NORMAL       = 0,			      /// Default justification is centered text
  JUSTIFY_CENTER_X     = 0,			      /// Contents centered horizontally
  JUSTIFY_LEFT         = 0x00008000,		      /// Contents left-justified
  JUSTIFY_RIGHT        = 0x00010000,		      /// Contents right-justified
  JUSTIFY_HZ_APART     = JUSTIFY_LEFT|JUSTIFY_RIGHT,  /// Combination of JUSTIFY_LEFT & JUSTIFY_RIGHT
  JUSTIFY_CENTER_Y     = 0,			      /// Contents centered vertically
  JUSTIFY_TOP          = 0x00020000,		      /// Contents aligned with label top
  JUSTIFY_BOTTOM       = 0x00040000,		      /// Contents aligned with label bottom
  JUSTIFY_VT_APART     = JUSTIFY_TOP|JUSTIFY_BOTTOM   /// Combination of JUSTIFY_TOP & JUSTIFY_BOTTOM
};


/* Default padding */
enum { DEFAULT_PAD = 2 };


/**
 * The Frame widget provides borders around some contents. Borders may be raised, sunken,
 * thick, ridged or etched.  They can also be turned off completely.
 * In addition, a certain amount of padding may be specified between the contents of
 * the widget and the borders.  The contents may be justified inside the widget using the
 * justification options.
 * The Frame widget is sometimes used by itself as a place holder, but most often is used
 * as a convenient base class for simple controls.
 */

/* Forward declarations */
struct dtkEvent;
struct dkDrawable;

/* Inherits from dkWindow */
struct dkFrame {
  struct dkWindow base;

	/* dkFrame fields */
	DKColor baseColor;    /* Base color */
	DKColor hiliteColor;  /* Highlight color */
	DKColor shadowColor;  /* Shadow color */
	DKColor borderColor;  // Border color */
	int     padtop;       /* Top padding */
	int     padbottom;    /* Bottom padding */
	int     padleft;      /* Left padding */
	int     padright;     /* right padding */
	int     border;       /* Border size */
};

/* Constructor for a dtkFrame */
struct dkFrame * dkFrameNew(struct dkWindow *p, DKuint opts);
void DtkFrameInit(struct dkFrame *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb);
void dkFrameCreate(void *pthis);
void dkFrameDrawFrame(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h);
long dkFrame_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

/* Drawing styles */
void dkFrameDrawDoubleRaisedRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h);
void dkFrameDrawRaisedRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h);
void dkFrameDrawSunkenRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h);
void dkFrameDrawDoubleSunkenRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h);

#endif /* FX_FRAME_H */
