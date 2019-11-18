/******************************************************************************
 *                                                                            *
 *                         L a b e l   W i d g e t                            *
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
 * $Id: FXLabel.h,v 1.31 2006/03/01 02:13:21 fox Exp $                        *
 *****************************************************************************/

#ifndef FX_LABEL_H
#define FX_LABEL_H

#include "fxapp.h"
#include "fxdefs.h"

#include "fxframe.h"
#include "fxfont.h"

/* Relationship options for icon-labels */
enum {
  ICON_UNDER_TEXT      = 0,			      /// Icon appears under text
  ICON_AFTER_TEXT      = 0x00080000,		      /// Icon appears after text (to its right)
  ICON_BEFORE_TEXT     = 0x00100000,		      /// Icon appears before text (to its left)
  ICON_ABOVE_TEXT      = 0x00200000,		      /// Icon appears above text
  ICON_BELOW_TEXT      = 0x00400000,		      /// Icon appears below text
  TEXT_OVER_ICON       = ICON_UNDER_TEXT,	      /// Same as ICON_UNDER_TEXT
  TEXT_AFTER_ICON      = ICON_BEFORE_TEXT,	      /// Same as ICON_BEFORE_TEXT
  TEXT_BEFORE_ICON     = ICON_AFTER_TEXT,	      /// Same as ICON_AFTER_TEXT
  TEXT_ABOVE_ICON      = ICON_BELOW_TEXT,	      /// Same as ICON_BELOW_TEXT
  TEXT_BELOW_ICON      = ICON_ABOVE_TEXT	      /// Same as ICON_ABOVE_TEXT
};


/* Normal way to show label */
enum {
  LABEL_NORMAL         = JUSTIFY_NORMAL | ICON_BEFORE_TEXT
};


/**
* A label widget can be used to place a text and/or icon for
* explanation purposes.  The text label may have an optional tooltip
* and/or help string.  Icon and label are placed relative to the widget
* using the justfication options, and relative to each other as determined
* by the icon relationship options.  A large number of arrangements is
* possible.
*/

/* Forward declarations */
struct dtkEvent;
struct dkDrawable;

/* Inherits from dkFrame */
struct dkLabel {
  struct dkFrame base;

	/* dkLabel fields */
	char *label;       // Text on the label
	struct dkFont *font;        // Label font
//  FXHotKey hotkey;      // Hotkey
  int    hotoff;      /* Offset in string */
	DKColor  textColor;   // Text color
//  FXString tip;         // Tooltip
//  FXString help;        // Help message
};

/* Constructor for a dkLabel */
struct dkLabel * dkLabelNew(struct dkWindow *p, char *title);
void dkLabelInit(struct dkLabel *pthis, struct dkWindow *p, char *title, DKuint opts);
long dkLabel_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

void dkLabelJustX(struct dkLabel *l, int *tx, int *ix, int tw, int iw);
void dkLabelJustY(struct dkLabel *l, int *ty, int *iy, int th, int ih);

int dkLabelWidth(struct dkLabel *l, char *text);
int dkLabelHeight(struct dkLabel *l, char *text);

void dkLabelDrawLabel(struct dkLabel *l, struct dtkDC *dc, char *text, int hot, int tx, int ty, int tw, int th);

#endif /* FX_LABEL_H */
