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
 * $Id: FXFrame.cpp,v 1.37 2006/01/22 17:58:27 fox Exp $                      *
 *****************************************************************************/

/* Frame Window object */

#include <stdio.h>
#include <stdlib.h>

#include "fxapp.h"
#include "fxdc.h"
#include "fxdrv.h"
#include "fxframe.h"

/*
 *   Notes:
 *   - This really should become the base class for everything that has
 *     a border.
 */

static long dkFramePaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static int dkFrameGetDefaultWidth(struct dkWindow *w);
static int dkFrameGetDefaultHeight(struct dkWindow *w);

/* Frame styles */
#define FRAME_MASK        (FRAME_SUNKEN | FRAME_RAISED | FRAME_THICK)

/* Map */
static struct dkMapEntry dkFrameMapEntry[] = {
  FXMAPFUNC(SEL_PAINT, 0, dkFramePaint)
};

/* Object implementation */
static struct dkMetaClass dkFrameMetaClass = {
  "dkFrame", dkFrameMapEntry, sizeof(dkFrameMapEntry) / sizeof(dkFrameMapEntry[0]), sizeof(struct dkMapEntry)
};

/* Handle message */
long
dkFrame_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;
  DKSelector sel = DKSEL(selhi, sello);

  me = DKMetaClassSearch(&dkFrameMetaClass, sel);
  return me ? me->func(pthis, obj, selhi, sello, data) : dkWindow_handle(pthis, obj, selhi, sello, data);
}

struct dkFrame * dkFrameNew(struct dkWindow *p, DKuint opts)
{
  struct dkFrame *ret;

  ret = fx_alloc(sizeof(struct dkFrame));

  DtkFrameInit(ret, p, opts, 0, 0, 0, 0, DEFAULT_PAD, DEFAULT_PAD, DEFAULT_PAD, DEFAULT_PAD);

  return ret;
}

void DtkFrameInit(struct dkFrame *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb)
{
  dkWindowChildInit((struct dkWindow *)pthis, p, opts, x, y, w, h);
  ((struct dkWindow *)pthis)->flags |= FLAG_SHOWN;
  ((struct dkWindow *)pthis)->backColor = ((struct dkWindow *)pthis)->app->baseColor;
  pthis->baseColor = ((struct dkWindow *)pthis)->app->baseColor;
  pthis->hiliteColor = ((struct dkWindow *)pthis)->app->hiliteColor;
  pthis->shadowColor = ((struct dkWindow *)pthis)->app->shadowColor;
  pthis->borderColor = ((struct dkWindow *)pthis)->app->borderColor;
  pthis->padtop = pt;
  pthis->padbottom = pb;
  pthis->padleft = pl;
  pthis->padright = pr;
  pthis->border = (((struct dkWindow *)pthis)->options & FRAME_THICK) ? 2 : (((struct dkWindow *)pthis)->options & (FRAME_SUNKEN | FRAME_RAISED)) ? 1 : 0;

  /* Setup vtbl by replacing dkWindow vtbl components with dkFrame ones */
  ((struct dkObject *)pthis)->handle = dkFrame_handle;
  ((struct dkWindow *)pthis)->getDefaultWidth = dkFrameGetDefaultWidth;
  ((struct dkWindow *)pthis)->getDefaultHeight = dkFrameGetDefaultHeight;
}

/* Get default width */
static int dkFrameGetDefaultWidth(struct dkWindow *w)
{
  struct dkFrame *f = (struct dkFrame *)w;
  return f->padleft + f->padright + (f->border << 1);
}

/* Get default height */
static int dkFrameGetDefaultHeight(struct dkWindow *w)
{
  struct dkFrame *f = (struct dkFrame *)w;
  return f->padtop + f->padbottom + (f->border << 1);
}

void dkFrameDrawBorderRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  dtkDrvDCSetForeground(dc, f->borderColor);
  dtkDrvDCFillRectangle(dc, x, y, w - 1, h - 1);
}

void dkFrameDrawRaisedRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (0 < w && 0 < h) {
    dtkDrvDCSetForeground(dc, f->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    dtkDrvDCSetForeground(dc, f->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
  }
}

void dkFrameDrawSunkenRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (0 < w && 0 < h) {
    dtkDrvDCSetForeground(dc, f->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
    dtkDrvDCSetForeground(dc, f->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
  }
}

void dkFrameDrawRidgeRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (0 < w && 0 < h) {
    dtkDrvDCSetForeground(dc, f->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
    dtkDrvDCSetForeground(dc, f->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    if (1 < w && 1 < h) {
      dtkDrvDCSetForeground(dc, f->hiliteColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
      dtkDrvDCSetForeground(dc, f->shadowColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
    }
  }
}

void dkFrameDrawGrooveRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (0 < w && 0 < h) {
    dtkDrvDCSetForeground(dc, f->shadowColor);
    dtkDrvDCFillRectangle(dc, x, y, w, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h);
    dtkDrvDCSetForeground(dc, f->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    if(1<w && 1<h){
      dtkDrvDCSetForeground(dc, f->shadowColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
      dtkDrvDCSetForeground(dc, f->hiliteColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
    }
  }
}

void dkFrameDrawDoubleSunkenRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
	if (0 < w && 0 < h) {
		dtkDrvDCSetForeground(dc, f->hiliteColor);
		dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
		dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
		dtkDrvDCSetForeground(dc, f->shadowColor);
		dtkDrvDCFillRectangle(dc, x, y, w - 1, 1);
		dtkDrvDCFillRectangle(dc, x, y, 1, h - 1);
		if (1 < w && 1 < h) {
			dtkDrvDCSetForeground(dc, f->borderColor);
			dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 3, 1);
			dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 3);
			dtkDrvDCSetForeground(dc, f->baseColor);
			dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
			dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
		}
	}
}

void dkFrameDrawDoubleRaisedRectangle(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  if (0 < w && 0 < h) {
    dtkDrvDCSetForeground(dc, f->borderColor);
    dtkDrvDCFillRectangle(dc, x, y + h - 1, w, 1);
    dtkDrvDCFillRectangle(dc, x + w - 1, y, 1, h);
    dtkDrvDCSetForeground(dc, f->hiliteColor);
    dtkDrvDCFillRectangle(dc, x, y, w - 1, 1);
    dtkDrvDCFillRectangle(dc, x, y, 1, h - 1);
    if (1 < w && 1 < h) {
      dtkDrvDCSetForeground(dc, f->baseColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + 1, y + 1, 1, h - 2);
      dtkDrvDCSetForeground(dc, f->shadowColor);
      dtkDrvDCFillRectangle(dc, x + 1, y + h - 2, w - 2, 1);
      dtkDrvDCFillRectangle(dc, x + w - 2, y + 1, 1, h - 2);
    }
  }
}

/* Draw border */
void dkFrameDrawFrame(struct dkFrame *f, struct dtkDC *dc, int x, int y, int w, int h)
{
  switch (((struct dkWindow *)f)->options & FRAME_MASK) {
  case FRAME_LINE: dkFrameDrawBorderRectangle(f, dc, x, y, w, h); break;
  case FRAME_SUNKEN: dkFrameDrawSunkenRectangle(f, dc, x, y, w, h); break;
  case FRAME_RAISED: dkFrameDrawRaisedRectangle(f, dc, x, y, w, h); break;
  case FRAME_GROOVE: dkFrameDrawGrooveRectangle(f, dc, x, y, w, h); break;
  case FRAME_RIDGE: dkFrameDrawRidgeRectangle(f, dc, x, y, w, h); break;
  case FRAME_SUNKEN | FRAME_THICK: dkFrameDrawDoubleSunkenRectangle(f, dc, x, y, w, h); break;
  case FRAME_RAISED | FRAME_THICK: dkFrameDrawDoubleRaisedRectangle(f, dc, x, y, w, h); break;
  }
}

/* A helper function to simulate C++ virtual functions */
void dkFrameCreate(void *pthis)
{
  DtkCreateWindow(pthis);
}

/* Handle repaint */
static long dkFramePaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	struct dtkDC dc;
	struct dkFrame *f = (struct dkFrame *)pthis;

	dtkDrvDCEventSetup(&dc, (struct dkWindow *)f, (struct dkEvent *)data);
	dtkDrvDCSetForeground(&dc, ((struct dkWindow *)f)->backColor);
	dtkDrvDCFillRectangle(&dc, f->border, f->border, ((struct dkWindow *)f)->width - (f->border << 1), ((struct dkWindow *)f)->height - (f->border << 1));
	dkFrameDrawFrame(f, &dc, 0, 0, ((struct dkWindow *)f)->width, ((struct dkWindow *)f)->height);
	dkDCEnd(&dc);
	return 1;
}
