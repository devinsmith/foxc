/******************************************************************************
 *                                                                            *
 *            D e v i c e   C o n t e x t   B a s e   C l a s s               *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 1999,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: FXDC.h,v 1.37 2006/01/22 17:58:00 fox Exp $                           *
 *****************************************************************************/

#ifndef FX_DC_H
#define FX_DC_H

#include "fxapp.h"
#include "fxdefs.h"
#include "fxpoint.h"

#include "fxwindow.h"

/** Drawing (BITBLT) functions */
enum DKFunction {
  BLT_CLR,                        /// D := 0
  BLT_SRC_AND_DST,                /// D := S & D
  BLT_SRC_AND_NOT_DST,            /// D := S & ~D
  BLT_SRC,                        /// D := S
  BLT_NOT_SRC_AND_DST,            /// D := ~S & D
  BLT_DST,                        /// D := D
  BLT_SRC_XOR_DST,                /// D := S ^ D
  BLT_SRC_OR_DST,                 /// D := S | D
  BLT_NOT_SRC_AND_NOT_DST,        /// D := ~S & ~D  ==  D := ~(S | D)
  BLT_NOT_SRC_XOR_DST,            /// D := ~S ^ D
  BLT_NOT_DST,                    /// D := ~D
  BLT_SRC_OR_NOT_DST,             /// D := S | ~D
  BLT_NOT_SRC,                    /// D := ~S
  BLT_NOT_SRC_OR_DST,             /// D := ~S | D
  BLT_NOT_SRC_OR_NOT_DST,         /// D := ~S | ~D  ==  ~(S & D)
  BLT_SET                         /// D := 1
};

/* Stipple/dither patterns */
enum DKStipplePattern {
  STIPPLE_0         = 0,
  STIPPLE_NONE      = 0,
  STIPPLE_BLACK     = 0,            /// All ones
  STIPPLE_1         = 1,
  STIPPLE_2         = 2,
  STIPPLE_3         = 3,
  STIPPLE_4         = 4,
  STIPPLE_5         = 5,
  STIPPLE_6         = 6,
  STIPPLE_7         = 7,
  STIPPLE_8         = 8,
  STIPPLE_GRAY      = 8,            /// 50% gray
  STIPPLE_9         = 9,
  STIPPLE_10        = 10,
  STIPPLE_11        = 11,
  STIPPLE_12        = 12,
  STIPPLE_13        = 13,
  STIPPLE_14        = 14,
  STIPPLE_15        = 15,
  STIPPLE_16        = 16,
  STIPPLE_WHITE     = 16,           /// All zeroes
  STIPPLE_HORZ      = 17,           /// Horizontal hatch pattern
  STIPPLE_VERT      = 18,           /// Vertical hatch pattern
  STIPPLE_CROSS     = 19,           /// Cross-hatch pattern
  STIPPLE_DIAG      = 20,           /// Diagonal // hatch pattern
  STIPPLE_REVDIAG   = 21,           /// Reverse diagonal \\ hatch pattern
  STIPPLE_CROSSDIAG = 22            /// Cross-diagonal hatch pattern
};

/* Line Styles */
enum DKLineStyle {
  LINE_SOLID,           /* Solid Lines */
  LINE_ONOFF_DASH,      /* On-off dashed lines */
  LINE_DOUBLE_DASH      /* Double dashed lines */
};

/* Line Cap Styles */
enum DKCapStyle {
  CAP_NOT_LAST,         /* Don't include last end cap */
  CAP_BUTT,             /* Butting line end caps */
  CAP_ROUND,            /* Round line end caps */
  CAP_PROJECTING        /* Projecting line end caps */
};

/* Line Join Styles */
enum DKJoinStyle {
  JOIN_MITER,           /* Mitered or pointy joints */
  JOIN_ROUND,           /* Round line joints */
  JOIN_BEVEL            /* Beveled or flat joints */
};

/* Fill Styles */
enum DKFillStyle {
  FILL_SOLID,           /* Fill with solid color */
  FILL_TILED,           /* Fill with tiled bitmap */
  FILL_STIPPLED,        /* Fill where stipple mask is 1 */
  FILL_OPAQUESTIPPLED   /* Fill with foreground where mask is 1, background otherwise */
};

/// Fill Rules
enum FXFillRule {
  RULE_EVEN_ODD,                  /// Even odd polygon filling
  RULE_WINDING                    /// Winding rule polygon filling
};

struct dtkDC {
  /* From DC */
  struct dkApp *app;               /* Application */
  void          *ctx;              /* Context handle */
  struct dkFont *font;             /* Drawing Font */
  enum DKStipplePattern pattern;   /* Stipple patern */
  void *stipple;                   /* This needs to be changed to a dkBitmap */
  struct dtkRectangle clip;        /* Clip rectangle */
  DKColor fg;                      /* Foreground color */
  DKColor bg;                      /* Background color */
  DKuint width;                    /* Line width */
  enum DKCapStyle cap;             /* Line cap style */
  enum DKJoinStyle join;           /* Line join style */
  enum DKLineStyle style;          /* Line style */

  /* DCWindow */
  struct dkWindow *surface;        /* Drawable surface */
  FXVisual *visual;               /* Visual of drawable */
  struct dtkRectangle rect;
  unsigned long   devfg;        /* Device foreground pixel value */
  unsigned long   devbg;        /* Device background pixel value */
  enum DKFillStyle fill;        /* Fill style */
  enum DKFunction  rop;         /* RasterOp */
  int tx;                       /* Tile dx */
  int ty;                       /* Tile dy */

#ifndef WIN32
	unsigned int    flags;        /* GC Flags */
	void           *xftDraw;      /* Hook used only for XFT support */
#else
	DKID            oldpalette;
	DKID            oldbrush;
	DKID            oldpen;
	int             needsNewBrush;
	int             needsNewPen;
	int             needsPath;
	int             needsClipReset;
#endif
};

void dkDCWindowSetFont(struct dtkDC *dc, struct dkFont *fnt);
void dkDCDrawText(struct dtkDC *dc, int x, int y, char *string, DKuint length);
void dkDCDrawFocusRectangle(struct dtkDC *dc, int x, int y, int w, int h);
void dkDCSetup(struct dtkDC *dc, struct dkWindow *w);
void dkDCSetClipRectangle(struct dtkDC *dc, int x, int y, int w, int h);
void dtkDrvDCEventSetup(struct dtkDC *dc, struct dkWindow *w, struct dkEvent *ev);
void dkDCEnd(struct dtkDC *dc);
void dtkDrvDCSetForeground(struct dtkDC *dc, unsigned int color);
void dtkDrvDCFillRectangle(struct dtkDC *dc, int x, int y, int width, int height);
void dkDCDrawRectangle(struct dtkDC *dc, int x, int y, int width, int height);
void dkDCFillPolygon(struct dtkDC *dc, struct dkPoint *points, DKuint npoints);
void dkDC_setStipplePat(struct dtkDC *dc, enum DKStipplePattern pat, int dx, int dy);
void dkDC_setFillStyle(struct dtkDC *dc, enum DKFillStyle fillstyle);
void dkDCSetBackground(struct dtkDC *dc, DKColor clr);

#endif /* FX_DC_H */
