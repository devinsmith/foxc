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
 * $Id: FXLabel.cpp,v 1.59.2.1 2006/12/11 15:57:26 fox Exp $                  *
 *****************************************************************************/

/* Label Window object */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fxapp.h"
#include "fxacceltable.h"
#include "fxdc.h"
#include "fxdrv.h"
#include "fxlabel.h"
#include "fxstring.h"

/*
 *   Notes:
 *   - When changing icon/font/etc, we should only recalc and update when it's different.
 *   - When text changes, do we delete the hot key, or parse it from the new label?
 *   - It makes sense for certain ``passive'' widgets such as labels to have onUpdate;
 *     for example, to show/hide/whatever based on changing data structures.
 */

static long dkLabelPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static int dkLabelGetDefaultWidth(struct dkWindow *w);
static int dkLabelGetDefaultHeight(struct dkWindow *w);

void dkLabelCreate(void *pthis);

/* Map */
static struct dkMapEntry dkLabelMapEntry[] = {
	FXMAPFUNC(SEL_PAINT, 0, dkLabelPaint)
};

/* Object implementation */
static struct dkMetaClass dkLabelMetaClass = {
	"dkLabel", dkLabelMapEntry, sizeof(dkLabelMapEntry) / sizeof(dkLabelMapEntry[0]), sizeof(struct dkMapEntry)
};

/* Handle message */
long
dkLabel_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	struct dkMapEntry *me;

	me = DKMetaClassSearch(&dkLabelMetaClass, DKSEL(selhi, sello));
	return me ? me->func(pthis, obj, selhi, sello, data) : dkFrame_handle(pthis, obj, selhi, sello, data);
}

struct dkLabel *
dkLabelNew(struct dkWindow *p, char *title)
{
	struct dkLabel *ret;

	ret = malloc(sizeof(struct dkLabel));
	dkLabelInit(ret, p, title, LABEL_NORMAL);

	return ret;
}

void dkLabelInit(struct dkLabel *pthis, struct dkWindow *p, char *title, DKuint opts)
{
  DtkFrameInit((struct dkFrame *)pthis, p, opts, 0, 0, 0, 0, DEFAULT_PAD, DEFAULT_PAD, DEFAULT_PAD, DEFAULT_PAD);
  ((struct dkWindow *)pthis)->flags |= FLAG_ENABLED;
  pthis->label = fx_stripHotKey(title);
  pthis->font = ((struct dkWindow *)pthis)->app->normalFont;
  pthis->textColor = ((struct dkWindow *)pthis)->app->foreColor;
  pthis->hotoff = fx_findHotKey(title);

  ((struct dkWindow *)pthis)->create = dkLabelCreate;
  ((struct dkObject *)pthis)->handle = dkLabel_handle;
  ((struct dkWindow *)pthis)->getDefaultWidth = dkLabelGetDefaultWidth;
  ((struct dkWindow *)pthis)->getDefaultHeight = dkLabelGetDefaultHeight;
}

void dkLabelCreate(void *pthis)
{
  struct dkLabel *l = (struct dkLabel *)pthis;

  dkFrameCreate(pthis);
  dkFontCreate(l->font);
}

/* Get default width */
static int dkLabelGetDefaultWidth(struct dkWindow *win)
{
	int tw = 0, iw = 0, s = 0, w;
	struct dkLabel *l = (struct dkLabel *)win;

	if (l->label != NULL) {
		tw = dkLabelWidth(l, l->label);
	}
	if (iw && tw) s = 4;
	if (!(((struct dkWindow *)l)->options & (ICON_AFTER_TEXT | ICON_BEFORE_TEXT)))
		w = FXMAX(tw, iw);
	else
		w = tw + iw + s;

	return w + ((struct dkFrame *)l)->padleft + ((struct dkFrame *)l)->padright + (((struct dkFrame *)l)->border << 1);
}

/* Get default height */
static int dkLabelGetDefaultHeight(struct dkWindow *win)
{
	struct dkLabel *l = (struct dkLabel *)win;
	int th = 0, ih = 0, h;

	if (l->label != NULL) {
		th = dkLabelHeight(l, l->label);
	}
	if (!(((struct dkWindow *)l)->options & (ICON_ABOVE_TEXT | ICON_BELOW_TEXT)))
		h = FXMAX(th, ih);
	else
		h = th + ih;

	return h + ((struct dkFrame *)l)->padtop + ((struct dkFrame *)l)->padbottom + (((struct dkFrame *)l)->border << 1);
}

/* Get height of multi-line label */
int dkLabelHeight(struct dkLabel *l, char *text)
{
	int beg, end;
	int th = 0;
	int len;

	beg = 0;
	len = strlen(text);
	do {
		end = beg;
		while (end < len && text[end] != '\n') end++;
		th += dkFontGetFontHeight(l->font);
	} while (end < len);
	return th;
}

/* Get width of multi-line label */
int dkLabelWidth(struct dkLabel *l, char *text)
{
	int beg, end;
	int w, tw = 0;
	int len;

	beg = 0;
	len = strlen(text);

	do {
		end = beg;
		while (end < len && text[end] != '\n') end++;
		if ((w = dkFontGetTextWidth(l->font, &text[beg], end - beg)) > tw) tw = w;
		beg = end + 1;
	} while (end < len);
	return tw;
}

/* Justify stuff in x-direction */
void dkLabelJustX(struct dkLabel *label, int *tx, int *ix, int tw, int iw)
{
  struct dkFrame *l = (struct dkFrame *)label;
	int s = 0;

	if (iw && tw) s = 4;
	if ((((struct dkWindow *)l)->options & JUSTIFY_LEFT) && (((struct dkWindow *)l)->options & JUSTIFY_RIGHT)) {
		if (((struct dkWindow *)l)->options & ICON_BEFORE_TEXT) {
			*ix = l->padleft + l->border;
			*tx = ((struct dkWindow *)l)->width - l->padright - l->border - tw;
		} else if (((struct dkWindow *)l)->options & ICON_AFTER_TEXT) {
			*tx = l->padleft + l->border;
			*ix = ((struct dkWindow *)l)->width - l->padright - l->border - iw;
		} else {
			*ix = l->border + l->padleft;
			*tx = l->border + l->padleft;
		}
	} else if (((struct dkWindow *)l)->options & JUSTIFY_LEFT) {
		if (((struct dkWindow *)l)->options & ICON_BEFORE_TEXT) {
			*ix = l->padleft + l->border;
			*tx = *ix + iw + s;
		} else if (((struct dkWindow *)l)->options & ICON_AFTER_TEXT) {
			*tx = l->padleft + l->border;
			*ix = *tx + tw + s;
		} else {
			*ix = l->border + l->padleft;
			*tx = l->border + l->padleft;
		}
	} else if (((struct dkWindow *)l)->options & JUSTIFY_RIGHT) {
		if (((struct dkWindow *)l)->options & ICON_BEFORE_TEXT) {
			*tx = ((struct dkWindow *)l)->width - l->padright - l->border - tw;
			*ix = *tx - iw - s;
		} else if (((struct dkWindow *)l)->options & ICON_AFTER_TEXT) {
			*ix = ((struct dkWindow *)l)->width - l->padright - l->border - iw;
			*tx = *ix - tw - s;
		} else {
			*ix = ((struct dkWindow *)l)->width - l->padright - l->border - iw;
			*tx = ((struct dkWindow *)l)->width - l->padright - l->border - tw;
		}
	} else {
		if (((struct dkWindow *)l)->options & ICON_BEFORE_TEXT) {
			*ix = l->border + l->padleft + (((struct dkWindow *)l)->width - l->padleft - l->padright - (l->border << 1) - tw - iw - s) / 2;
			*tx = *ix + iw + s;
		} else if (((struct dkWindow *)l)->options & ICON_AFTER_TEXT) {
			*tx = l->border + l->padleft + (((struct dkWindow *)l)->width - l->padleft - l->padright - (l->border << 1) - tw - iw - s) / 2;
			*ix = *tx + tw + s;
		} else {
			*ix = l->border + l->padleft + (((struct dkWindow *)l)->width - l->padleft - l->padright - (l->border << 1) - iw) / 2;
			*tx = l->border + l->padleft + (((struct dkWindow *)l)->width - l->padleft - l->padright - (l->border << 1) - tw) / 2;
		}
	}
}

/* Justify stuff in y-direction */
void dkLabelJustY(struct dkLabel *label, int *ty, int *iy, int th, int ih)
{
  struct dkFrame *l = (struct dkFrame *)label;

  if ((((struct dkWindow *)l)->options & JUSTIFY_TOP) && (((struct dkWindow *)l)->options & JUSTIFY_BOTTOM)) {
		if (((struct dkWindow *)l)->options & ICON_ABOVE_TEXT) {
			*iy = l->padtop + l->border;
			*ty = ((struct dkWindow *)l)->height - l->padbottom - l->border - th;
		} else if (((struct dkWindow *)l)->options & ICON_BELOW_TEXT) {
			*ty = l->padtop + l->border;
			*iy = ((struct dkWindow *)l)->height - l->padbottom - l->border - ih;
		} else {
			*iy = l->border + l->padtop;
			*ty = l->border + l->padtop;
		}
	} else if (((struct dkWindow *)l)->options & JUSTIFY_TOP) {
		if (((struct dkWindow *)l)->options & ICON_ABOVE_TEXT) {
			*iy = l->padtop + l->border;
			*ty = *iy + ih;
		} else if (((struct dkWindow *)l)->options & ICON_BELOW_TEXT) {
			*ty = l->padtop + l->border;
			*iy = *ty + th;
		} else {
			*iy = l->border + l->padtop;
			*ty = l->border + l->padtop;
		}
	} else if (((struct dkWindow *)l)->options & JUSTIFY_BOTTOM) {
		if (((struct dkWindow *)l)->options & ICON_ABOVE_TEXT) {
			*ty = ((struct dkWindow *)l)->height - l->padbottom - l->border - th;
			*iy = *ty - ih;
		} else if (((struct dkWindow *)l)->options & ICON_BELOW_TEXT) {
			*iy = ((struct dkWindow *)l)->height - l->padbottom - l->border - ih;
			*ty = *iy - th;
		} else {
			*iy = ((struct dkWindow *)l)->height - l->padbottom - l->border - ih;
			*ty = ((struct dkWindow *)l)->height - l->padbottom - l->border - th;
		}
	} else {
		if (((struct dkWindow *)l)->options & ICON_ABOVE_TEXT) {
			*iy = l->border + l->padtop + (((struct dkWindow *)l)->height - l->padbottom - l->padtop - (l->border << 1) - th - ih) / 2;
			*ty = *iy + ih;
		} else if (((struct dkWindow *)l)->options & ICON_BELOW_TEXT) {
			*ty = l->border + l->padtop + (((struct dkWindow *)l)->height - l->padbottom - l->padtop - (l->border << 1) - th - ih) / 2;
			*iy = *ty + th;
		} else {
			*iy = l->border + l->padtop + (((struct dkWindow *)l)->height - l->padbottom - l->padtop - (l->border << 1) - ih) / 2;
			*ty = l->border + l->padtop + (((struct dkWindow *)l)->height - l->padbottom - l->padtop - (l->border << 1) - th) / 2;
		}
	}
}

/* Draw multi-line label, with underline for hotkey */
void dkLabelDrawLabel(struct dkLabel *l, struct dtkDC *dc, char *text, int hot, int tx, int ty, int tw, int th)
{
	int beg, end;
	int xx, yy;
	int len;

	len = strlen(text);
	yy = ty + dkFontGetFontAscent(l->font);
	beg = 0;

	do {
		end = beg;
		while (end < len && text[end] != '\n') end++;
		if (((struct dkWindow *)l)->options & JUSTIFY_LEFT) xx = tx;
		else if (((struct dkWindow *)l)->options & JUSTIFY_RIGHT)
			xx = tx + tw - dkFontGetTextWidth(l->font, &text[beg], end - beg);
		else xx = tx + (tw - dkFontGetTextWidth(l->font, &text[beg], end - beg)) / 2;
		dkDCDrawText(dc, xx, yy, &text[beg], end - beg);

    if (beg <= hot && hot < end) {
      dtkDrvDCFillRectangle(dc, xx + dkFontGetTextWidth(l->font, &text[beg], hot-beg), yy + 1, dkFontGetTextWidth(l->font, &text[hot], dk_wclen(&text[hot])), 1);
    }
		yy += dkFontGetFontHeight(l->font);
		beg = end + 1;
	} while (end < len);
}

/* Handle repaint */
static long
dkLabelPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	struct dtkDC dc;
	struct dkLabel *l = (struct dkLabel *)pthis;
	int tw = 0, th = 0, iw = 0, ih = 0, tx, ty, ix, iy;

	dtkDrvDCEventSetup(&dc, (struct dkWindow *)l, (struct dkEvent *)data);
	dtkDrvDCSetForeground(&dc, ((struct dkWindow *)l)->backColor);
	dtkDrvDCFillRectangle(&dc, 0, 0, ((struct dkWindow *)l)->width,
	    ((struct dkWindow *)l)->height);

	if (l->label) {
		tw = dkLabelWidth(l, l->label);
		th = dkLabelHeight(l, l->label);
	}

	/* Do some justification */
	dkLabelJustX(l, &tx, &ix, tw, iw);
	dkLabelJustY(l, &ty, &iy, th, ih);

	if (l->label) {
		dkDCWindowSetFont(&dc, l->font);
		// no support for disabled right now
		dtkDrvDCSetForeground(&dc, l->textColor);
		dkLabelDrawLabel(l, &dc, l->label, l->hotoff, tx, ty, tw, th);
	}

	dkFrameDrawFrame((struct dkFrame *)l, &dc, 0, 0, ((struct dkWindow *)l)->width, ((struct dkWindow *)l)->height);
	dkDCEnd(&dc);
	return 1;
}
