/******************************************************************************
 *                                                                            *
 *           H o r i z o n t a l   C o n t a i n e r   W i d g e t            *
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
 * $Id: FXHorizontalFrame.cpp,v 1.28 2006/01/22 17:58:31 fox Exp $            *
 *****************************************************************************/

#include "fxhorizontalframe.h"

/*
  Notes:
  - Filled items shrink as well as stretch.
  - Stretch is proportional to default size; this way, at default size,
    it is exactly correct.
  - Tabbing order takes widget layout into account
*/

static int dkHorizontalFrame_getDefaultWidth(struct dkWindow *win);
static int dkHorizontalFrame_getDefaultHeight(struct dkWindow *win);
void dkHorizontalFrame_layout(struct dkWindow *win);
void dkHorizontalFrameInit(struct dkWindow *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs);

#if 0
/* Map */
static struct dkMapEntry dkPackerMapEntry[] = {
  DKMAPFUNC(SEL_PAINT, 0, dkPacker_onPaint)
};

/* Object implementation */
static struct dkMetaClass dkPackerMetaClass = {
  "dkPacker", dkPackerMapEntry, sizeof(dkPackerMapEntry) / sizeof(dkPackerMapEntry[0]), sizeof(struct dkMapEntry)
};

/* Handle message */
long dkPacker_handle(void *pthis, struct dkObject *obj, DKSelector sel, void *data)
{
  struct dkMapEntry *me;

  me = DKMetaClassSearch(&dkPackerMetaClass, sel);
  return me ? me->func(pthis, obj, sel, data) : dkComposite_handle(pthis, obj, sel, data);
}
#endif

struct dkWindow *dkHorizontalFrameNew(struct dkWindow *p, DKuint opts)
{
  struct dkWindow *hf;

  hf = malloc(sizeof(struct dkWindow));
  hf->wExtra = malloc(sizeof(struct dkPacker));

  dkHorizontalFrameInit(hf, p, opts, 0, 0, 0, 0, DEFAULT_SPACING, DEFAULT_SPACING, DEFAULT_SPACING, DEFAULT_SPACING, DEFAULT_SPACING, DEFAULT_SPACING);

  return hf;
}

/* Make a horizontal one */
void dkHorizontalFrameInit(struct dkWindow *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs)
{
  dkWindowChildInit(pthis, p, opts, x, y, w, h);
  dkPackerSetup((struct dkPacker *)pthis->wExtra, pthis, pl, pr, pt, pb, hs, vs);

  /* Setup the vtbl */
//  ((struct dkObject *)pthis)->handle = dkPacker_handle;
  pthis->getDefaultWidth = dkHorizontalFrame_getDefaultWidth;
  pthis->getDefaultHeight = dkHorizontalFrame_getDefaultHeight;
  pthis->layout = dkHorizontalFrame_layout;
}

/* Compute minimum width based on child layout hints */
static int dkHorizontalFrame_getDefaultWidth(struct dkWindow *win)
{
  int w, wcum, wmax, mw;
  struct dkWindow *child;
  struct dkPacker *pack;
  DKuint hints;
  wmax = wcum = mw = 0;

  pack = (struct dkPacker *)win->wExtra;

  if (win->options & PACK_UNIFORM_WIDTH) mw = dkCompositeMaxChildWidth(win);
  for (child = win->first; child; child = child->next) {
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
        if (child->next) wcum += pack->hspacing;
        wcum += w;
      }
      else {
        if (w > wcum) wcum = w;
      }
    }
  }
  wcum += pack->padleft + pack->padright + (pack->border << 1);
  return FXMAX(wcum, wmax);
}

/* Compute minimum height based on child layout hints */
static int dkHorizontalFrame_getDefaultHeight(struct dkWindow *win)
{
  int h, hcum, hmax, mh;
  struct dkWindow *child;
  struct dkPacker *pack;
  DKuint hints;
  hmax = hcum = mh = 0;

  pack = (struct dkPacker *)win->wExtra;

  if (win->options & PACK_UNIFORM_HEIGHT) mh = dkCompositeMaxChildHeight(win);
  for (child = win->first; child; child = child->next) {
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
        if (child->next) hcum += pack->vspacing;
        hcum += h;
      }
      else {
        if (h > hcum) hcum = h;
      }
    }
  }
  hcum += pack->padtop + pack->padbottom + (pack->border << 1);
  return FXMAX(hcum, hmax);
}

/* Recalculate layout */
void dkHorizontalFrame_layout(struct dkWindow *win)
{
  int left, right, top, bottom, remain, extra_space, total_space, t, x, y, w, h;
  struct dkWindow *child;
  struct dkPacker *pack;
  int sumexpand = 0;
  int numexpand = 0;
  int mw = 0, mh = 0;
  int e = 0;
  DKuint hints;

  pack = (struct dkPacker *)win->wExtra;
  /* Placement rectangle; right/bottom non-inclusive */
  left = pack->border + pack->padleft;
  right = win->width - pack->border - pack->padright;
  top = pack->border + pack->padtop;
  bottom = win->height - pack->border - pack->padbottom;
  remain = right - left;

  /* Get maximum child size */
  if (win->options & PACK_UNIFORM_WIDTH) mw = dkCompositeMaxChildWidth(win);
  if (win->options & PACK_UNIFORM_HEIGHT) mh = dkCompositeMaxChildHeight(win);

  /* Find number of paddable children and total width */
  for (child = win->first; child; child = child->next) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      if (!((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X))) {     // LAYOUT_FIX_X
        if (hints & LAYOUT_FIX_WIDTH) w = child->width;
        else if (win->options & PACK_UNIFORM_WIDTH) w = mw;
        else w = child->getDefaultWidth(child);
        /* FXASSERT(w>=0); */
        if ((hints & LAYOUT_CENTER_X) || ((hints & LAYOUT_FILL_X) && !(hints & LAYOUT_FIX_WIDTH))) {
          sumexpand += w;
          numexpand += 1;
        }
        else {
          remain -= w;
        }
        remain -= pack->hspacing;
      }
    }
  }

  /* Child spacing correction */
  remain += pack->hspacing;

  /* Do the layout */
  for (child = win->first; child; child = child->next) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);

      /* Determine child height */
      if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
      else if (win->options & PACK_UNIFORM_HEIGHT) h = mh;
      else if (hints & LAYOUT_FILL_Y) h = bottom - top;
      else h = child->getDefaultHeight(child);

      /* Determine child y-position */
      if ((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y)) y = child->ypos;
      else if (hints & LAYOUT_CENTER_Y) y = top + (bottom - top - h) / 2;
      else if (hints & LAYOUT_BOTTOM) y = bottom - h;
      else y = top;

      /* Layout child in X */
      x = child->xpos;
      if (hints & LAYOUT_FIX_WIDTH) w = child->width;
      else if (win->options & PACK_UNIFORM_WIDTH) w = mw;
      else w = child->getDefaultWidth(child);
      if (!((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X))) {     /* LAYOUT_FIX_X */
        extra_space = 0;
        total_space = 0;
        if ((hints & LAYOUT_FILL_X) && !(hints & LAYOUT_FIX_WIDTH)) {
          if (sumexpand > 0) {                            /* Divide space proportionally to width */
            t = w * remain;
            //FXASSERT(sumexpand>0);
            w = t / sumexpand;
            e += t % sumexpand;
            if (e >= sumexpand) { w++; e-=sumexpand; }
          }
          else {                                       /* Divide the space equally */
            //FXASSERT(numexpand>0);
            w = remain / numexpand;
            e += remain % numexpand;
            if (e >= numexpand) { w++; e-=numexpand; }
          }
        }
        else if (hints & LAYOUT_CENTER_X) {
          if (sumexpand > 0) {                            /* Divide space proportionally to width */
            t = w * remain;
            //FXASSERT(sumexpand>0);
            total_space = t / sumexpand - w;
            e += t % sumexpand;
            if (e >= sumexpand) { total_space++; e-=sumexpand; }
          }
          else {                                       /* Divide the space equally */
//            FXASSERT(numexpand>0);
            total_space = remain / numexpand - w;
            e += remain % numexpand;
            if (e >= numexpand) { total_space++; e-=numexpand; }
          }
          extra_space = total_space / 2;
        }
        if (hints & LAYOUT_RIGHT) {
          x = right - w - extra_space;
          right = right - w - pack->hspacing - total_space;
        }
        else {
          x = left + extra_space;
          left = left + w + pack->hspacing + total_space;
        }
      }
      child->position(child, x, y, w, h);
    }
  }
  win->flags &= ~FLAG_DIRTY;
}

