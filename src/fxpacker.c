/******************************************************************************
 *                                                                            *
 *             P a c k e r   C o n t a i n e r   W i d g e t                  *
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
 * $Id: FXPacker.cpp,v 1.46 2006/01/22 17:58:37 fox Exp $                     *
 *****************************************************************************/

#include "fxdc.h"
#include "fxpacker.h"

/*
  To do:
  - Now observes LAYOUT_FIX_X and LAYOUT_FIX_Y hints.
  - LAYOUT_FIX_WIDTH and LAYOUT_FIX_HEIGHT take precedence over PACK_UNIFORM_WIDTH and
    PACK_UNIFORM_HEIGHT!
  - Tabbing order takes widget layout into account
*/
static int dkPacker_getDefaultWidth(struct dkWindow *win);
static int dkPacker_getDefaultHeight(struct dkWindow *win);
void dkPacker_layout(struct dkWindow *win);
long dkPacker_onPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

/* Frame styles */
#define FRAME_MASK      (FRAME_SUNKEN | FRAME_RAISED | FRAME_THICK)

/* Map */
static struct dkMapEntry dkPackerMapEntry[] = {
  FXMAPFUNC(SEL_PAINT, 0, dkPacker_onPaint)
};

/* Object implementation */
static struct dkMetaClass dkPackerMetaClass = {
  "dkPacker", dkPackerMapEntry, sizeof(dkPackerMapEntry) / sizeof(dkPackerMapEntry[0]), sizeof(struct dkMapEntry)
};

/* Handle message */
long dkPacker_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;

  me = DKMetaClassSearch(&dkPackerMetaClass, DKSEL(selhi, sello));
  return me ? me->func(pthis, obj, selhi, sello, data) : dkComposite_handle(pthis, obj, selhi, sello, data);
}

void dkPackerSetup(struct dkPacker *p, struct dkWindow *win, int pl, int pr, int pt, int pb, int hs, int vs)
{
  /* Pull in dkComposite vtbl */
  dkCompositeVtblSetup(win);
  ((struct dkObject *)win)->meta = &dkPackerMetaClass;
  /* Setup the rest of the vtbl */
  ((struct dkObject *)win)->handle = dkPacker_handle;
  win->getDefaultWidth = dkPacker_getDefaultWidth;
  win->getDefaultHeight = dkPacker_getDefaultHeight;
  win->layout = dkPacker_layout;

  win->flags |= FLAG_SHOWN;
  p->baseColor = win->app->baseColor;
  p->hiliteColor = win->app->hiliteColor;
  p->shadowColor = win->app->shadowColor;
  p->borderColor = win->app->borderColor;
  p->padtop = pt;
  p->padbottom = pb;
  p->padleft = pl;
  p->padright = pr;
  p->hspacing = hs;
  p->vspacing = vs;
  p->border = win->options & FRAME_THICK ? 2 : win->options & (FRAME_SUNKEN | FRAME_RAISED) ? 1 : 0;
}

/* Create child frame window */
void dkPackerInit(struct dkPacker *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs)
{
  dkWindowChildInit((struct dkWindow *)pthis, p, opts, x, y, w, h);
  dkPackerSetup(pthis, (struct dkWindow *)pthis, pl, pr, pt, pb, hs, vs);
}

/* Compute minimum width based on child layout hints */
static int dkPacker_getDefaultWidth(struct dkWindow *win)
{
  int w, wcum, wmax, mw;
  struct dkWindow *child;
  DKuint hints;
  wmax = wcum = mw = 0;

  if (win->options & PACK_UNIFORM_WIDTH) mw = dkCompositeMaxChildWidth(win);
  for (child = win->last; child; child = child->prev) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      if (hints & LAYOUT_FIX_WIDTH) w = child->width;
      else if (win->options & PACK_UNIFORM_WIDTH) w = mw;
      else w = child->getDefaultWidth(child);
      if ((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X)) {        /* Fixed X */
        w = child->xpos + w;
        if (w > wmax) wmax = w;
      }
      else if (hints & LAYOUT_SIDE_LEFT) {                          /* Left or right */
        if (child->next) wcum += ((struct dkPacker *)win)->hspacing;
        wcum += w;
      }
      else {
        if (w > wcum) wcum = w;
      }
    }
  }
  wcum += ((struct dkPacker *)win)->padleft + ((struct dkPacker *)win)->padright + (((struct dkPacker *)win)->border << 1);
  return FXMAX(wcum, wmax);
}

/* Compute minimum height based on child layout hints */
static int dkPacker_getDefaultHeight(struct dkWindow *win)
{
  int h, hcum, hmax, mh;
  struct dkWindow *child;
  DKuint hints;
  hmax = hcum = mh = 0;

  if (win->options & PACK_UNIFORM_HEIGHT) mh = dkCompositeMaxChildHeight(win);
  for (child= win->last; child; child = child->prev) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
      else if (win->options & PACK_UNIFORM_HEIGHT) h = mh;
      else h = child->getDefaultHeight(child);
      if ((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y)) {       /* Fixed Y */
        h = child->ypos + h;
        if (h > hmax) hmax = h;
      }
      else if (!(hints & LAYOUT_SIDE_LEFT)) {                       /* Top or bottom */
        if (child->next) hcum += ((struct dkPacker *)win)->vspacing;
        hcum += h;
      }
      else {
        if (h > hcum) hcum = h;
      }
    }
  }
  hcum += ((struct dkPacker *)win)->padtop + ((struct dkPacker *)win)->padbottom + (((struct dkPacker *)win)->border<<1);
  return FXMAX(hcum, hmax);
}

/* Recalculate layout */
void dkPacker_layout(struct dkWindow *win)
{
  int left, right, top, bottom, x, y, w, h;
  int mw = 0, mh = 0;
  struct dkWindow *child;
  DKuint hints;

  /* Placement rectangle; right/bottom non-inclusive */
  left = ((struct dkPacker *)win)->border + ((struct dkPacker *)win)->padleft;
  right = win->width - ((struct dkPacker *)win)->border - ((struct dkPacker *)win)->padright;
  top = ((struct dkPacker *)win)->border + ((struct dkPacker *)win)->padtop;
  bottom = win->height - ((struct dkPacker *)win)->border - ((struct dkPacker *)win)->padbottom;

  /* Get maximum child size */
  if (win->options & PACK_UNIFORM_WIDTH) mw = dkCompositeMaxChildWidth(win);
  if (win->options & PACK_UNIFORM_HEIGHT) mh = dkCompositeMaxChildHeight(win);

  /* Pack them in the cavity */
  for (child = win->first; child; child = child->next) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      x = child->xpos;
      y = child->ypos;

      /* Height */
      if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
      else if (win->options & PACK_UNIFORM_HEIGHT) h = mh;
      else if (hints & LAYOUT_FILL_Y) h = bottom - top;
      else h = child->getDefaultHeight(child);

      /* Width */
      if (hints & LAYOUT_FIX_WIDTH) w = child->width;
      else if (win->options & PACK_UNIFORM_WIDTH) w = mw;
      else if (hints & LAYOUT_FILL_X) w = right - left;
      else w = child->getDefaultWidth(child);

      /* Vertical */
      if(hints & LAYOUT_SIDE_LEFT) {

        /* Y */
        if (!((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y))) {
          if (hints & LAYOUT_CENTER_Y) y = top + (bottom - top - h) / 2;
          else if (hints & LAYOUT_BOTTOM) y = bottom - h;
          else y = top;
        }

        /* X */
        if (!((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X))) {
          if (hints & LAYOUT_CENTER_X) x = left + (right - left - w) / 2;
          else if (hints & LAYOUT_SIDE_BOTTOM) {
            x = right - w;
            right -= (w + ((struct dkPacker *)win)->hspacing);
          }
          else {
            x = left;
            left += (w + ((struct dkPacker *)win)->hspacing);
          }
        }
      }

      /* Horizontal */
      else {

        /* X */
        if (!((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X))) {
          if (hints & LAYOUT_CENTER_X) x = left + (right - left - w) / 2;
          else if (hints & LAYOUT_RIGHT) x = right - w;
          else x = left;
        }

        /* Y */
        if (!((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y))) {
          if (hints & LAYOUT_CENTER_Y) y = top + (bottom - top - h) / 2;
          else if (hints & LAYOUT_SIDE_BOTTOM) {
            y = bottom - h;
            bottom -= (h + ((struct dkPacker *)win)->vspacing);
          }
          else {
            y = top;
            top += (h + ((struct dkPacker *)win)->vspacing);
          }
        }
      }
      child->position(child, x, y, w, h);
    }
  }
  win->flags &= ~FLAG_DIRTY;
}

void dkPacker_drawBorderRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  dtkDrvDCSetForeground(dc, p->borderColor);
  dkDCDrawRectangle(dc, x, y, w - 1, h - 1);
}

void dkPacker_drawRaisedRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    dtkDrvDCSetForeground(dc, p->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    dtkDrvDCSetForeground(dc, p->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
  }
}

void dkPacker_drawSunkenRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    dtkDrvDCSetForeground(dc, p->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
    dtkDrvDCSetForeground(dc, p->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
  }
}

void dkPacker_drawRidgeRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    dtkDrvDCSetForeground(dc, p->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
    dtkDrvDCSetForeground(dc, p->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    if (w > 1 && h > 1) {
      dtkDrvDCSetForeground(dc, p->hiliteColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
      dtkDrvDCSetForeground(dc, p->shadowColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
    }
  }
}

void dkPacker_drawGrooveRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    dtkDrvDCSetForeground(dc, p->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
    dtkDrvDCSetForeground(dc, p->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    if (w > 1 && h > 1) {
      dtkDrvDCSetForeground(dc, p->shadowColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
      dtkDrvDCSetForeground(dc, p->hiliteColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
    }
  }
}

void dkPacker_drawDoubleRaisedRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    dtkDrvDCSetForeground(dc, p->borderColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    dtkDrvDCSetForeground(dc, p->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y, w - 1, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h - 1);
    if (w > 1 && h > 1) {
      dtkDrvDCSetForeground(dc, p->baseColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 2);
      dtkDrvDCSetForeground(dc, p->shadowColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
    }
  }
}

void dkPacker_drawDoubleSunkenRectangle(struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (w > 0 && h > 0) {
    dtkDrvDCSetForeground(dc, p->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    dtkDrvDCSetForeground(dc, p->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y, w - 1, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h - 1);
    if (w > 1 && h > 1) {
      dtkDrvDCSetForeground(dc, p->borderColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
      dtkDrvDCSetForeground(dc, p->baseColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
    }
  }
}

void dkPacker_drawFrame(struct dkWindow *win, struct dkPacker *p, struct dtkDC *dc, int x, int y, int w, int h)
{
  switch (win->options & FRAME_MASK) {
    case FRAME_LINE: dkPacker_drawBorderRectangle(p, dc, x, y, w, h); break;
    case FRAME_SUNKEN: dkPacker_drawSunkenRectangle(p, dc, x, y, w, h); break;
    case FRAME_RAISED: dkPacker_drawRaisedRectangle(p, dc, x, y, w, h); break;
    case FRAME_GROOVE: dkPacker_drawGrooveRectangle(p, dc, x, y, w, h); break;
    case FRAME_RIDGE: dkPacker_drawRidgeRectangle(p, dc, x, y, w, h); break;
    case FRAME_SUNKEN | FRAME_THICK: dkPacker_drawDoubleSunkenRectangle(p, dc, x, y, w, h); break;
    case FRAME_RAISED | FRAME_THICK: dkPacker_drawDoubleRaisedRectangle(p, dc, x, y, w, h); break;
  }
}

/* Handle repaint */
long dkPacker_onPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dtkDC dc;
  struct dkWindow *win = (struct dkWindow *)pthis;
  struct dkPacker *p = (struct dkPacker *)win->wExtra;
  struct dkEvent *ev = (struct dkEvent *)data;

  dtkDrvDCEventSetup(&dc, win, ev);
  dtkDrvDCSetForeground(&dc, win->backColor);
  dtkDrvDCFillRectangle(&dc, ev->rect.x, ev->rect.y, ev->rect.w, ev->rect.h);
  dkPacker_drawFrame(win, p, &dc, 0, 0, win->width, win->height);
  dkDCEnd(&dc);
  return 1;
}
