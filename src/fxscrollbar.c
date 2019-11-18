/******************************************************************************
 *                                                                            *
 *                    S c r o l l   B a r   W i d g e t                       *
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
 * $Id: FXScrollBar.cpp,v 1.27 2006/01/22 17:58:41 fox Exp $                  *
 *****************************************************************************/

#include "fxdc.h"
#include "fxscrollbar.h"
#include "fxpoint.h"

/*
  Notes:
  - Should increase/decrease, and slider get messages instead?
  - Scrollbar items should derive from FXWindow (as they are very simple).
  - If non-scrollable, but drawn anyway, don't draw thumb!
  - In case of a coarse range, we have rounding also.
  - The API's setPosition(), setRange() and setPage() should probably have
    an optional notify callback.
*/

static int dkScrollBar_getDefaultWidth(struct dkWindow *win);
static int dkScrollBar_getDefaultHeight(struct dkWindow *win);
void dkScrollBar_layout(struct dkWindow *w);
static long dkScrollBar_onPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);

static long dkScrollCornerPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
void dkScrollCornerEnable(struct dkWindow *w);
void dkScrollCornerDisable(struct dkWindow *w);

#define SCROLLBAR_MASK  (SCROLLBAR_HORIZONTAL|SCROLLBAR_WHEELJUMP)

/* Map */
static struct dkMapEntry dkScrollBarMap[] = {
  FXMAPFUNC(SEL_PAINT, 0, dkScrollBar_onPaint)
};

/* Object implementation */
static struct dkMetaClass dkScrollBarMetaClass = {
  "dkScrollBar", dkScrollBarMap, sizeof(dkScrollBarMap) / sizeof(dkScrollBarMap[0]), sizeof(struct dkMapEntry)
};

long dkScrollBar_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;
  DKSelector sel = DKSEL(selhi, sello);

  me = DKMetaClassSearch(&dkScrollBarMetaClass, sel);
  return me ? me->func(pthis, obj, selhi, sello, data) : dkWindow_handle(pthis, obj, selhi, sello, data);
}

struct dkScrollBar *dkScrollBarNew(struct dkWindow *p, struct dkObject *tgt, DKSelector sel, DKuint opts, int x, int y, int w, int h)
{
  struct dkScrollBar *ret;

  ret = malloc(sizeof(struct dkScrollBar));
  dkScrollBarInit(ret, p, tgt, sel, opts, x, y, w, h);
  return ret;
}

/* Make a scrollbar */
void dkScrollBarInit(struct dkScrollBar *pthis, struct dkWindow *p, struct dkObject *tgt, DKSelector sel, DKuint opts, int x, int y, int w, int h)
{
  dkWindowChildInit((struct dkWindow *)pthis, (struct dkWindow *)p, opts, x, y, w, h);
  ((struct dkObject *)pthis)->meta = &dkScrollBarMetaClass;

  /* Setup overloads */
  ((struct dkWindow *)pthis)->getDefaultWidth = dkScrollBar_getDefaultWidth;
  ((struct dkWindow *)pthis)->getDefaultHeight = dkScrollBar_getDefaultHeight;
  ((struct dkWindow *)pthis)->layout = dkScrollBar_layout;
  ((struct dkObject *)pthis)->handle = dkScrollBar_handle;

  /* Setup object */
  ((struct dkWindow *)pthis)->flags |= FLAG_ENABLED | FLAG_SHOWN;
  ((struct dkWindow *)pthis)->backColor = ((struct dkWindow *)pthis)->app->baseColor;
  pthis->hiliteColor = ((struct dkWindow *)pthis)->app->hiliteColor;
  pthis->shadowColor = ((struct dkWindow *)pthis)->app->shadowColor;
  pthis->borderColor = ((struct dkWindow *)pthis)->app->borderColor;
  pthis->arrowColor = ((struct dkWindow *)pthis)->app->foreColor;
  pthis->barsize = ((struct dkWindow *)pthis)->app->scrollBarSize;
  pthis->thumbpos = pthis->barsize;
  pthis->thumbsize = pthis->barsize >> 1;
  ((struct dkWindow *)pthis)->target = tgt;
  ((struct dkWindow *)pthis)->message = sel;
  pthis->dragpoint = 0;
  pthis->range = 100;
  pthis->page = 1;
  pthis->line = 1;
  pthis->pos = 0;
  pthis->mode = MODE_NONE;
}

static int dkScrollBar_getDefaultWidth(struct dkWindow *win)
{
  return (win->options & SCROLLBAR_HORIZONTAL) ? ((struct dkScrollBar *)win)->barsize + ((struct dkScrollBar *)win)->barsize + (((struct dkScrollBar *)win)->barsize >> 1) : ((struct dkScrollBar *)win)->barsize;
}

static int dkScrollBar_getDefaultHeight(struct dkWindow *win)
{
  return (win->options & SCROLLBAR_HORIZONTAL) ? ((struct dkScrollBar *)win)->barsize : ((struct dkScrollBar *)win)->barsize + ((struct dkScrollBar *)win)->barsize + (((struct dkScrollBar *)win)->barsize >> 1);
}

/* Set range */
void dkScrollBar_setRange(struct dkScrollBar *sb, int r)
{
  if (r < 1) r = 1;
  if (sb->range != r) {
    sb->range = r;
    dkScrollBar_setPage(sb, sb->page);
  }
}

/* Set page size */
void dkScrollBar_setPage(struct dkScrollBar *sb, int p)
{
  if (p < 1) p = 1;
  if (p > sb->range) p = sb->range;
  if (sb->page != p) {
    sb->page = p;
    dkScrollBar_setPosition(sb, sb->pos);
  }
}

/* Set line size */
void dkScrollBar_setLine(struct dkScrollBar *sb, int l)
{
  if (l < 1) l = 1;
  sb->line = l;
}

/* Set position; tricky because the thumb size may have changed
 * as well; we do the minimal possible update to repaint properly. */
void dkScrollBar_setPosition(struct dkScrollBar *sb, int p)
{
  int total, travel, lo, hi, l, h;

  sb->pos = p;
  if (sb->pos < 0) sb->pos = 0;
  if (sb->pos > (sb->range - sb->page)) sb->pos = sb->range - sb->page;
  lo = sb->thumbpos;
  hi = sb->thumbpos + sb->thumbsize;
  if (((struct dkWindow *)sb)->options & SCROLLBAR_HORIZONTAL) {
    total = ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->height;
    sb->thumbsize = (total * sb->page) / sb->range;
    if (sb->thumbsize < (sb->barsize >> 1)) sb->thumbsize = (sb->barsize >> 1);
    travel = total - sb->thumbsize;
    if (sb->range > sb->page) { sb->thumbpos = ((struct dkWindow *)sb)->height + (int)((((double)sb->pos) * travel) / (sb->range - sb->page)); }
    else { sb->thumbpos = ((struct dkWindow *)sb)->height; }
    l = sb->thumbpos;
    h = sb->thumbpos + sb->thumbsize;
    if (l != lo || h != hi) {
      dkWindowUpdateRect((struct dkWindow *)sb, FXMIN(l, lo), 0, FXMAX(h, hi) - FXMIN(l, lo), ((struct dkWindow *)sb)->height);
    }
  }
  else {
    total = ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->width;
    sb->thumbsize = (total * sb->page) / sb->range;
    if (sb->thumbsize < (sb->barsize >> 1)) sb->thumbsize = (sb->barsize >> 1);
    travel = total - sb->thumbsize;
    if (sb->range > sb->page) { sb->thumbpos = ((struct dkWindow *)sb)->width + (int)((((double)sb->pos) * travel) / (sb->range - sb->page)); }
    else { sb->thumbpos = ((struct dkWindow *)sb)->width; }
    l = sb->thumbpos;
    h = sb->thumbpos + sb->thumbsize;
    if (l != lo || h != hi) {
      dkWindowUpdateRect((struct dkWindow *)sb, 0, FXMIN(l, lo), ((struct dkWindow *)sb)->width, FXMAX(h, hi) - FXMIN(l, lo));
    }
  }
}

/* Draw left arrow */
void dkScrollBar_drawLeftArrow(struct dkScrollBar *sb, struct dtkDC *dc, int x, int y, int w, int h, DKbool down)
{
  struct dkPoint points[3];
  int ah, ab;

  ab = (h - 7) | 1;
  ah = ab >> 1;
  x = x + ((w - ah) >> 1);
  y = y + ((h - ab) >> 1);
  if (down) { ++x; ++y; }
  points[0].x = x + ah;
  points[0].y = y;
  points[1].x = x + ah;
  points[1].y = y + ab - 1;
  points[2].x = x;
  points[2].y = y + (ab >> 1);
  dtkDrvDCSetForeground(dc, sb->arrowColor);
  dkDCFillPolygon(dc, points, 3);
}

/* Draw right arrow */
void dkScrollBar_drawRightArrow(struct dkScrollBar *sb, struct dtkDC *dc, int x, int y, int w, int h, DKbool down)
{
  struct dkPoint points[3];
  int ah, ab;

  ab = (h - 7) | 1;
  ah = ab >> 1;

  x = x + ((w - ah) >> 1);
  y = y + ((h - ab) >> 1);
  if (down) { ++x; ++y; }
  points[0].x = x;
  points[0].y = y;
  points[1].x = x;
  points[1].y = y + ab - 1;
  points[2].x = x + ah;
  points[2].y = y + (ab >> 1);
  dtkDrvDCSetForeground(dc, sb->arrowColor);
  dkDCFillPolygon(dc, points, 3);
}

/* Draw up arrow */
void dkScrollBar_drawUpArrow(struct dkScrollBar *sb, struct dtkDC *dc, int x, int y, int w, int h, DKbool down)
{
  struct dkPoint points[3];
  int ah, ab;

  ab = (w - 7) | 1;
  ah = ab >> 1;
  x = x + ((w - ab) >> 1);
  y = y + ((h - ah) >> 1);
  if (down) { ++x; ++y; }
  points[0].x = x + (ab >> 1);
  points[0].y = y - 1;
  points[1].x = x;
  points[1].y = y + ah;
  points[2].x = x + ab;
  points[2].y = y + ah;
  dtkDrvDCSetForeground(dc, sb->arrowColor);
  dkDCFillPolygon(dc, points, 3);
}

/* Draw down arrow */
void dkScrollBar_drawDownArrow(struct dkScrollBar *sb, struct dtkDC *dc, int x, int y, int w, int h, DKbool down)
{
  struct dkPoint points[3];
  int ah, ab;

  ab = (w - 7) | 1;
  ah = ab >> 1;
  x = x + ((w - ab) >> 1);
  y = y + ((h - ah) >> 1);
  if (down) { ++x; ++y; }
  points[0].x = x + 1;
  points[0].y = y;
  points[1].x = x + ab - 1;
  points[1].y = y;
  points[2].x = x + (ab >> 1);
  points[2].y = y + ah;
  dtkDrvDCSetForeground(dc, sb->arrowColor);
  dkDCFillPolygon(dc, points, 3);
}

/* Layout changed */
void dkScrollBar_layout(struct dkWindow *w)
{
  dkScrollBar_setPosition((struct dkScrollBar *)w, ((struct dkScrollBar *)w)->pos);
  w->flags &= ~FLAG_DIRTY;
}

/* Draw button in scrollbar; this is slightly different from a raised rectangle */
void dkScrollBar_drawButton(struct dkScrollBar *sb, struct dtkDC *dc, int x, int y, int w, int h, DKbool down)
{
  dtkDrvDCSetForeground(dc, ((struct dkWindow *)sb)->backColor);
  dtkDrvDCFillRectangle(dc, x + 2, y + 2, w - 4, h - 4);
  if (!down) {
    dtkDrvDCSetForeground(dc, ((struct dkWindow *)sb)->backColor);
    dtkDrvDCFillRectangle(dc, x, y, w - 1, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h - 1);
    dtkDrvDCSetForeground(dc, sb->hiliteColor);
    dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 2, 1);
    dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 2);
    dtkDrvDCSetForeground(dc, sb->shadowColor);
    dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
    dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
    dtkDrvDCSetForeground(dc, sb->borderColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
  }
  else {
    dtkDrvDCSetForeground(dc, sb->borderColor);
    dtkDrvDCFillRectangle(dc, x, y, w - 2, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h - 2);
    dtkDrvDCSetForeground(dc, sb->shadowColor);
    dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
    dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
    dtkDrvDCSetForeground(dc, sb->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w - 1, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y + 1, 1, h - 1);
    dtkDrvDCSetForeground(dc, ((struct dkWindow *)sb)->backColor);
    dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 1, 1);
    dtkDrvDCFillRectangle(dc, x + w - 2, y + 2, 1, h - 2);
  }
}

long dkScrollBar_onPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dtkDC dc;
  struct dkEvent *ev = (struct dkEvent *)ptr;
  struct dkScrollBar *sb = (struct dkScrollBar *)pthis;
  int total;

  dtkDrvDCEventSetup(&dc, (struct dkWindow *)pthis, ev);
  if (((struct dkWindow *)sb)->options & SCROLLBAR_HORIZONTAL) {
    total = ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->height;
    if (sb->thumbsize < total) {                 /* Scrollable */
      dkScrollBar_drawButton(sb, &dc, sb->thumbpos, 0, sb->thumbsize, ((struct dkWindow *)sb)->height, 0);
      dkDC_setStipplePat(&dc, STIPPLE_GRAY, 0, 0);
      dkDC_setFillStyle(&dc, FILL_OPAQUESTIPPLED);
      if (sb->mode == MODE_PAGE_DEC) {
        dtkDrvDCSetForeground(&dc, ((struct dkWindow *)sb)->backColor);
        dkDCSetBackground(&dc, sb->shadowColor);
      } else {
        dtkDrvDCSetForeground(&dc, sb->hiliteColor);
        dkDCSetBackground(&dc, ((struct dkWindow *)sb)->backColor);
      }
      dtkDrvDCFillRectangle(&dc, ((struct dkWindow *)sb)->height, 0, sb->thumbpos - ((struct dkWindow *)sb)->height, ((struct dkWindow *)sb)->height);
      if (sb->mode == MODE_PAGE_INC) {
        dtkDrvDCSetForeground(&dc, ((struct dkWindow *)sb)->backColor);
        dkDCSetBackground(&dc, sb->shadowColor);
      } else {
        dtkDrvDCSetForeground(&dc, sb->hiliteColor);
        dkDCSetBackground(&dc, ((struct dkWindow *)sb)->backColor);
      }
      dtkDrvDCFillRectangle(&dc, sb->thumbpos + sb->thumbsize, 0, ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->height - sb->thumbpos - sb->thumbsize, ((struct dkWindow *)sb)->height);
    } else {  /* Non-scrollable */
      dkDC_setStipplePat(&dc, STIPPLE_GRAY, 0, 0);
      dkDC_setFillStyle(&dc, FILL_OPAQUESTIPPLED);
      dtkDrvDCSetForeground(&dc, sb->hiliteColor);
      dkDCSetBackground(&dc, ((struct dkWindow *)sb)->backColor);
      dtkDrvDCFillRectangle(&dc, ((struct dkWindow *)sb)->height, 0, total, ((struct dkWindow *)sb)->height);
    }
    dkDC_setFillStyle(&dc, FILL_SOLID);
    dkScrollBar_drawButton(sb, &dc, ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->height, 0, ((struct dkWindow *)sb)->height, ((struct dkWindow *)sb)->height, (sb->mode == MODE_INC));
    dkScrollBar_drawRightArrow(sb, &dc, ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->height, 0, ((struct dkWindow *)sb)->height, ((struct dkWindow *)sb)->height, (sb->mode == MODE_INC));
    dkScrollBar_drawButton(sb, &dc, 0, 0, ((struct dkWindow *)sb)->height, ((struct dkWindow *)sb)->height, (sb->mode == MODE_DEC));
    dkScrollBar_drawLeftArrow(sb, &dc, 0, 0, ((struct dkWindow *)sb)->height, ((struct dkWindow *)sb)->height, (sb->mode == MODE_DEC));
  } else {
    total = ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->width - ((struct dkWindow *)sb)->width;
    if (sb->thumbsize < total) {  /* Scrollable */
      dkScrollBar_drawButton(sb, &dc, 0, sb->thumbpos, ((struct dkWindow *)sb)->width, sb->thumbsize, 0);
      dkDC_setStipplePat(&dc, STIPPLE_GRAY, 0, 0);
      dkDC_setFillStyle(&dc, FILL_OPAQUESTIPPLED);
      if (sb->mode == MODE_PAGE_DEC) {
        dtkDrvDCSetForeground(&dc, ((struct dkWindow *)sb)->backColor);
        dkDCSetBackground(&dc, sb->shadowColor);
      } else {
        dtkDrvDCSetForeground(&dc, sb->hiliteColor);
        dkDCSetBackground(&dc, ((struct dkWindow *)sb)->backColor);
      }
      dtkDrvDCFillRectangle(&dc, 0, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, sb->thumbpos - ((struct dkWindow *)sb)->width);
      if (sb->mode == MODE_PAGE_INC) {
        dtkDrvDCSetForeground(&dc, ((struct dkWindow *)sb)->backColor);
        dkDCSetBackground(&dc, sb->shadowColor);
      } else {
        dtkDrvDCSetForeground(&dc, sb->hiliteColor);
        dkDCSetBackground(&dc, ((struct dkWindow *)sb)->backColor);
      }
      dtkDrvDCFillRectangle(&dc, 0, sb->thumbpos + sb->thumbsize, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->width - sb->thumbpos - sb->thumbsize);
    } else { /* Non-scrollable */
      dkDC_setStipplePat(&dc, STIPPLE_GRAY, 0, 0);
      dkDC_setFillStyle(&dc, FILL_OPAQUESTIPPLED);
      dtkDrvDCSetForeground(&dc, sb->hiliteColor);
      dkDCSetBackground(&dc, ((struct dkWindow *)sb)->backColor);
      dtkDrvDCFillRectangle(&dc, 0, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, total);
    }
    dkDC_setFillStyle(&dc, FILL_SOLID);
    dkScrollBar_drawButton(sb, &dc, 0, ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, (sb->mode == MODE_INC));
    dkScrollBar_drawDownArrow(sb, &dc, 0, ((struct dkWindow *)sb)->height - ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, (sb->mode == MODE_INC));
    dkScrollBar_drawButton(sb, &dc, 0, 0, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, (sb->mode == MODE_DEC));
    dkScrollBar_drawUpArrow(sb, &dc, 0, 0, ((struct dkWindow *)sb)->width, ((struct dkWindow *)sb)->width, (sb->mode == MODE_DEC));
  }
  return 1;
}

/*******************************************************************************/

/* Map */
static struct dkMapEntry dkScrollCornerMap[] = {
  FXMAPFUNC(SEL_PAINT, 0, dkScrollCornerPaint)
};

/* Object implementation */
static struct dkMetaClass dkScrollCornerMetaClass = {
  "dkScrollCorner", dkScrollCornerMap, sizeof(dkScrollCornerMap) / sizeof(dkScrollCornerMap[0]), sizeof(struct dkMapEntry)
};

/* Handle message */
long dkScrollCorner_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;
  DKSelector sel = DKSEL(selhi, sello);

  me = DKMetaClassSearch(&dkScrollCornerMetaClass, sel);
  return me ? me->func(pthis, obj, selhi, sello, data) : dkWindow_handle(pthis, obj, selhi, sello, data);
}

/* Construct and init */
struct dkScrollCorner *dkScrollCornerNew(struct dkComposite *p)
{
  struct dkScrollCorner *ret;

  ret = fx_alloc(sizeof(struct dkScrollCorner));
  dkScrollCornerInit(ret, p);

  return ret;
}

void dkScrollCornerInit(struct dkScrollCorner *pthis, struct dkComposite *p)
{
  dkWindowChildInit((struct dkWindow *)pthis, (struct dkWindow *)p, 0, 0, 0, 0, 0);
  ((struct dkObject *)pthis)->meta = &dkScrollCornerMetaClass;

  ((struct dkObject *)pthis)->handle = dkScrollCorner_handle;
  ((struct dkWindow *)pthis)->enable = dkScrollCornerEnable;
  ((struct dkWindow *)pthis)->disable = dkScrollCornerDisable;

  ((struct dkWindow *)pthis)->backColor = ((struct dkWindow *)pthis)->app->baseColor;
  ((struct dkWindow *)pthis)->flags |= FLAG_ENABLED | FLAG_SHOWN;
}

/* Slightly different from Frame border */
static long dkScrollCornerPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dtkDC dc;
  struct dkWindow *win = (struct dkWindow *)pthis;
  struct dkEvent *ev = (struct dkEvent *)ptr;

  /* Start drawing */
  dtkDrvDCEventSetup(&dc, win, ev);
  dtkDrvDCSetForeground(&dc, win->backColor);
  dtkDrvDCFillRectangle(&dc, ev->rect.x, ev->rect.y, ev->rect.w, ev->rect.h);
  dkDCEnd(&dc);
  return 1;
}

void dkScrollCornerEnable(struct dkWindow *w)
{
  /* Do nothing */
}

void dkScrollCornerDisable(struct dkWindow *w)
{
  /* Do nothing */
}

