/******************************************************************************
 *                                                                            *
 *              V e r t i c a l   C o n t a i n e r   W i d g e t             *
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
 * $Id: FXVerticalFrame.cpp,v 1.28 2006/01/22 17:58:51 fox Exp $              *
 *****************************************************************************/

#include "fxverticalframe.h"

/*
  Notes:
  - Filled items shrink as well as stretch.
  - Stretch is proportional to default size; this way, at default size,
    it is exactly correct.
  - Tabbing order takes widget layout into account
*/

static int dkVerticalFrame_getDefaultWidth(struct dkWindow *win);
static int dkVerticalFrame_getDefaultHeight(struct dkWindow *win);
void dkVerticalFrame_layout(struct dkWindow *win);
void dkVerticalFrameInit(struct dkWindow *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs);

struct dkWindow *
dkVerticalFrameNew(struct dkWindow *p, DKuint opts)
{
  struct dkWindow *vf;

  vf = malloc(sizeof(struct dkWindow));
  vf->wExtra = malloc(sizeof(struct dkPacker));

  dkVerticalFrameInit(vf, p, opts, 0, 0, 0, 0, DEFAULT_SPACING,
    DEFAULT_SPACING, DEFAULT_SPACING, DEFAULT_SPACING, DEFAULT_SPACING,
    DEFAULT_SPACING);

  return vf;
}

struct dkWindow *
dkVerticalFrameNewEx(struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs)
{
  struct dkWindow *vf;

  vf = malloc(sizeof(struct dkWindow));
  vf->wExtra = malloc(sizeof(struct dkPacker));

  dkVerticalFrameInit(vf, p, opts, x, y, w, h, pl, pr, pt, pb, hs, vs);

  return vf;
}

/* Make a vertical one */
void
dkVerticalFrameInit(struct dkWindow *pthis, struct dkWindow *p, DKuint opts,
    int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs)
{
  dkWindowChildInit(pthis, p, opts, x, y, w, h);
  dkPackerSetup((struct dkPacker *)pthis->wExtra, pthis, pl, pr, pt, pb, hs, vs);

  /* Setup the vtbl */
  pthis->getDefaultWidth = dkVerticalFrame_getDefaultWidth;
  pthis->getDefaultHeight = dkVerticalFrame_getDefaultHeight;
  pthis->layout = dkVerticalFrame_layout;
}

/* Compute minimum width based on child layout hints */
static int
dkVerticalFrame_getDefaultWidth(struct dkWindow *win)
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
      else {
        if (w > wcum) wcum = w;
      }
    }
  }
  wcum += pack->padleft + pack->padright + (pack->border << 1);
  return FXMAX(wcum, wmax);
}

/* Compute minimum height based on child layout hints */
static int
dkVerticalFrame_getDefaultHeight(struct dkWindow *win)
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
      else {
        if (hcum) hcum += pack->vspacing;
        hcum += h;
      }
    }
  }
  hcum += pack->padtop + pack->padbottom + (pack->border << 1);
  return FXMAX(hcum, hmax);
}

/* Recalculate layout */
void
dkVerticalFrame_layout(struct dkWindow *win)
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
  remain = bottom - top;

  /* Get maximum child size */
  if (win->options & PACK_UNIFORM_WIDTH) mw = dkCompositeMaxChildWidth(win);
  if (win->options & PACK_UNIFORM_HEIGHT) mh = dkCompositeMaxChildHeight(win);

  /* Find number of paddable children and total height */
  for (child = win->first; child; child = child->next) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);
      if (!((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y))) {     /* LAYOUT_FIX_Y */
        if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
        else if (win->options & PACK_UNIFORM_HEIGHT) h = mh;
        else h = child->getDefaultHeight(child);
        /* FXASSERT(h>=0); */
        if ((hints & LAYOUT_CENTER_Y) || ((hints & LAYOUT_FILL_Y) && !(hints & LAYOUT_FIX_HEIGHT))) {
          sumexpand += h;
          numexpand += 1;
        }
        else {
          remain -= h;
        }
        remain -= pack->vspacing;
      }
    }
  }

  /* Child spacing correction */
  remain += pack->vspacing;

  /* Do the layout */
  for (child = win->first; child; child = child->next) {
    if (dkWindowIsShown(child)) {
      hints = dkWindowGetLayoutHints(child);

      /* Determine child width */
      if (hints & LAYOUT_FIX_WIDTH) w = child->width;
      else if (win->options & PACK_UNIFORM_WIDTH) w = mw;
      else if (hints & LAYOUT_FILL_X) w = right - left;
      else w = child->getDefaultWidth(child);

      /* Determine child x-position */
      if ((hints & LAYOUT_RIGHT) && (hints & LAYOUT_CENTER_X)) x = child->xpos;
      else if (hints & LAYOUT_CENTER_X) x = left + (right - left - w) / 2;
      else if (hints & LAYOUT_RIGHT) x = right - w;
      else x = left;

      /* Layout child in Y */
      y = child->ypos;
      if (hints & LAYOUT_FIX_HEIGHT) h = child->height;
      else if (win->options & PACK_UNIFORM_HEIGHT) h = mh;
      else h = child->getDefaultHeight(child);
      if (!((hints & LAYOUT_BOTTOM) && (hints & LAYOUT_CENTER_Y))) {     /* LAYOUT_FIX_Y */
        extra_space = 0;
        total_space = 0;
        if ((hints & LAYOUT_FILL_Y) && !(hints & LAYOUT_FIX_HEIGHT)) {
          if (sumexpand > 0) {                            /* Divide space proportionally to height */
            t = h * remain;
            //FXASSERT(sumexpand>0);
            h = t / sumexpand;
            e += t % sumexpand;
            if (e >= sumexpand) { h++; e-=sumexpand; }
          }
          else {                                       /* Divide the space equally */
            //FXASSERT(numexpand>0);
            h = remain / numexpand;
            e += remain % numexpand;
            if (e >= numexpand) { h++; e-=numexpand; }
          }
        }
        else if (hints & LAYOUT_CENTER_Y) {
          if (sumexpand > 0) {                            /* Divide space proportionally to height */
            t = h * remain;
            //FXASSERT(sumexpand>0);
            total_space = t / sumexpand - h;
            e += t % sumexpand;
            if (e >= sumexpand) { total_space++; e-=sumexpand; }
          }
          else {                                       /* Divide the space equally */
//          FXASSERT(numexpand>0);
            total_space = remain / numexpand - h;
            e += remain % numexpand;
            if (e >= numexpand) { total_space++; e-=numexpand; }
          }
          extra_space = total_space / 2;
        }
        if (hints & LAYOUT_BOTTOM) {
          y = bottom - h - extra_space;
          bottom = bottom - h - pack->hspacing - total_space;
        }
        else {
          y = top + extra_space;
          top = top + h + pack->vspacing + total_space;
        }
      }
      child->position(child, x, y, w, h);
    }
  }
  win->flags &= ~FLAG_DIRTY;
}

