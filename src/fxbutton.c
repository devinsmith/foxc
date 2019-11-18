/******************************************************************************
 *                                                                            *
 *                        B u t t o n   W i d g e t                           *
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
 * $Id: FXButton.cpp,v 1.67 2006/01/22 17:58:18 fox Exp $                     *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "fxdc.h"
#include "fxdrv.h"
#include "fxbutton.h"

/*
  Notes:
  - Use flags for button instead of a whole integer
  - Add ``flat'' toolbar style also
  - Need check-style also (stay in when pressed, pop out when unpressed).
  - Who owns the icon(s)?
  - Arrow buttons should auto-repeat with a timer of some kind
  - "&Label\tTooltip\tHelptext\thttp://server/application/helponitem.html"
  - CheckButton should send SEL_COMMAND.
  - Default button mode:- should somehow get focus.
  - Add button multiple-click translations elsewhere
  - Button should be able to behave like a check (radio) button.
  - Need to draw ``around'' the icon etc. So it doesn't flash to background.
*/

/* Button styles */
#define BUTTON_MASK (BUTTON_AUTOGRAY|BUTTON_AUTOHIDE|BUTTON_TOOLBAR|BUTTON_DEFAULT|BUTTON_INITIAL)

static long dkButtonPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnUpdate(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnEnter(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnLeave(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnFocusIn(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnFocusOut(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnUngrabbed(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnLeftBtnPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static long dkButtonOnLeftBtnRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);
static int dkButton_canFocus(void);
static void dkButton_setFocus(struct dkWindow *w);
static void dkButton_killFocus(struct dkWindow *w);
static void dkButtonSetDefault(struct dkWindow *w, DKbool enable);
static int dkButtonPaint2(struct dkWindow *pthis, struct dkEvent *data);

/* Map */
static struct dkMapEntry dkButtonMapEntry[] = {
  FXMAPFUNC(SEL_PAINT, 0, dkButtonPaint),
  FXMAPFUNC(SEL_UPDATE, 0, dkButtonOnUpdate),
  FXMAPFUNC(SEL_ENTER, 0, dkButtonOnEnter),
  FXMAPFUNC(SEL_LEAVE, 0, dkButtonOnLeave),
  FXMAPFUNC(SEL_FOCUSIN, 0, dkButtonOnFocusIn),
  FXMAPFUNC(SEL_FOCUSOUT, 0, dkButtonOnFocusOut),
  FXMAPFUNC(SEL_UNGRABBED, 0, dkButtonOnUngrabbed),
  FXMAPFUNC(SEL_LEFTBUTTONPRESS, 0, dkButtonOnLeftBtnPress),
  FXMAPFUNC(SEL_LEFTBUTTONRELEASE, 0, dkButtonOnLeftBtnRelease)
};

/* Object implementation */
static struct dkMetaClass dkButtonMetaClass = {
  "dkButton", dkButtonMapEntry, sizeof(dkButtonMapEntry) / sizeof(dkButtonMapEntry[0]), sizeof(struct dkMapEntry)
};

/* Handle message */
long
dkButton_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkMapEntry *me;
  DKSelector sel = DKSEL(selhi, sello);

  me = DKMetaClassSearch(&dkButtonMetaClass, sel);
  return me ? me->func(pthis, obj, selhi, sello, data) : dkLabel_handle(pthis, obj, selhi, sello, data);
}

struct dkButton * dkButtonNew(struct dkWindow *parent, char *text)
{
  struct dkButton *ret;

  ret = fx_alloc(sizeof(struct dkButton));

  dkButtonInit(ret, parent, text, BUTTON_NORMAL);

  return ret;
}

void dkButtonInit(struct dkButton *pthis, struct dkWindow *p, char *text, DKuint opts)
{
  dkLabelInit((struct dkLabel *)pthis, p, text, opts);
  ((struct dkObject *)pthis)->meta = &dkButtonMetaClass;

  /* Setup the vtbl */
  ((struct dkObject *)pthis)->handle = dkButton_handle;
  ((struct dkWindow *)pthis)->canFocus = dkButton_canFocus;
  ((struct dkWindow *)pthis)->setDefault = dkButtonSetDefault;
  ((struct dkWindow *)pthis)->setFocus = dkButton_setFocus;
  ((struct dkWindow *)pthis)->killFocus = dkButton_killFocus;
  ((struct dkWindow *)pthis)->onPaint = dkButtonPaint2;

  pthis->state = STATE_UP;
  if (((struct dkWindow *)pthis)->options & BUTTON_INITIAL) {
//    setInitial(TRUE);
    ((struct dkWindow *)pthis)->setDefault((struct dkWindow *)pthis, TRUE);
  }

}

/* If window can have focus */
static int dkButton_canFocus(void)
{
  return 1;
}

/* Set focus to this widget */
void dkButton_setFocus(struct dkWindow *w)
{
  dkWindow_setFocus(w);
  if (w->options & BUTTON_DEFAULT) w->setDefault(w, TRUE);
  dkWindowUpdate(w);
}

/* Kill focus to this widget */
void dkButton_killFocus(struct dkWindow *w)
{
  dkWindow_killFocus(w);
  if (w->options & BUTTON_DEFAULT) w->setDefault(w, MAYBE);
  dkWindowUpdate(w);
}

/* Implement auto-hide or auto-gray modes */
static long dkButtonOnUpdate(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  if (!dkWindow_onUpdate(pthis, obj, selhi, sello, data)) {
    //if(options&BUTTON_AUTOHIDE){if(shown()){hide();recalc();}}
    //if(options&BUTTON_AUTOGRAY){disable();}
  }
  return 1;
}

/* Gained focus */
static long dkButtonOnFocusIn(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  dkWindow_onFocusIn(pthis, obj, selhi, sello, data);
  dkWindowUpdate((struct dkWindow *)pthis);
  return 1;
}

/* Lost focus */
static long dkButtonOnFocusOut(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  dkWindow_onFocusOut(pthis, obj, selhi, sello, data);
  dkWindowUpdate((struct dkWindow *)pthis);
  return 1;
}

/* Entered button */
static long dkButtonOnEnter(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkWindow *b = (struct dkWindow *)pthis;

  dkWindow_onEnter(pthis, obj, selhi, sello, data);
  if (dkWindowIsEnabled(b)) {
    if ((b->flags & FLAG_PRESSED) && (((struct dkButton *)b)->state != STATE_ENGAGED))
      dkButtonSetState((struct dkButton *)b, STATE_DOWN);
    if (b->options & BUTTON_TOOLBAR) dkWindowUpdate(b);
  }
  return 1;
}

/* Left button */
static long dkButtonOnLeave(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkWindow *b = (struct dkWindow *)pthis;

  dkWindow_onLeave(pthis, obj, selhi, sello, data);
  if (dkWindowIsEnabled(b)) {
    if ((b->flags & FLAG_PRESSED) && (((struct dkButton *)b)->state != STATE_ENGAGED))
      dkButtonSetState((struct dkButton *)b, STATE_UP);
    if (b->options & BUTTON_TOOLBAR) dkWindowUpdate(b);
  }
  return 1;
}

/* Pressed mouse button */
static long dkButtonOnLeftBtnPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *b = (struct dkWindow *)pthis;

  ((struct dkObject *)b)->handle(pthis, (struct dkObject *)pthis, SEL_FOCUS_SELF, 0, ptr);
  b->flags &= ~FLAG_TIP;
  if (dkWindowIsEnabled((struct dkWindow *)b) && !(b->flags & FLAG_PRESSED)) {
    dkWindowGrab((struct dkWindow *)b);
    if (((struct dkButton *)b)->state != STATE_ENGAGED)
      dkButtonSetState((struct dkButton *)b, STATE_DOWN);
    b->flags |= FLAG_PRESSED;
    b->flags &= ~FLAG_UPDATE;
    return 1;
  }
  return 0;
}

static long dkButtonOnLeftBtnRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *b = (struct dkWindow *)pthis;
  DKbool click;

  click = (((struct dkButton *)b)->state == STATE_DOWN);
  if (dkWindowIsEnabled(b) && (b->flags & FLAG_PRESSED)) {
    dkWindowUngrab(b);
    b->flags |= FLAG_UPDATE;
    b->flags &= ~FLAG_PRESSED;
    if (((struct dkButton *)b)->state != STATE_ENGAGED)
      dkButtonSetState((struct dkButton *)b, STATE_UP);
    if (click && b->actionCb) {
      b->actionCb(b, b->cbextra);
    }
    return 1;
  }
  return 0;
}

/* Lost the grab for some reason */
static long dkButtonOnUngrabbed(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dkButton *b = (struct dkButton *)pthis;

  printf("Got to dkButtonOnUngrabbed\n");
  if (b->state != STATE_ENGAGED) dkButtonSetState(b, STATE_UP);
  ((struct dkWindow *)b)->flags &= ~FLAG_PRESSED;
  ((struct dkWindow *)b)->flags |= FLAG_UPDATE;
  return 1;
}

static long
dkButtonPaint(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  struct dtkDC dc;
  struct dkButton *b = (struct dkButton *)pthis;
  int tw = 0, th = 0, iw = 0, ih = 0, tx, ty, ix, iy;

  /* Start drawing */
  dtkDrvDCEventSetup(&dc, (struct dkWindow *)b, (struct dkEvent *)data);

  /* Got a border at all? */
  if (((struct dkWindow *)b)->options & (FRAME_RAISED | FRAME_SUNKEN)) {

    /* Toolbar style */
    if (((struct dkWindow *)b)->options & BUTTON_TOOLBAR) {
      printf("TODO: Toolbar style\n");
    }
    /* Normal style */
    else {

      /* Default */
      if (dkWindowIsDefault((struct dkWindow *)b)) {
        printf("It's the default button\n");
      }

      /* Non-default */
      else {
        /* Draw in up state if disabled or up */
        if (!dkWindowIsEnabled((struct dkWindow *)b) || (b->state == STATE_UP)) {
          dtkDrvDCSetForeground(&dc, ((struct dkWindow *)b)->backColor);
          dtkDrvDCFillRectangle(&dc, ((struct dkFrame *)b)->border, ((struct dkFrame *)b)->border, ((struct dkWindow *)b)->width - ((struct dkFrame *)b)->border * 2, ((struct dkWindow *)b)->height - ((struct dkFrame *)b)->border * 2);
          if (((struct dkWindow *)b)->options & FRAME_THICK) dkFrameDrawDoubleRaisedRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
          else dkFrameDrawRaisedRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
        }
        /* Draw sunken if enabled and either checked or pressed */
        else {
          if (b->state == STATE_ENGAGED) dtkDrvDCSetForeground(&dc, ((struct dkFrame *)b)->hiliteColor);
          else dtkDrvDCSetForeground(&dc, ((struct dkWindow *)b)->backColor);
          dtkDrvDCFillRectangle(&dc, ((struct dkFrame *)b)->border, ((struct dkFrame *)b)->border, ((struct dkWindow *)b)->width - ((struct dkFrame *)b)->border * 2, ((struct dkWindow *)b)->height - ((struct dkFrame *)b)->border * 2);
          if (((struct dkWindow *)b)->options & FRAME_THICK) dkFrameDrawDoubleSunkenRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
          else dkFrameDrawSunkenRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
        }
      }
    }
  }

  /* No borders */
  else {
    printf("TODO: No borders\n");
  }

  /* Place text & icon */
  if (((struct dkLabel *)b)->label) {
    tw = dkLabelWidth((struct dkLabel *)b, ((struct dkLabel *)b)->label);
    th = dkLabelHeight((struct dkLabel *)b, ((struct dkLabel *)b)->label);
  }

  /* Do some justification */
  dkLabelJustX((struct dkLabel *)b, &tx, &ix, tw, iw);
  dkLabelJustY((struct dkLabel *)b, &ty, &iy, th, ih);

  /* Shift a bit when pressed */
  if (b->state && (((struct dkWindow *)b)->options & (FRAME_RAISED | FRAME_SUNKEN))) { ++tx; ++ty; ++ix; ++iy; }

  /* Draw enabled state */
  if (dkWindowIsEnabled((struct dkWindow *)b)) {
    if (((struct dkLabel *)b)->label) {
      dkDCWindowSetFont(&dc, ((struct dkLabel *)b)->font);
      dtkDrvDCSetForeground(&dc, ((struct dkLabel *)b)->textColor);
      dkLabelDrawLabel((struct dkLabel *)b, &dc, ((struct dkLabel *)b)->label, ((struct dkLabel *)b)->hotoff, tx, ty, tw, th);
    }
    if (dkWindowHasFocus((struct dkWindow *)b)) {
      dkDCDrawFocusRectangle(&dc, ((struct dkFrame *)b)->border + 1, ((struct dkFrame *)b)->border + 1,
          ((struct dkWindow *)b)->width - 2 * ((struct dkFrame *)b)->border - 2,
          ((struct dkWindow *)b)->height - 2 * ((struct dkFrame *)b)->border - 2);
    }
  }

  /* Draw grayed-out state */
  else {
    printf("TODO: button grayed out\n");
  }

  dkDCEnd(&dc);
  return 1;
}

static int
dkButtonPaint2(struct dkWindow *pthis, struct dkEvent *data)
{
	struct dtkDC dc;
	struct dkButton *b = (struct dkButton *)pthis;
	int tw = 0, th = 0, iw = 0, ih = 0, tx, ty, ix, iy;

	/* Start drawing */
	dtkDrvDCEventSetup(&dc, (struct dkWindow *)b, data);

	/* Got a border at all? */
	if (((struct dkWindow *)b)->options & (FRAME_RAISED | FRAME_SUNKEN)) {

		/* Toolbar style */
		if (((struct dkWindow *)b)->options & BUTTON_TOOLBAR) {
			printf("TODO: Toolbar style\n");
		}
		/* Normal style */
		else {

      /* Default */
      if (dkWindowIsDefault((struct dkWindow *)b)) {
        printf("It's the default button\n");
      }

      /* Non-default */
      else {
        /* Draw in up state if disabled or up */
        if (!dkWindowIsEnabled((struct dkWindow *)b) || (b->state == STATE_UP)) {
          dtkDrvDCSetForeground(&dc, ((struct dkWindow *)b)->backColor);
          dtkDrvDCFillRectangle(&dc, ((struct dkFrame *)b)->border, ((struct dkFrame *)b)->border, ((struct dkWindow *)b)->width - ((struct dkFrame *)b)->border * 2, ((struct dkWindow *)b)->height - ((struct dkFrame *)b)->border * 2);
          if (((struct dkWindow *)b)->options & FRAME_THICK) dkFrameDrawDoubleRaisedRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
          else dkFrameDrawRaisedRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
        }
        /* Draw sunken if enabled and either checked or pressed */
        else {
          if (b->state == STATE_ENGAGED) dtkDrvDCSetForeground(&dc, ((struct dkFrame *)b)->hiliteColor);
          else dtkDrvDCSetForeground(&dc, ((struct dkWindow *)b)->backColor);
          dtkDrvDCFillRectangle(&dc, ((struct dkFrame *)b)->border, ((struct dkFrame *)b)->border, ((struct dkWindow *)b)->width - ((struct dkFrame *)b)->border * 2, ((struct dkWindow *)b)->height - ((struct dkFrame *)b)->border * 2);
          if (((struct dkWindow *)b)->options & FRAME_THICK) dkFrameDrawDoubleSunkenRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
          else dkFrameDrawSunkenRectangle((struct dkFrame *)b, &dc, 0, 0, ((struct dkWindow *)b)->width, ((struct dkWindow *)b)->height);
        }
      }
    }
  }

  /* No borders */
  else {
    printf("TODO: No borders\n");
  }

  /* Place text & icon */
  if (((struct dkLabel *)b)->label) {
    tw = dkLabelWidth((struct dkLabel *)b, ((struct dkLabel *)b)->label);
    th = dkLabelHeight((struct dkLabel *)b, ((struct dkLabel *)b)->label);
  }

  /* Do some justification */
  dkLabelJustX((struct dkLabel *)b, &tx, &ix, tw, iw);
  dkLabelJustY((struct dkLabel *)b, &ty, &iy, th, ih);

  /* Shift a bit when pressed */
  if (b->state && (((struct dkWindow *)b)->options & (FRAME_RAISED | FRAME_SUNKEN))) { ++tx; ++ty; ++ix; ++iy; }

  /* Draw enabled state */
  if (dkWindowIsEnabled((struct dkWindow *)b)) {
    if (((struct dkLabel *)b)->label) {
      dkDCWindowSetFont(&dc, ((struct dkLabel *)b)->font);
      dtkDrvDCSetForeground(&dc, ((struct dkLabel *)b)->textColor);
      dkLabelDrawLabel((struct dkLabel *)b, &dc, ((struct dkLabel *)b)->label, ((struct dkLabel *)b)->hotoff, tx, ty, tw, th);
    }
    if (dkWindowHasFocus((struct dkWindow *)b)) {
      dkDCDrawFocusRectangle(&dc, ((struct dkFrame *)b)->border + 1, ((struct dkFrame *)b)->border + 1,
          ((struct dkWindow *)b)->width - 2 * ((struct dkFrame *)b)->border - 2,
          ((struct dkWindow *)b)->height - 2 * ((struct dkFrame *)b)->border - 2);
    }
  }

  /* Draw grayed-out state */
  else {
    printf("TODO: button grayed out\n");
  }

  dkDCEnd(&dc);
  return 1;
}

/* Set button state */
void dkButtonSetState(struct dkButton *b, DKuint s)
{
  if (b->state != s){
    b->state = s;
    dkWindowUpdate((struct dkWindow *)b);
  }
}

/* Make widget drawn as default */
void dkButtonSetDefault(struct dkWindow *w, DKbool enable)
{
  dkWindow_setDefault(w, enable);
  dkWindowUpdate(w);
}

