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
 * $Id: FXDC.cpp,v 1.38 2006/01/22 17:58:21 fox Exp $                         *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifndef WIN32
#include "config.h"
#endif

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#if defined(HAVE_XFT_H)
#include <X11/Xft/Xft.h>
//#include <fontconfig.h>
#endif
#endif

#include "fxapp.h"
#include "fxdc.h"
#include "fxdrv.h"
#include "fxfont.h"
#include "fxstring.h"
#include "fxpoint.h"

static void dkDCInit(struct dtkDC *dc, struct dkApp *app)
{
  /* DC constructor */
  dc->app = app;
  dc->ctx = NULL;
  dc->tx = 0;
  dc->ty = 0;
  dc->fg = 0;
  dc->width = 0;
  dc->cap = CAP_BUTT;
  dc->join = JOIN_MITER;
  dc->style = LINE_SOLID;
  dc->fill = FILL_SOLID;
  dc->rop = BLT_SRC;
}

#ifdef WIN32
/* Begin locks in a drawable surface */
static void
dtkDrvDCBegin(struct dtkDC *dc, struct dkWindow *w)
{
	LOGBRUSH lb;

	if (!w) { printf("FXDCWindow::begin: NULL drawable.\n"); }
	if (!w->xid) { printf("FXDCWindow::begin: drawable not created yet.\n"); }

	dc->surface = w;  /* Careful - surface->wid can be HWND or HBITMAP depending on the window */
	dc->visual = w->visual;
	/* TODO: XXX: This call to GetDC below should be a virtual call, but it will require us to change to
	 * a dkDrawable. */
//	dc->ctx = dc->visual->gc;
	dc->ctx = GetDC((HWND)w->xid);
	dc->rect.x = dc->clip.x = 0;
	dc->rect.y = dc->clip.y = 0;
	dc->rect.w = dc->clip.w = w->width;
	dc->rect.h = dc->clip.h = w->height;

	/* Select and realize palette, if necessary */
	if (dc->visual->colormap) {
		dc->oldpalette = SelectPalette((HDC)dc->ctx,
		    (HPALETTE)dc->visual->colormap, 0);
		RealizePalette((HDC)dc->ctx);
	}

	dc->devfg = ~0;
	dc->devbg = 0;

	/* Create our default pen (black, solid, one pixel wide) */
	lb.lbStyle = BS_SOLID;
	lb.lbColor = PALETTERGB(0, 0, 0);
	lb.lbHatch = 0;
	dc->oldpen = SelectObject((HDC)dc->ctx, ExtCreatePen(PS_GEOMETRIC | PS_SOLID | PS_ENDCAP_FLAT | PS_JOIN_MITER, 1, &lb, 0, NULL));

	/* Create our default brush (solid white, for fills) */
	lb.lbStyle = BS_SOLID;
	lb.lbColor = PALETTERGB(255, 255, 255);
	lb.lbHatch = 0;
	dc->oldbrush = SelectObject((HDC)dc->ctx, CreateBrushIndirect(&lb));

	/* Text allignment */
	SetTextAlign((HDC)dc->ctx, TA_BASELINE | TA_LEFT);

	/* Polygon fill mode */
	SetPolyFillMode((HDC)dc->ctx, ALTERNATE);

	/* Reset flags */
	dc->needsNewBrush = FALSE;
	dc->needsNewPen = FALSE;
	dc->needsPath = FALSE;
	dc->needsClipReset = FALSE;
}

/* Construct for normal painting */
void dkDCSetup(struct dtkDC *dc, struct dkWindow *w)
{
  dkDCInit(dc, w->app);

  /* Standard */
  dc->oldpalette = NULL;
  dc->oldbrush = NULL;
  dc->oldpen = NULL;
  dc->needsNewBrush = FALSE;
  dc->needsNewPen = FALSE;
  dc->needsPath = FALSE;
  dc->needsClipReset = FALSE;
  dtkDrvDCBegin(dc, w);
}

/* Construct for expose event painting */
void
dtkDrvDCEventSetup(struct dtkDC *dc, struct dkWindow *w, struct dkEvent *event)
{
  HRGN hrgn;

  dkDCInit(dc, w->app);

	/* Standard */
	dc->oldpalette = NULL;
	dc->oldbrush = NULL;
	dc->oldpen = NULL;
	dc->needsNewBrush = FALSE;
	dc->needsNewPen = FALSE;
	dc->needsPath = FALSE;
	dc->needsClipReset = FALSE;
	dtkDrvDCBegin(dc, w);
	dc->rect.x = dc->clip.x = event->rect.x;
	dc->rect.y = dc->clip.y = event->rect.y;
	dc->rect.w = dc->clip.w = event->rect.w;
	dc->rect.h = dc->clip.h = event->rect.h;
	hrgn = CreateRectRgn(dc->clip.x, dc->clip.y, dc->clip.x + dc->clip.w,
	    dc->clip.y + dc->clip.h);
	SelectClipRgn((HDC)dc->ctx, hrgn);
	DeleteObject(hrgn);
}

/* End unlocks the drawable surface */
void
dkDCEnd(struct dtkDC *dc)
{
	if (dc->ctx) {
		DeleteObject(SelectObject((HDC)dc->ctx, dc->oldpen));
		DeleteObject(SelectObject((HDC)dc->ctx, dc->oldbrush));
		if (dc->visual->colormap) {
			SelectPalette((HDC)dc->ctx, (HPALETTE)dc->oldpalette, 0);
		}
		ReleaseDC((HWND)dc->surface->xid, (HDC)dc->ctx);
		//dc->surface

		dc->ctx = NULL;
	}
}

void
dtkDrvDCSetForeground(struct dtkDC *dc, unsigned int clr)
{
	if(!dc->surface) {
		printf("FXDCWindow::setForeground: DC not connected to drawable.\n"); 
	}
	dc->devfg = dtkDrvVisualGetPixel(dc->visual, clr);
	dc->needsNewPen = TRUE;
	dc->needsNewBrush = TRUE;
	SetTextColor((HDC)dc->ctx, dc->devfg);
	dc->fg = clr;
}

static void updatePen(struct dtkDC *dc)
{
  DWORD dashes[32];
  DWORD penstyle, i;
  LOGBRUSH lb;

  /* Setup brush of this pen */
  switch (dc->fill) {
    case FILL_SOLID:
      lb.lbStyle = BS_SOLID;
      lb.lbColor = dc->devfg;
      lb.lbHatch = 0;
      break;
    case FILL_TILED:
      lb.lbStyle = BS_SOLID;
      lb.lbColor = dc->devfg;
      lb.lbHatch = 0;
      break;
    case FILL_STIPPLED:
      printf("TODO TODO. Requires dkBitmap to be done\n");
      break;
    case FILL_OPAQUESTIPPLED:
      printf("TODO TODO2. Requires dkBitmap to be done\n");
      break;
  }

  penstyle = 0;

  /* Cap style */
  if (dc->cap == CAP_ROUND)
    penstyle |= PS_JOIN_ROUND;
  else if (dc->cap == CAP_PROJECTING)
    penstyle |=  PS_ENDCAP_SQUARE;
  else
    penstyle |= PS_ENDCAP_FLAT;

  /* Join style */
  if (dc->join == JOIN_MITER)
    penstyle |= PS_JOIN_MITER;
  else if (dc->join == JOIN_ROUND)
    penstyle |= PS_JOIN_ROUND;
  else
    penstyle |= PS_JOIN_BEVEL;

  /* Kind of pen */
  penstyle |= PS_GEOMETRIC;

  /* Line style */
  if (dc->style == LINE_SOLID) {
    penstyle |= PS_SOLID;
    DeleteObject(SelectObject((HDC)dc->ctx, ExtCreatePen(penstyle, dc->width, &lb, 0, NULL)));
  }

  if (dc->fill == FILL_STIPPLED) {
    SetBkMode((HDC)dc->ctx, TRANSPARENT);  /* Alas, only works for BS_HATCHED... */
  } else {
    SetBkMode((HDC)dc->ctx, OPAQUE);
  }

  dc->needsNewPen = FALSE;

}

static void updateBrush(struct dtkDC *dc)
{
  LOGBRUSH lb;
	switch (dc->fill) {
	case FILL_SOLID:
		lb.lbStyle = BS_SOLID;
		lb.lbColor = dc->devfg;
		lb.lbHatch = 0;
		DeleteObject(SelectObject((HDC)dc->ctx, CreateBrushIndirect(&lb)));
		break;
	case FILL_TILED:
		printf("Tiled fill\n");
		break;
	default:
		printf("Something else\n");
		break;
	}
	if (dc->fill == FILL_STIPPLED) {
		printf("TODO: Fill stippled\n");
	} else {
		SetBkMode((HDC)dc->ctx, OPAQUE);
	}
	dc->needsNewBrush = FALSE;
}

void
dtkDrvDCFillRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
	HPEN hpen;

	if(!dc->surface) {
		printf("FXDCWindow::fillRectangle: DC not connected to drawable.\n");
	}
	if (dc->needsNewBrush)
		updateBrush(dc);

	hpen = (HPEN)SelectObject((HDC)dc->ctx, GetStockObject(NULL_PEN));
	Rectangle((HDC)dc->ctx, x, y, x + w + 1, y + h + 1);
	SelectObject((HDC)dc->ctx, hpen);
}

/* Unfilled rectangle */
void dkDCDrawRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
  HBRUSH hbrush;

  if (!dc->surface) {
    dkerror("dkdc::drawRectangle: DC not connected to drawable.\n");
  }
  if (dc->needsNewPen)
    updatePen(dc);

  hbrush = (HBRUSH)SelectObject((HDC)dc->ctx, (HBRUSH)GetStockObject(NULL_BRUSH));
  Rectangle((HDC)dc->ctx, x, y, x + w + 1, y + h + 1);
  SelectObject((HDC)dc->ctx, hbrush);
}

void dkDCSetClipRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
  HRGN hrgn;

  if (!dc->surface) {
    dkerror("dkDCSetClipRectangle: DC not connected to drawable.\n");
  }
  dc->clip.x = DTKMAX(x, dc->rect.x);
  dc->clip.y = DTKMAX(y, dc->rect.y);
  dc->clip.w = DTKMIN(x + w, dc->rect.x + dc->rect.w) - dc->clip.x;
  dc->clip.h = DTKMIN(y + h, dc->rect.y + dc->rect.h) - dc->clip.y;
  if (dc->clip.w <= 0) dc->clip.w = 0;
  if (dc->clip.h <= 0) dc->clip.h = 0;

  hrgn = CreateRectRgn(dc->clip.x, dc->clip.y, dc->clip.x + dc->clip.w, dc->clip.y + dc->clip.h);
  SelectClipRgn((HDC)dc->ctx, hrgn);
  DeleteObject(hrgn);
}

void dkDCWindowSetFont(struct dtkDC *dc, struct dkFont *fnt)
{
	if (!dc->surface) {
		printf("%s: DC not connected to drawable.\n", __func__);
	}
	if (!fnt || !fnt->xid) {
		printf("%s: illegal or NULL font specified.\n", __func__);
	}
	SelectObject((HDC)dc->ctx, fnt->xid);
	dc->font = fnt;
}

/* Filled simple polygon */
void dkDCFillPolygon(struct dtkDC *dc, struct dkPoint *points, DKuint npoints)
{
  DKuint i;
  POINT pts[1360];      // Worst case limit according to MSDN
  if (!dc->surface) { dkerror("FXDCWindow::fillPolygon: DC not connected to drawable.\n"); }
  if (npoints >= 1360) { dkerror("FXDCWindow::fillPolygon: too many points.\n"); }
  if (dc->needsNewBrush) updateBrush(dc);
  HPEN hpen = (HPEN)SelectObject((HDC)dc->ctx, GetStockObject(NULL_PEN));
  for (i = 0; i < npoints; i++) {
    pts[i].x = points[i].x;
    pts[i].y = points[i].y;
  }
  Polygon((HDC)dc->ctx, pts, npoints);
  SelectObject((HDC)dc->ctx, hpen);
}

/* Draw string with base line starting at x, y */
void dkDCDrawText(struct dtkDC *dc, int x, int y, char *string, DKuint length)
{
	DKnchar sbuffer[4096];
	int count;
	int bkmode;

	if (!dc->surface) {
		printf("%s: DC not connected to drawable.\n", __func__);
	}
	if (!dc->font) {
		printf("%s: no font selected.\n", __func__);
	}

	count = dkString_utf2ncs(sbuffer, string, DTKMIN(length, 4096));
	bkmode = SetBkMode((HDC)dc->ctx, TRANSPARENT);
	TextOutW((HDC)dc->ctx, x, y, sbuffer, count);
	SetBkMode((HDC)dc->ctx, bkmode);
}

void dkDCDrawFocusRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
  HBRUSH hbrush;
  HBRUSH holdbrush;
  COLORREF coldback;
  COLORREF coldtext;

  if (!dc->surface) {
    dkerror("dkDC:drawFocusRectangle: DC not connected to drawable.\n");
  }

  hbrush = CreatePatternBrush((HBITMAP)dc->app->stipples[STIPPLE_GRAY]);
  holdbrush = (HBRUSH)SelectObject((HDC)dc->ctx, hbrush);
  coldback = SetBkColor((HDC)dc->ctx, RGB(255, 255, 255));
  coldtext = SetTextColor((HDC)dc->ctx, RGB(0, 0, 0));
  SetBrushOrgEx((HDC)dc->ctx, x, y, NULL);
  PatBlt((HDC)dc->ctx, x, y, w - 1, 1, PATINVERT);
  PatBlt((HDC)dc->ctx, x + w - 1, y, 1, h - 1, PATINVERT);
  PatBlt((HDC)dc->ctx, x + 1, y + h - 1, w - 1, 1, PATINVERT);
  PatBlt((HDC)dc->ctx, x, y + 1, 1, h - 1, PATINVERT);
  SelectObject((HDC)dc->ctx, holdbrush);
  DeleteObject(hbrush);
  SetBkColor((HDC)dc->ctx, coldback);
  SetTextColor((HDC)dc->ctx, coldtext);
  SetBrushOrgEx((HDC)dc->ctx, dc->tx, dc->ty, NULL);
}

void dkDC_setStipplePat(struct dtkDC *dc, enum DKStipplePattern pat, int dx, int dy)
{
  if (!dc->surface) {
    dkerror("dkDC:setStipplePat: DC not connected to drawable.\n");
  }
  dc->stipple = NULL;
  dc->pattern = pat;
  dc->needsNewBrush = TRUE;
  dc->needsNewPen = TRUE;
  dc->tx = dx;
  dc->ty = dy;
}

void dkDC_setFillStyle(struct dtkDC *dc, enum DKFillStyle fillstyle)
{
  if (!dc->surface) {
    dkerror("dkDC:setFillStyle: DC not connected to drawable.\n");
  }
  dc->fill = fillstyle;
  dc->needsNewBrush = TRUE;
  dc->needsNewPen = TRUE;
}

void dkDCSetBackground(struct dtkDC *dc, DKColor clr)
{
  if (!dc->surface) {
    dkerror("dkDC:setBackground: DC not connected to drawable.\n");
  }
  dc->devbg = dtkDrvVisualGetPixel(dc->visual, clr);
  SetBkColor((HDC)dc->ctx, dc->devbg);
  dc->bg = clr;
}

#else

/* Begin locks in a drawable surface */
static void
dtkDrvDCBegin(struct dtkDC *dc, struct dkWindow *w)
{
	if(!w){ printf("FXDCWindow::begin: NULL drawable.\n"); }
	if(!w->xid){ printf("FXDCWindow::begin: drawable not created yet.\n"); }

	dc->surface = w;
	dc->visual = w->visual;
	dc->rect.x = dc->clip.x = 0;
	dc->rect.y = dc->clip.y = 0;
	dc->rect.w = dc->clip.w = w->width;
	dc->rect.h = dc->clip.h = w->height;
	dc->devfg = ~0;
	dc->devbg = 0;
	dc->ctx = dc->visual->gc;
	dc->flags = 0;
#ifdef HAVE_XFT_H
	dc->xftDraw = (void*)XftDrawCreate(dc->app->display, (Drawable)dc->surface->xid, (Visual*)dc->visual->visual,(Colormap)dc->visual->colormap);
#endif
}

void dkDCSetup(struct dtkDC *dc, struct dkWindow *w)
{
  dkDCInit(dc, w->app);

#ifdef HAVE_XFT_H
  dc->xftDraw = NULL;
#endif
  dtkDrvDCBegin(dc, w);
}

void
dtkDrvDCEventSetup(struct dtkDC *dc, struct dkWindow *w, struct dkEvent *event)
{
  dkDCInit(dc, w->app);

#ifdef HAVE_XFT_H
	dc->xftDraw = NULL;
#endif
	dtkDrvDCBegin(dc, w);
	dc->rect.x = dc->clip.x = event->rect.x;
	dc->rect.y = dc->clip.y = event->rect.y;
	dc->rect.w = dc->clip.w = event->rect.w;
	dc->rect.h = dc->clip.h = event->rect.h;
	XSetClipRectangles(w->app->display, (GC)dc->ctx, 0, 0, (XRectangle*)&dc->clip, 1, Unsorted);
#ifdef HAVE_XFT_H
	XftDrawSetClipRectangles((XftDraw*)dc->xftDraw, 0, 0, (XRectangle*)&dc->clip,1);
#endif
	dc->flags |= GCClipMask;
}

void
dkDCEnd(struct dtkDC *dc)
{
	unsigned int flags;
	Display *disp = dc->app->display;

	flags = dc->flags;
	if (dc->flags) {
		XGCValues gcv;
		if (flags & GCFunction) gcv.function = BLT_SRC;
		if (flags & GCForeground) gcv.foreground = BlackPixel(disp, DefaultScreen(disp));
		if (flags & GCBackground) gcv.background = WhitePixel(disp, DefaultScreen(disp));
		if (flags & GCLineWidth) gcv.line_width = 0;
		if (flags & GCCapStyle) gcv.cap_style = CAP_BUTT;
		if (flags & GCJoinStyle) gcv.join_style = JOIN_MITER;
		if (flags & GCLineStyle) gcv.line_style = LINE_SOLID;
		if (flags & GCFillStyle) gcv.fill_style = FILL_SOLID;
//		if (flags & GCStipple) gcv.stipple = getApp()->stipples[STIPPLE_WHITE];    // Needed for IRIX6.4 bug workaround!
		if (flags & GCFillRule) gcv.fill_rule = RULE_EVEN_ODD;
#ifndef HAVE_XFT_H
		if(flags & GCFont) gcv.font = dc->app->normalFont->xid;
#endif
		if (flags & GCClipMask) gcv.clip_mask = None;
		if (flags & GCClipXOrigin) gcv.clip_x_origin = 0;
		if (flags & GCClipYOrigin) gcv.clip_y_origin = 0;
		if (flags & GCDashOffset) gcv.dash_offset = 0;
		if (flags & GCDashList) gcv.dashes = 4;
		if (flags & GCTileStipXOrigin) gcv.ts_x_origin = 0;
		if (flags & GCTileStipYOrigin) gcv.ts_y_origin = 0;
		if (flags & GCGraphicsExposures) gcv.graphics_exposures = True;
		if (flags & GCSubwindowMode) gcv.subwindow_mode = ClipByChildren;
		XChangeGC(disp, (GC)dc->ctx, dc->flags, &gcv);
		dc->flags = 0;
	}
	dc->surface = NULL;
#ifdef HAVE_XFT_H
	if (dc->xftDraw) { XftDrawDestroy((XftDraw*)dc->xftDraw); dc->xftDraw=NULL; }
#endif
}

void
dtkDrvDCSetForeground(struct dtkDC *dc, unsigned int clr)
{
	if(!dc->surface){ printf("FXDCWindow::setForeground: DC not connected to drawable.\n"); }
	dc->devfg = dtkDrvVisualGetPixel(dc->visual, clr);
	XSetForeground(dc->app->display, (GC)dc->ctx, dc->devfg);
	dc->flags |= GCForeground;
	dc->fg = clr;
}

/* Set background color */
void dkDCSetBackground(struct dtkDC *dc, DKColor clr)
{
  if (!dc->surface) { dkerror("FXDCWindow::setBackground: DC not connected to drawable.\n"); }
  dc->devbg = dtkDrvVisualGetPixel(dc->visual, clr);
  XSetBackground(dc->app->display, (GC)dc->ctx, dc->devbg);
  dc->flags |= GCBackground;
  dc->bg = clr;
}

void dkDCSetClipRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
  if (!dc->surface) {
    dkerror("dkDCSetClipRectangle: DC not connected to drawable.\n");
  }
  dc->clip.x = FXMAX(x, dc->rect.x);
  dc->clip.y = FXMAX(y, dc->rect.y);
  dc->clip.w = FXMIN(x + w, dc->rect.x + dc->rect.w) - dc->clip.x;
  dc->clip.h = FXMIN(y + h, dc->rect.y + dc->rect.h) - dc->clip.y;
  if (dc->clip.w <= 0) dc->clip.w = 0;
  if (dc->clip.h <= 0) dc->clip.h = 0;

  XSetClipRectangles(dc->app->display, (GC)dc->ctx, 0, 0, (XRectangle *)&dc->clip, 1, Unsorted);
#ifdef HAVE_XFT_H
  XftDrawSetClipRectangles((XftDraw *)dc->xftDraw, 0, 0, (XRectangle *)&dc->clip, 1);
#endif
  dc->flags |= GCClipMask;
}

void
dtkDrvDCFillRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
	if(!dc->surface){ printf("FXDCWindow::fillRectangle: DC not connected to drawable.\n"); }
	XFillRectangle(dc->app->display, dc->surface->xid, (GC)dc->ctx, x, y, w, h);
}

void dkDCDrawRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
  if (!dc->surface) { dkerror("FXDCWindow::drawRectangle: DC not connected to drawable.\n"); }
  XDrawRectangle(dc->app->display, dc->surface->xid, (GC)dc->ctx, x, y, w, h);
}

/* Set text font */
void dkDCWindowSetFont(struct dtkDC *dc, struct dkFont *fnt)
{
	if (!dc->surface) {
		printf("%s: DC not connected to drawable.\n", __func__);
	}
	if (!fnt || !fnt->xid) {
		printf("%s: illegal or NULL font specified.\n", __func__);
	}
#ifndef HAVE_XFT_H
	XSetFont(dc->app->display, (GC)dc->ctx, fnt->xid);
	dc->flags |= GCFont;
#endif
	dc->font = fnt;
}

/* Draw string with base line starting at x, y */
void dkDCDrawText(struct dtkDC *dc, int x, int y, char *string, DKuint length)
{
#ifdef HAVE_XFT_H
	XftColor color;
#endif
	if (!dc->surface) {
		printf("%s: DC not connected to drawable.\n", __func__);
	}
	if (!dc->font) {
		printf("%s: no font selected.\n", __func__);
	}
#ifdef HAVE_XFT_H
	color.pixel = dc->devfg;
	color.color.red = FXREDVAL(dc->fg) * 257;
	color.color.green = FXGREENVAL(dc->fg) * 257;
	color.color.blue = FXBLUEVAL(dc->fg) * 257;
	color.color.alpha = FXALPHAVAL(dc->fg) * 257;
	XftDrawStringUtf8((XftDraw*)dc->xftDraw, &color, (XftFont*)dc->font->font, x, y, (const FcChar8*)string, length);
#else
#endif
}

/* Fill polygon */
void dkDCFillPolygon(struct dtkDC *dc, struct dkPoint *points, DKuint npoints)
{
  if (!dc->surface) { dkerror("FXDCWindow::fillArcs: DC not connected to drawable.\n"); }
  XFillPolygon(dc->app->display, dc->surface->xid, (GC)dc->ctx, (XPoint*)points, npoints, Convex, CoordModeOrigin);
}

void dkDCDrawFocusRectangle(struct dtkDC *dc, int x, int y, int w, int h)
{
  XGCValues gcv;

  if (!dc->surface) { dkerror("dkDC::drawFocusRectangle: DC not connected to drawable.\n"); }
  gcv.stipple = dc->app->stipples[STIPPLE_GRAY];
  gcv.fill_style = FILL_STIPPLED;
  gcv.background = 0;
  gcv.foreground = 0xffffffff;    // Maybe should use FILL_OPAQUESTIPPLED and current fg/bg color and BLT_SRC
  gcv.function = BLT_SRC_XOR_DST; // This would be more flexible
  gcv.ts_x_origin = x;
  gcv.ts_y_origin = y;
  XChangeGC(dc->app->display, (GC)dc->ctx, GCTileStipXOrigin | GCTileStipYOrigin | GCForeground | GCBackground | GCFunction | GCStipple | GCFillStyle, &gcv);
  XFillRectangle(dc->app->display, dc->surface->xid, (GC)dc->ctx, x, y, w - 1, 1);
  XFillRectangle(dc->app->display, dc->surface->xid, (GC)dc->ctx, x + w - 1, y, 1, h - 1);
  XFillRectangle(dc->app->display, dc->surface->xid, (GC)dc->ctx, x + 1, y + h - 1, w - 1, 1);
  XFillRectangle(dc->app->display, dc->surface->xid, (GC)dc->ctx, x, y + 1, 1, h - 1);
  gcv.stipple = dc->app->stipples[STIPPLE_WHITE];    // Needed for IRIX6.4 bug workaround!
  gcv.fill_style = dc->fill;
  gcv.background = dc->devbg;
  gcv.foreground = dc->devfg;
  gcv.function = dc->rop;
  gcv.ts_x_origin = dc->tx;
  gcv.ts_y_origin = dc->ty;
  XChangeGC(dc->app->display, (GC)dc->ctx, GCTileStipXOrigin | GCTileStipYOrigin | GCForeground | GCBackground | GCFunction | GCStipple | GCFillStyle, &gcv);
}

/* Set stipple pattern */
void dkDC_setStipplePat(struct dtkDC *dc, enum DKStipplePattern pat, int dx, int dy)
{
  XGCValues gcv;

  if (!dc->surface) { dkerror("dkDC::setStipple: DC not connected to drawable.\n"); }
  if (pat > STIPPLE_CROSSDIAG) pat = STIPPLE_CROSSDIAG;
  gcv.stipple = dc->app->stipples[pat];
  gcv.ts_x_origin = dx;
  gcv.ts_y_origin = dy;
  XChangeGC(dc->app->display, (GC)dc->ctx, GCTileStipXOrigin | GCTileStipYOrigin | GCStipple, &gcv);
  if (dx) dc->flags |= GCTileStipXOrigin;
  if (dy) dc->flags |= GCTileStipYOrigin;
  dc->stipple = NULL;
  dc->pattern = pat;
  dc->flags |= GCStipple;
  dc->tx = dx;
  dc->ty = dy;
}

/* Set fill style */
void dkDC_setFillStyle(struct dtkDC *dc, enum DKFillStyle fillstyle)
{
  if (!dc->surface) { dkerror("dkDCWindow::setFillStyle: DC not connected to drawable.\n"); }
  XSetFillStyle(dc->app->display, (GC)dc->ctx, fillstyle);
  dc->flags |= GCFillStyle;
  dc->fill = fillstyle;
}

#endif

