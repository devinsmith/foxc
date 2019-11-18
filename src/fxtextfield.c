/******************************************************************************
 *                                                                            *
 *                      T e x t   F i e l d   W i d g e t                     *
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
 * $Id: FXTextField.cpp,v 1.171.2.3 2008/09/22 20:46:59 fox Exp $             *
 *****************************************************************************/

#include <string.h>

#include "fxkeys.h"
#include "fxtextfield.h"

/*
  Notes:

  - TextField passes string ptr in the SEL_COMMAND callback.
  - Double-click should select word, triple click all of text field.
  - TextField should return 0 for all unhandled keys!
  - Pressing mouse button will set the focus w/o claiming selection!
  - Change of cursor only implies makePositionVisible() if done by user.
  - Input verify and input verify callback operation:

    1) The input is tested to see if it qualifies as an integer or
       real number.
    2) The target is allowed to raise an objection: if a target does NOT
       handle the message, or handles the message and returns 0, then the
       new input is accepted.
    3) If none of the above applies the input is simply accepted;
       this is the default mode for generic text type-in.

    Note that the target callback is called AFTER already having verified that
    the entry is a number, so a target can simply assume that this has been checked
    already and just perform additional checks [e.g. numeric range].

    Also note that verify callbacks should allow for partially complete inputs,
    and that these inputs could be built up a character at a time, and in no
    particular order.

  - Option to grow/shrink textfield to fit text.
  - Perhaps need selstartpos,selendpos member variables to keep track of selection.
  - Maybe also send SEL_SELECTED, SEL_DESELECTED?
  - Need block cursor when in overstrike mode.
*/

static long dkTextField_onPaint(void *pthis, struct dkObject *obj, void *ptr);
static long dkTextField_onLeftBtnPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onLeftBtnRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onBlink(void *pthis, struct dkObject *obj, void *ptr);
static long dkTextField_onFocusSelf(void *pthis, struct dkObject *sender, DKSelector selhi, DKSelector sello, void* ptr);
static long dkTextField_onFocusIn(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onFocusOut(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onKeyPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onKeyRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdInsertString(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdBackspace(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdCursorHome(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdCursorEnd(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdCursorLeft(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdCursorRight(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdDelete(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdMark(void *pthis, struct dkObject *obj, void *ptr);
static long dkTextField_onCmdExtend(void *pthis, struct dkObject *obj, void *ptr);
static long dkTextField_onSelectionGained(void *pthis, struct dkObject *sender, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onSelectionLost(void *pthis, struct dkObject *sender, DKSelector selhi, DKSelector sello, void *ptr);
static long dkTextField_onCmdDeleteSel(void *pthis, struct dkObject *sender, void *ptr);

static int dkTextFieldGetDefaultWidth(struct dkWindow *w);
static int dkTextFieldGetDefaultHeight(struct dkWindow *w);
void dkTextField_drawTextRange(struct dkTextField *tf, struct dtkDC *dc, int fm, int to);
void dkTextFieldCreate(void *pthis);
static int dkTextFieldCanFocus(void);
void dkTextField_enable(struct dkWindow *w);
static void dkTextField_makePositionVisible(struct dkTextField *tf, int pos);
void dkTextField_layout(struct dkWindow *w);
void dkTextField_setFocus(struct dkWindow *w);
void dkTextField_setAnchorPos(struct dkTextField *tf, int pos);
int dkTextField_isOverstrike(struct dkTextField *tf);
int dkTextField_killSelection(struct dkTextField *tf);

/* Object implementation */
static struct dkMetaClass dkTextFieldMetaClass = {
  "dkTextField", NULL, 0, 0
};

/* Handle message */
long dkTextField_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  switch (selhi) {
    case SEL_PAINT:
      return dkTextField_onPaint(pthis, obj, data);
    case SEL_LEFTBUTTONPRESS:
      return dkTextField_onLeftBtnPress(pthis, obj, selhi, sello, data);
    case SEL_LEFTBUTTONRELEASE:
      return dkTextField_onLeftBtnRelease(pthis, obj, selhi, sello, data);
    case SEL_KEYPRESS:
      return dkTextField_onKeyPress(pthis, obj, selhi, sello, data);
    case SEL_KEYRELEASE:
      return dkTextField_onKeyRelease(pthis, obj, selhi, sello, data);
    case SEL_SELECTION_LOST:
      return dkTextField_onSelectionLost(pthis, obj, selhi, sello, data);
    case SEL_SELECTION_GAINED:
      return dkTextField_onSelectionGained(pthis, obj, selhi, sello, data);
    case SEL_FOCUSIN:
      return dkTextField_onFocusIn(pthis, obj, selhi, sello, data);
    case SEL_FOCUSOUT:
      return dkTextField_onFocusOut(pthis, obj, selhi, sello, data);
    case SEL_FOCUS_SELF:
      return dkTextField_onFocusSelf(pthis, obj, selhi, sello, data);
    case SEL_COMMAND:
      switch (sello) {
        case TF_ID_CURSOR_HOME:
          return dkTextField_onCmdCursorHome(pthis, obj, selhi, sello, data);
        case TF_ID_CURSOR_END:
          return dkTextField_onCmdCursorEnd(pthis, obj, selhi, sello, data);
        case TF_ID_CURSOR_RIGHT:
          return dkTextField_onCmdCursorRight(pthis, obj, selhi, sello, data);
        case TF_ID_CURSOR_LEFT:
          return dkTextField_onCmdCursorLeft(pthis, obj, selhi, sello, data);
        case ID_INSERT_STRING:
          return dkTextField_onCmdInsertString(pthis, obj, selhi, sello, data);
        case ID_MARK:
          return dkTextField_onCmdMark(pthis, obj, data);
        case ID_EXTEND:
          return dkTextField_onCmdExtend(pthis, obj, data);
        case ID_DELETE_SEL:
          return dkTextField_onCmdDeleteSel(pthis, obj, data);
        case ID_BACKSPACE:
          return dkTextField_onCmdBackspace(pthis, obj, selhi, sello, data);
        case ID_DESELECT_ALL:
          dkTextField_killSelection((struct dkTextField *)pthis);
          return 1;
        case ID_DELETE:
          return dkTextField_onCmdDelete(pthis, obj, selhi, sello, data);
        default:
          printf("What is sello: %d\n", sello);
      }
    case SEL_TIMEOUT:
      if (sello == TF_ID_BLINK)
        return dkTextField_onBlink(pthis, obj, data);
  }

  return dkFrame_handle(pthis, obj, selhi, sello, data);
}

struct dkTextField *dkTextFieldNew(struct dkComposite *p, int ncols, DKuint opts)
{
  struct dkTextField *ret;

  ret = fx_alloc(sizeof(struct dkTextField));
  dkTextFieldInit(ret, p, ncols, NULL, opts);

  return ret;
}

void dkTextFieldInit(struct dkTextField *pthis, struct dkComposite *p, int ncols, struct dkObject *tgt, DKuint opts)
{
  DtkFrameInit((struct dkFrame *)pthis, (struct dkWindow *)p, opts, 0, 0, 0, 0, DEFAULT_PAD, DEFAULT_PAD, DEFAULT_PAD, DEFAULT_PAD);
  ((struct dkObject *)pthis)->meta = &dkTextFieldMetaClass;

  ((struct dkObject *)pthis)->handle = dkTextField_handle;
  ((struct dkWindow *)pthis)->create = dkTextFieldCreate;
  ((struct dkWindow *)pthis)->canFocus = dkTextFieldCanFocus;
  ((struct dkWindow *)pthis)->getDefaultWidth = dkTextFieldGetDefaultWidth;
  ((struct dkWindow *)pthis)->getDefaultHeight = dkTextFieldGetDefaultHeight;
  ((struct dkWindow *)pthis)->enable = dkTextField_enable;
  ((struct dkWindow *)pthis)->layout = dkTextField_layout;
  ((struct dkWindow *)pthis)->setFocus = dkTextField_setFocus;

  pthis->contents = dstr_new_empty();
  if (ncols < 0) ncols = 0;
  ((struct dkWindow *)pthis)->flags |= FLAG_ENABLED;
  ((struct dkWindow *)pthis)->target = tgt;
  if (!(((struct dkWindow *)pthis)->options & JUSTIFY_RIGHT)) ((struct dkWindow *)pthis)->options |= JUSTIFY_LEFT;
  pthis->font = ((struct dkWindow *)pthis)->app->normalFont;
  ((struct dkWindow *)pthis)->backColor = ((struct dkWindow *)pthis)->app->backColor;
  pthis->textColor = ((struct dkWindow *)pthis)->app->foreColor;
  pthis->selbackColor = ((struct dkWindow *)pthis)->app->selbackColor;
  pthis->seltextColor = ((struct dkWindow *)pthis)->app->selforeColor;
  pthis->cursorColor = ((struct dkWindow *)pthis)->app->foreColor;
  pthis->cursor = 0;
  pthis->anchor = 0;
  pthis->columns = ncols;
  pthis->shift = 0;

}

void dkTextFieldCreate(void *pthis)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;

  dkFrameCreate(pthis);
  dkFontCreate(tf->font);
}

void dkTextField_enable(struct dkWindow *w)
{
  if (!(w->flags & FLAG_ENABLED)) {
    printf("Enabling text window\n");
  }
}

/* If window can have focus */
static int dkTextFieldCanFocus(void)
{
  return 1;
}

/* Into focus chain */
void dkTextField_setFocus(struct dkWindow *w)
{
  dkWindow_setFocus(w);
  dkWindow_setDefault(w, TRUE);
  w->flags &= ~FLAG_UPDATE;
//  if(getApp()->hasInputMethod()){
//    createComposeContext();
//    }
}

int dkTextField_isOverstrike(struct dkTextField *tf)
{
  return (((struct dkWindow *)tf)->options & TEXTFIELD_OVERSTRIKE) != 0;
}


/* Force position to become fully visible; we assume layout is correct */
static void dkTextField_makePositionVisible(struct dkTextField *tf, int pos)
{
  int rr = ((struct dkWindow *)tf)->width - ((struct dkFrame *)tf)->border - ((struct dkFrame *)tf)->padright;
  int ll = ((struct dkFrame *)tf)->border + ((struct dkFrame *)tf)->padleft;
  int ww = rr - ll;
  int oldshift = tf->shift;
  int xx;

  if (!((struct dkWindow *)tf)->xid) return;
  pos = dkString_validate(tf->contents->str, DKCLAMP(0, pos, dstr_getlength(tf->contents)));
  if (((struct dkWindow *)tf)->options & JUSTIFY_RIGHT) {
    if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD)
      xx = dkFontGetTextWidth(tf->font, "*",1) * dstr_count2(tf->contents, pos, dstr_getlength(tf->contents));
    else
      xx = dkFontGetTextWidth(tf->font, &tf->contents->str[pos], dstr_getlength(tf->contents) - pos);
    if (tf->shift - xx > 0) tf->shift = xx;
    else if (tf->shift - xx < -ww) tf->shift = xx - ww;
  }
  else if (((struct dkWindow *)tf)->options & JUSTIFY_LEFT) {
    if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD)
      xx = dkFontGetTextWidth(tf->font, "*", 1) * dstr_index(tf->contents, pos);
    else
      xx = dkFontGetTextWidth(tf->font, tf->contents->str, pos);
    if (tf->shift + xx < 0) tf->shift = -xx;
    else if (tf->shift + xx >= ww) tf->shift = ww - xx;
  }
  else {
    if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD)
      xx = dkFontGetTextWidth(tf->font, "*", 1) * dstr_index(tf->contents, pos) - (dkFontGetTextWidth(tf->font, "*", 1) * dstr_count(tf->contents)) / 2;
    else
      xx = dkFontGetTextWidth(tf->font, tf->contents->str, pos) - dkFontGetTextWidth(tf->font, tf->contents->str, dstr_getlength(tf->contents)) / 2;
    if (tf->shift + ww / 2 + xx < 0) tf->shift = -ww / 2 - xx;
    else if (tf->shift + ww / 2 + xx >= ww) tf->shift = ww - ww / 2 - xx;
  }
  if (tf->shift != oldshift) {
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
  }
}

void dkTextField_layout(struct dkWindow *w)
{
  int rr = w->width - ((struct dkFrame *)w)->border - ((struct dkFrame *)w)->padright;
  int ll = ((struct dkFrame *)w)->border + ((struct dkFrame *)w)->padleft;
  int ww = rr - ll;
  int tw;
  struct dkTextField *tf = (struct dkTextField *)w;

  if (!w->xid)
    return;

  /* Figure text width */
  if (w->options & TEXTFIELD_PASSWD)
    tw = dkFontGetTextWidth(tf->font, "*", 1) * dstr_count(tf->contents);
  else
    tw = dkFontGetTextWidth(tf->font, tf->contents->str, dstr_getlength(tf->contents));

  /* Constrain shift */
  if (w->options & JUSTIFY_RIGHT) {
    if (ww >= tw) tf->shift = 0;
    else if (tf->shift < 0) tf->shift = 0;
    else if (tf->shift > tw - ww) tf->shift = tw - ww;
  }
  else if (w->options & JUSTIFY_LEFT) {
    if (ww >= tw) tf->shift = 0;
    else if (tf->shift > 0) tf->shift = 0;
    else if (tf->shift < ww - tw) tf->shift = ww - tw;
  }
  else {
    if (ww >= tw) tf->shift = 0;
    else if (tf->shift > tw / 2 - ww / 2) tf->shift = tw / 2 - ww / 2;
    else if (tf->shift < (ww - ww / 2) - tw / 2) tf->shift = (ww - ww / 2) - tw / 2;
  }

  /* Keep cursor in the picture if resizing field */
  dkTextField_makePositionVisible(tf, tf->cursor);

  /* Always redraw */
  dkWindowUpdate(w);

  w->flags &= ~FLAG_DIRTY;
}

/* Find index from coord */
int dkTextField_index(struct dkTextField *tf, int x)
{
  int rr, ll, mm;
  int pos, xx, cw;
  int len = dstr_getlength(tf->contents);

  rr = ((struct dkWindow *)tf)->width - ((struct dkFrame *)tf)->border - ((struct dkFrame *)tf)->padright;
  ll = ((struct dkFrame *)tf)->border + ((struct dkFrame *)tf)->padleft;
  mm = (ll + rr) / 2;

  if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD) {
    cw = dkFontGetTextWidth(tf->font, "*", 1);
    if (((struct dkWindow *)tf)->options & JUSTIFY_RIGHT) xx = rr - cw * dstr_count(tf->contents);
    else if (((struct dkWindow *)tf)->options & JUSTIFY_LEFT) xx = ll;
    else xx = mm - (cw * dstr_count(tf->contents)) / 2;
    xx += tf->shift;
    pos = dstr_offset(tf->contents, (x - xx + (cw >> 1)) / cw);
  }
  else {
    if (((struct dkWindow *)tf)->options & JUSTIFY_RIGHT) xx = rr - dkFontGetTextWidth(tf->font, tf->contents->str, len);
    else if (((struct dkWindow *)tf)->options & JUSTIFY_LEFT) xx = ll;
    else xx = mm - dkFontGetTextWidth(tf->font, tf->contents->str, len) / 2;
    xx += tf->shift;
    for (pos = 0; pos < len; pos = dstr_inc(tf->contents, pos)) {
      cw = dkFontGetTextWidth(tf->font, &tf->contents->str[pos], ds_extent(tf->contents, pos));
      if (x < (xx + (cw >> 1))) break;
      xx += cw;
    }
  }
  if (pos < 0) pos = 0;
  if (pos > len) pos = len;
  return pos;
}

/* Get default width */
static int dkTextFieldGetDefaultWidth(struct dkWindow *w)
{
  struct dkFrame *f = (struct dkFrame *)w;

  return f->padleft + f->padright + (f->border << 1) + ((struct dkTextField *)f)->columns * dkFontGetTextWidth(((struct dkTextField *)f)->font, "8", 1);
}

DKbool dkTextField_isEditable(struct dkTextField *tf)
{
  return (((struct dkWindow *)tf)->options & TEXTFIELD_READONLY) == 0;
}

void dkTextField_setCursorPos(struct dkTextField *tf, int pos)
{
  pos = dkString_validate(tf->contents->str, DKCLAMP(0, pos, dstr_getlength(tf->contents)));
  if (tf->cursor != pos) {
    dkTextField_drawCursor(tf, 0);
    tf->cursor = pos;
    if (dkTextField_isEditable(tf) && dkWindowHasFocus((struct dkWindow *)tf))
      dkTextField_drawCursor(tf, FLAG_CARET);
  }
}

/* Kill the selection */
int dkTextField_killSelection(struct dkTextField *tf)
{
  if (dkWindowHasSelection((struct dkWindow *)tf)) {
    dkWindow_releaseSelection((struct dkWindow *)tf);
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
    return 1;
  }
  return 0;
}

void dkTextField_setAnchorPos(struct dkTextField *tf, int pos)
{
  tf->anchor = dkString_validate(tf->contents->str, DKCLAMP(0, pos, dstr_getlength(tf->contents)));
}

/* Draw the cursor; need to draw 2 characters around the cursor
 * due to possible overhanging in certain fonts.  Also, need to
 * completely erase and redraw because of ClearType.
 * Kudos to Bill Baxter for help with this code. */
void dkTextField_drawCursor(struct dkTextField *tf, DKuint state)
{
  int cl, ch, xx, xlo, xhi;
  if ((state ^ ((struct dkWindow *)tf)->flags) & FLAG_CARET) {
    if (((struct dkWindow *)tf)->xid) {
      struct dtkDC dc;

      dkDCSetup(&dc, (struct dkWindow *)tf);
//      FXASSERT(0<=cursor && cursor<=contents.length());
//      FXASSERT(0<=anchor && anchor<=contents.length());
      xx = dkTextField_coord(tf, tf->cursor) - 1;

      // Clip rectangle around cursor
      xlo = FXMAX(xx - 2, ((struct dkFrame *)tf)->border);
      xhi = FXMIN(xx + 3, ((struct dkWindow *)tf)->width - ((struct dkFrame *)tf)->border);

      // Cursor can overhang padding but not borders
      dkDCSetClipRectangle(&dc, xlo, ((struct dkFrame *)tf)->border, xhi - xlo, ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));

      // Draw I beam
      if (state & FLAG_CARET) {

        // Draw I-beam
        dtkDrvDCSetForeground(&dc, tf->cursorColor);
        dtkDrvDCFillRectangle(&dc, xx, ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border, 1, ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->padbottom - ((struct dkFrame *)tf)->padtop - (((struct dkFrame *)tf)->border << 1));
        dtkDrvDCFillRectangle(&dc, xx - 2, ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border, 5, 1);
        dtkDrvDCFillRectangle(&dc, xx - 2, ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->border - ((struct dkFrame *)tf)->padbottom - 1, 5, 1);
      }

      // Erase I-beam
      else {

        // Erase I-beam, plus the text immediately surrounding it
        dtkDrvDCSetForeground(&dc, ((struct dkWindow *)tf)->backColor);
        dtkDrvDCFillRectangle(&dc, xx - 2, ((struct dkFrame *)tf)->border, 5, ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));

        // Draw two characters before and after cursor
        cl = ch = tf->cursor;
        if (0 < cl) {
          cl = dstr_dec(tf->contents, cl);
          if (0 < cl) {
            cl = dstr_dec(tf->contents, cl);
          }
        }
        if (ch < dstr_getlength(tf->contents)) {
          ch = dstr_inc(tf->contents, ch);
          if (ch < dstr_getlength(tf->contents)) {
            ch = dstr_inc(tf->contents, ch);
          }
        }
        dkTextField_drawTextRange(tf, &dc, cl, ch);
      }
      dkDCEnd(&dc);
    }
    ((struct dkWindow *)tf)->flags ^= FLAG_CARET;
  }
}

/* Find coordinate from index */
int dkTextField_coord(struct dkTextField *tf, int i)
{
  int rr = ((struct dkWindow *)tf)->width - ((struct dkFrame *)tf)->border - ((struct dkFrame *)tf)->padright;
  int ll = ((struct dkFrame *)tf)->border + ((struct dkFrame *)tf)->padleft;
  int mm = (ll + rr) / 2;
  int pos;

  //FXASSERT(0<=i && i<=contents.length());
  if (((struct dkWindow *)tf)->options & JUSTIFY_RIGHT) {
    if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD) {
      pos = rr - dkFontGetTextWidth(tf->font, "*", 1) * (dstr_count(tf->contents) - dstr_index(tf->contents, i));
    }
    else {
      pos = rr - dkFontGetTextWidth(tf->font, &tf->contents->str[i], dstr_getlength(tf->contents) - i);
    }
  }
  else if (((struct dkWindow *)tf)->options & JUSTIFY_LEFT) {
    if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD) {
      pos = ll + dkFontGetTextWidth(tf->font, "*", 1) * dstr_index(tf->contents, i);
    }
    else {
      pos = ll + dkFontGetTextWidth(tf->font, tf->contents->str, i);
    }
  }
  else {
    if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD) {
      pos = mm + dkFontGetTextWidth(tf->font, "*",1) * dstr_index(tf->contents, i) - (dkFontGetTextWidth(tf->font, "*", 1) * dstr_count(tf->contents)) / 2;
    }
    else {
      pos = mm + dkFontGetTextWidth(tf->font, tf->contents->str, i) - dkFontGetTextWidth(tf->font, tf->contents->str, dstr_getlength(tf->contents)) / 2;
    }
  }
  return pos + tf->shift;
}


/* Get default height */
static int dkTextFieldGetDefaultHeight(struct dkWindow *w)
{
  struct dkFrame *f = (struct dkFrame *)w;

  return f->padtop + f->padbottom + (f->border << 1) + dkFontGetFontHeight(((struct dkTextField *)f)->font);
}

/* Focus on widget itself */
static long dkTextField_onFocusSelf(void *pthis, struct dkObject *sender, DKSelector selhi, DKSelector sello, void* ptr)
{
  if (dkWindow_onFocusSelf(pthis, sender, selhi, sello, ptr)) {
    struct dkEvent *event = (struct dkEvent *)ptr;
    if (event->type == SEL_KEYPRESS || event->type == SEL_KEYRELEASE) {
      dkTextField_handle(pthis, pthis, SEL_COMMAND, TF_ID_SELECT_ALL, NULL);
    }
    return 1;
  }
  return 0;
}

/* Draw range of text, fm to to in character length */
void dkTextField_drawTextRange(struct dkTextField *tf, struct dtkDC *dc, int fm, int to)
{
  int sx, ex, xx, yy, cw, hh, ww, si, ei, lx, rx, t;
  int rr = ((struct dkWindow *)tf)->width - ((struct dkFrame *)tf)->border - ((struct dkFrame *)tf)->padright;
  int ll = ((struct dkFrame *)tf)->border + ((struct dkFrame *)tf)->padleft;
  int mm = (ll + rr) / 2;

  if (to <= fm) return;

  dkDCWindowSetFont(dc, tf->font);

  /* Text color */
  dtkDrvDCSetForeground(dc, tf->textColor);

  /* Height */
  hh = dkFontGetFontHeight(tf->font);

  /* Text sticks to top of field */
  if (((struct dkWindow *)tf)->options & JUSTIFY_TOP) {
    yy = ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border;
  }

  /* Text sticks to bottom of field */
  else if (((struct dkWindow *)tf)->options & JUSTIFY_BOTTOM) {
    yy = ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->padbottom - ((struct dkFrame *)tf)->border - hh;
  }

  /* Text centered in y */
  else {
    yy = ((struct dkFrame *)tf)->border + ((struct dkFrame *)tf)->padtop + (((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->padbottom - ((struct dkFrame *)tf)->padtop - (((struct dkFrame *)tf)->border << 1) - hh) / 2;
  }

  if (tf->anchor < tf->cursor) {
    si = tf->anchor; ei = tf->cursor;
  } else {
    si = tf->cursor; ei = tf->anchor;
  }

  /* Password mode */
  if (((struct dkWindow *)tf)->options & TEXTFIELD_PASSWD) {
    /* TODO: Finish implementation */
  }
  /* Normal mode */
  else {
    ww = dkFontGetTextWidth(tf->font, tf->contents->str, dstr_getlength(tf->contents));

    /* Text sticks to right of field */
    if (((struct dkWindow *)tf)->options & JUSTIFY_RIGHT) {
      xx = tf->shift + rr - ww;
    }

    /* Text sticks on left of field */
    else if (((struct dkWindow *)tf)->options & JUSTIFY_LEFT) {
      xx = tf->shift + ll;
    }

    /* Text centered in field */
    else {
      xx = tf->shift + mm - ww / 2;
    }

    /* Reduce to avoid drawing excessive amounts of text */
    lx = xx + dkFontGetTextWidth(tf->font, &tf->contents->str[0], fm);
    rx = lx + dkFontGetTextWidth(tf->font, &tf->contents->str[fm], to - fm);
    while (fm < to) {
      t = dstr_inc(tf->contents, fm);
      cw = dkFontGetTextWidth(tf->font, &tf->contents->str[fm], t - fm);
      if (lx + cw >= 0) break;
      lx += cw;
      fm = t;
    }
    while (fm < to) {
      t = dstr_dec(tf->contents, to);
      cw = dkFontGetTextWidth(tf->font, &tf->contents->str[t], to - t);
      if (rx - cw < ((struct dkWindow *)tf)->width) break;
      rx -= cw;
      to = t;
    }

    /* Adjust selected range */
    if (si < fm) si = fm;
    if (ei > to) ei = to;

    /* Nothing selected */
    if (!dkWindowHasSelection((struct dkWindow *)tf) || to <= si || ei <= fm) {
      dkTextField_drawTextFragment(tf, dc, xx, yy, fm, to);
    }
    /* Stuff selected */
    else {
      if (fm < si) {
        dkTextField_drawTextFragment(tf, dc, xx, yy, fm, si);
      } else {
        si = fm;
      }
      if (ei < to) {
        dkTextField_drawTextFragment(tf, dc, xx, yy, ei, to);
      } else {
        ei = to;
      }
      if (si < ei) {
        sx = xx + dkFontGetTextWidth(tf->font, tf->contents->str, si);
        ex = xx + dkFontGetTextWidth(tf->font, tf->contents->str, ei);
        if (dkWindowHasFocus((struct dkWindow *)tf)) {
          dtkDrvDCSetForeground(dc, tf->selbackColor);
          dtkDrvDCFillRectangle(dc, sx, ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border, ex - sx, ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->padtop - ((struct dkFrame *)tf)->padbottom - (((struct dkFrame *)tf)->border << 1));
          dtkDrvDCSetForeground(dc, tf->seltextColor);
          dkTextField_drawTextFragment(tf, dc, xx, yy, si, ei);
        } else {
          dtkDrvDCSetForeground(dc, ((struct dkFrame *)tf)->baseColor);
          dtkDrvDCFillRectangle(dc, sx, ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border, ex - sx, ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->padtop - ((struct dkFrame *)tf)->padbottom - (((struct dkFrame *)tf)->border << 1));
          dtkDrvDCSetForeground(dc, tf->textColor);
          dkTextField_drawTextFragment(tf, dc, xx, yy, si, ei);
        }
      }
    }
  }
}

/* Draw text fragment */
void dkTextField_drawTextFragment(struct dkTextField *tf, struct dtkDC *dc, int x, int y, int fm, int to)
{
  x += dkFontGetTextWidth(tf->font, tf->contents->str, fm);
  y += dkFontGetFontAscent(tf->font);
  dkDCDrawText(dc, x, y, &tf->contents->str[fm], to - fm);
}

static long dkTextField_onPaint(void *pthis, struct dkObject *obj, void *ptr)
{
  struct dtkDC dc;
  struct dkTextField *tf = (struct dkTextField *)pthis;
  struct dkEvent *ev = (struct dkEvent *)ptr;

  /* Start drawing */
  dtkDrvDCEventSetup(&dc, (struct dkWindow *)tf, ev);

  /* Draw frame */
  dkFrameDrawFrame((struct dkFrame *)tf, &dc, 0, 0, ((struct dkWindow *)tf)->width, ((struct dkWindow *)tf)->height);

  /* Gray background if disabled */
  if (dkWindowIsEnabled((struct dkWindow *)tf))
    dtkDrvDCSetForeground(&dc, ((struct dkWindow *)tf)->backColor);
  else
    dtkDrvDCSetForeground(&dc, ((struct dkFrame *)tf)->baseColor);

  /* Draw background */
  dtkDrvDCFillRectangle(&dc, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border,
      ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1),
      ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));

  /* Draw text, clipped against frame interior */
  dkDCSetClipRectangle(&dc, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
  dkTextField_drawTextRange(tf, &dc, 0, dstr_getlength(tf->contents));

  /* Draw caret */
  if (((struct dkWindow *)tf)->flags & FLAG_CARET) {
    int xx = dkTextField_coord(tf, tf->cursor) - 1;
    dtkDrvDCSetForeground(&dc, tf->cursorColor);
    dtkDrvDCFillRectangle(&dc, xx, ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border, 1, ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->padbottom - ((struct dkFrame *)tf)->padtop - (((struct dkFrame *)tf)->border << 1));
    dtkDrvDCFillRectangle(&dc, xx - 2, ((struct dkFrame *)tf)->padtop + ((struct dkFrame *)tf)->border, 5, 1);
    dtkDrvDCFillRectangle(&dc, xx - 2, ((struct dkWindow *)tf)->height - ((struct dkFrame *)tf)->border - ((struct dkFrame *)tf)->padbottom - 1, 5, 1);
  }

  dkDCEnd(&dc);
  return 1;
}

static long dkTextField_onLeftBtnPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkEvent *ev = (struct dkEvent *)ptr;
  struct dkWindow *tf = (struct dkWindow *)pthis;

  tf->flags &= ~FLAG_TIP;
  dkTextField_handle(pthis, pthis, SEL_FOCUS_SELF, 0, ptr);
  if (dkWindowIsEnabled(tf)) {
    dkWindowGrab(tf);
    if (tf->target && ((struct dkObject *)tf->target)->handle(tf->target, (struct dkObject *)pthis, SEL_LEFTBUTTONPRESS, tf->message, ptr))
      return 1;
    tf->flags &= ~FLAG_UPDATE;
    if (ev->click_count == 1) {
      dkTextField_setCursorPos((struct dkTextField *)tf, dkTextField_index((struct dkTextField *)tf, ev->win_x));
      if (ev->state & SHIFTMASK) {
        //extendSelection...
      }
      else {
        dkTextField_killSelection((struct dkTextField *)tf);
        dkTextField_setAnchorPos((struct dkTextField *)tf, ((struct dkTextField *)tf)->cursor);
      }
      dkTextField_makePositionVisible((struct dkTextField *)tf, ((struct dkTextField *)tf)->cursor);
      tf->flags |= FLAG_PRESSED;
    }
    else {
      dkTextField_setAnchorPos((struct dkTextField *)tf, 0);
      dkTextField_setCursorPos((struct dkTextField *)tf, dstr_getlength(((struct dkTextField *)tf)->contents));
      // extendSel
      dkTextField_makePositionVisible((struct dkTextField *)tf, ((struct dkTextField *)tf)->cursor);
    }
    return 1;
  }
  return 0;
}

/* Released left button */
static long dkTextField_onLeftBtnRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkWindow *tf = (struct dkWindow *)pthis;
  if (dkWindowIsEnabled(tf)) {
    dkWindowUngrab(tf);
    tf->flags &= ~FLAG_PRESSED;
    if (tf->target && ((struct dkObject *)tf->target)->handle(tf->target, (struct dkObject *)pthis, SEL_LEFTBUTTONRELEASE, tf->message, ptr))
      return 1;
    return 1;
  }
  return 0;
}

/* Blink the cursor */
static long dkTextField_onBlink(void *pthis, struct dkObject *obj, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_drawCursor(tf, ((struct dkWindow *)tf)->flags ^ FLAG_CARET);
  fxAppAddTimeout(((struct dkWindow *)tf)->app, (struct dkObject *)tf, TF_ID_BLINK, ((struct dkWindow *)tf)->app->blinkSpeed, NULL);
  return 0;
}

/* Gained focus */
static long dkTextField_onFocusIn(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;

  dkWindow_onFocusIn(pthis, obj, selhi, sello, ptr);
  if (dkTextField_isEditable(tf)) {
    fxAppAddTimeout(((struct dkWindow *)tf)->app, (struct dkObject *)tf, TF_ID_BLINK, ((struct dkWindow *)tf)->app->blinkSpeed, NULL);
    dkTextField_drawCursor(tf, FLAG_CARET);
  }
  if (dkWindowHasSelection((struct dkWindow *)tf)) {
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
  }
  return 1;
}

/* Lost focus */
static long dkTextField_onFocusOut(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;

  dkWindow_onFocusOut(pthis, obj, selhi, sello, ptr);
  fxAppRemoveTimeout(((struct dkWindow *)tf)->app, (struct dkObject *)tf, TF_ID_BLINK);
  dkTextField_drawCursor(tf, 0);
  if (dkWindowHasSelection((struct dkWindow *)tf)) {
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
  }
  return 1;
}

/* Pressed a key */
static long dkTextField_onKeyPress(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkEvent *event = (struct dkEvent *)ptr;
  struct dkWindow *tf = (struct dkWindow *)pthis;

  tf->flags &= ~FLAG_TIP;
  if (dkWindowIsEnabled(tf)) {
    DKTRACE((200, "%s::onKeyPress keysym=0x%04x state=%04x\n", ((struct dkObject *)tf)->meta->className, event->code, event->state));
    if (tf->target && tf->target->handle(tf->target, pthis, SEL_KEYPRESS, tf->message, ptr))
      return 1;
    tf->flags &= ~FLAG_UPDATE;
    switch (event->code) {
      case KEY_Right:
      case KEY_KP_Right:
        if (!(event->state & SHIFTMASK)) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DESELECT_ALL, NULL);
        }
        if (event->state & CONTROLMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_CURSOR_WORD_RIGHT, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, TF_ID_CURSOR_RIGHT, NULL);
        }
        if (event->state & SHIFTMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_EXTEND, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_MARK, NULL);
        }
        return 1;
      case KEY_Left:
      case KEY_KP_Left:
        if (!(event->state & SHIFTMASK)) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DESELECT_ALL, NULL);
        }
        if (event->state & CONTROLMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_CURSOR_WORD_LEFT, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, TF_ID_CURSOR_LEFT, NULL);
        }
        if (event->state & SHIFTMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_EXTEND, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_MARK, NULL);
        }
        return 1;
      case KEY_Home:
      case KEY_KP_Home:
        if (!(event->state & SHIFTMASK)) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DESELECT_ALL, NULL);
        }
        dkTextField_handle(pthis, pthis, SEL_COMMAND, TF_ID_CURSOR_HOME, NULL);
        if (event->state & SHIFTMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_EXTEND, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_MARK, NULL);
        }
        return 1;
      case KEY_End:
      case KEY_KP_End:
        if (!(event->state & SHIFTMASK)) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DESELECT_ALL, NULL);
        }
        dkTextField_handle(pthis, pthis, SEL_COMMAND, TF_ID_CURSOR_END, NULL);
        if (event->state & SHIFTMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_EXTEND, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_MARK, NULL);
        }
        return 1;
      case KEY_Insert:
      case KEY_KP_Insert:
        if (event->state & CONTROLMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_COPY_SEL, NULL);
          return 1;
        }
        else if (event->state & SHIFTMASK) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_PASTE_SEL, NULL);
          return 1;
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_TOGGLE_OVERSTRIKE, NULL);
        }
        return 1;
      case KEY_Delete:
      case KEY_KP_Delete:
        if (dkWindowHasSelection(tf)) {
          if (event->state & SHIFTMASK) {
            dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_CUT_SEL, NULL);
          }
          else {
            dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DELETE_SEL, NULL);
          }
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DELETE, NULL);
        }
        return 1;
      case KEY_BackSpace:
        if (dkWindowHasSelection(tf)) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_DELETE_SEL, NULL);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_BACKSPACE, NULL);
        }
        return 1;
      case KEY_Return:
      case KEY_KP_Enter:
        if (dkTextField_isEditable((struct dkTextField *)tf)) {
          tf->flags |= FLAG_UPDATE;
          tf->flags &= ~FLAG_CHANGED;
          printf("tf->message = %d\n", tf->message);
          ((struct dkObject *)tf)->handle(tf, pthis, SEL_COMMAND, tf->message, (void *)event->text.str);
        }
        else {
          dkAppBeep(((struct dkWindow *)tf)->app);
        }
        return 1;
      case KEY_a:
        if (!(event->state & CONTROLMASK)) goto ins;
        dkTextField_handle(pthis, pthis, SEL_COMMAND, TF_ID_SELECT_ALL, NULL);
        return 1;
      case KEY_x:
        if (!(event->state & CONTROLMASK)) goto ins;
      case KEY_F20:   /* Sun Cut key */
        dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_CUT_SEL, NULL);
        return 1;
      case KEY_c:
        if (!(event->state & CONTROLMASK)) goto ins;
      case KEY_F16:  /* Sun Copy key */
        dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_COPY_SEL, NULL);
        return 1;
      case KEY_v:
        if (!(event->state & CONTROLMASK)) goto ins;
      case KEY_F18:  /* Sun Paste key */
        dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_PASTE_SEL, NULL);
        return 1;
      default:
ins:    if ((event->state & (CONTROLMASK | ALTMASK)) || ((DKuchar)event->text.str[0] < 32)) return 0;
        if (dkTextField_isOverstrike((struct dkTextField *)tf)) {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_OVERST_STRING, (void *)event->text.str);
        }
        else {
          dkTextField_handle(pthis, pthis, SEL_COMMAND, ID_INSERT_STRING, (void *)event->text.str);
        }
        return 1;
    }
  }
  return 0;
}

static long dkTextField_onKeyRelease(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkEvent *event = (struct dkEvent *)ptr;
  struct dkWindow *tf = (struct dkWindow *)pthis;

  if (dkWindowIsEnabled(tf)) {
    DKTRACE((200, "%s::onKeyPress keysym=0x%04x state=%04x\n", ((struct dkObject *)tf)->meta->className, event->code, event->state));
    if (tf->target && tf->target->handle(tf->target, pthis, SEL_KEYRELEASE, tf->message, ptr))
      return 1;
    switch (event->code) {
      case KEY_Right:
      case KEY_KP_Right:
      case KEY_Left:
      case KEY_KP_Left:
      case KEY_Home:
      case KEY_KP_Home:
      case KEY_End:
      case KEY_KP_End:
      case KEY_Insert:
      case KEY_KP_Insert:
      case KEY_Delete:
      case KEY_BackSpace:
      case KEY_Return:
      case KEY_F20:                    /* Sun Cut key */
      case KEY_F16:                    /* Sun Copy key */
      case KEY_F18:                    /* Sun Paste key */
        return 1;
      case KEY_a:
      case KEY_x:
      case KEY_c:
      case KEY_v:
        if (event->state & CONTROLMASK) return 1;
      default:
        if ((event->state & (CONTROLMASK | ALTMASK)) || ((DKuchar)event->text.str[0] < 32))
          return 0;
        return 1;
    }
  }
  return 0;
}

/* Insert a string */
static long dkTextField_onCmdInsertString(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;

  if (dkTextField_isEditable(tf)) {
    struct dstr tentative;
    int len;
    int reppos;
    int replen;

    dstr_copy(&tentative, tf->contents);
    len = strlen((char *)ptr);
    reppos = tf->cursor;
    replen = 0;

    if (dkWindowHasSelection((struct dkWindow *)tf)) {
      reppos = FXMIN(tf->anchor, tf->cursor);
      replen = FXMAX(tf->anchor, tf->cursor) - reppos;
    }
    dstr_replace(&tentative, reppos, replen, (char *)ptr, len);

    /* SEL_VERIFY */

    dkTextField_setCursorPos(tf, reppos);
    dkTextField_setAnchorPos(tf, reppos);
    dstr_copy(tf->contents, &tentative);
    dstr_destroy(&tentative);
    dkTextField_layout((struct dkWindow *)tf);
    dkTextField_setCursorPos(tf, reppos + len);
    dkTextField_setAnchorPos(tf, reppos + len);
    dkTextField_makePositionVisible(tf, reppos + len);
    dkTextField_killSelection(tf);
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
    ((struct dkWindow *)tf)->flags |= FLAG_CHANGED;
  }
  else {
    dkAppBeep(((struct dkWindow *)tf)->app);
  }

  return 1;

}

/* Backspace character */
static long dkTextField_onCmdBackspace(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;

  if (dkTextField_isEditable(tf) && tf->cursor > 0) {
    dkTextField_setCursorPos(tf, dstr_dec(tf->contents, tf->cursor));
    dkTextField_setAnchorPos(tf, tf->cursor);
    dstr_erase(tf->contents, tf->cursor, ds_extent(tf->contents, tf->cursor));
    dkTextField_layout((struct dkWindow *)tf);
    dkTextField_makePositionVisible(tf, tf->cursor);
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
    ((struct dkWindow *)tf)->flags |= FLAG_CHANGED;
  }
  else {
    dkAppBeep(((struct dkWindow *)tf)->app);
  }
  return 1;
}

/* Move cursor to begin of line */
static long dkTextField_onCmdCursorHome(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_setCursorPos(tf, 0);
  dkTextField_makePositionVisible(tf, 0);
  return 1;
}

/* Move cursor to end of line */
static long dkTextField_onCmdCursorEnd(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_setCursorPos(tf, dstr_getlength(tf->contents));
  dkTextField_makePositionVisible(tf, tf->cursor);
  return 1;
}

/* Move cursor left */
static long dkTextField_onCmdCursorLeft(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_setCursorPos(tf, dstr_dec(tf->contents, tf->cursor));
  dkTextField_makePositionVisible(tf, tf->cursor);
  return 1;
}

/* Move cursor right */
static long dkTextField_onCmdCursorRight(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_setCursorPos(tf, dstr_inc(tf->contents, tf->cursor));
  dkTextField_makePositionVisible(tf, tf->cursor);
  return 1;
}

static long dkTextField_onCmdDelete(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;

  if (dkTextField_isEditable(tf) && tf->cursor < dstr_getlength(tf->contents)) {
    dstr_erase(tf->contents, tf->cursor, ds_extent(tf->contents, tf->cursor));
    dkTextField_layout((struct dkWindow *)tf);
    dkTextField_setCursorPos(tf, tf->cursor);
    dkTextField_setAnchorPos(tf, tf->cursor);
    dkTextField_makePositionVisible(tf, tf->cursor);
    dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
    ((struct dkWindow *)tf)->flags |= FLAG_CHANGED;
  }
  else {
    dkAppBeep(((struct dkWindow *)tf)->app);
  }
  return 1;
}

/* Extend selection */
DKbool dkTextField_extendSelection(struct dkTextField *tf, int pos)
{
  DKDragType types[4];

  /* Validate position to start of character */
  pos = dkString_validate(tf->contents->str, DKCLAMP(0, pos, dstr_getlength(tf->contents)));

  /* Got a selection at all? */
  if (tf->anchor != pos) {
    types[0] = dkstringType;
    types[1] = dktextType;
    types[2] = dkutf8Type;
    types[3] = dkutf16Type;
    if (!dkWindowHasSelection((struct dkWindow *)tf)) {
      dkWindowAcquireSelection((struct dkWindow *)tf, types, 4);
    }
  } else {
    if (dkWindowHasSelection((struct dkWindow *)tf)) {
      dkWindow_releaseSelection((struct dkWindow *)tf);
    }
  }

  dkWindowUpdateRect((struct dkWindow *)tf, ((struct dkFrame *)tf)->border, ((struct dkFrame *)tf)->border, ((struct dkWindow *)tf)->width - (((struct dkFrame *)tf)->border << 1), ((struct dkWindow *)tf)->height - (((struct dkFrame *)tf)->border << 1));
  return TRUE;
}

/* Mark */
static long dkTextField_onCmdMark(void *pthis, struct dkObject *obj, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_setAnchorPos(tf, tf->cursor);
  return 1;
}

/* Extend */
static long dkTextField_onCmdExtend(void *pthis, struct dkObject *obj, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  dkTextField_extendSelection(tf, tf->cursor);
  return 1;
}

/* We now really do have the selection; repaint the text field */
static long dkTextField_onSelectionGained(void *pthis, struct dkObject *sender, DKSelector selhi, DKSelector sello, void *ptr)
{
  dkWindow_onSelectionGained(pthis, sender, selhi, sello, ptr);
  dkWindowUpdate((struct dkWindow *)pthis);
  return 1;
}

/* We lost the selection somehow; repaint the text field */
static long dkTextField_onSelectionLost(void *pthis, struct dkObject *sender, DKSelector selhi, DKSelector sello, void *ptr)
{
  dkWindow_onSelectionLost(pthis, sender, selhi, sello, ptr);
  dkWindowUpdate((struct dkWindow *)pthis);
  return 1;
}

/* Delete selection */
static long dkTextField_onCmdDeleteSel(void *pthis, struct dkObject *sender, void *ptr)
{
  struct dkTextField *tf = (struct dkTextField *)pthis;
  int st, en;

  if (dkTextField_isEditable(tf)) {
    if (!dkWindowHasSelection((struct dkWindow *)tf)) return 1;
    st = FXMIN(tf->anchor, tf->cursor);
    en = FXMAX(tf->anchor, tf->cursor);
    dkTextField_setCursorPos(tf, st);
    dkTextField_setAnchorPos(tf, en);
    dstr_erase(tf->contents, st, en - st);
    dkTextField_layout((struct dkWindow *)tf);
    dkTextField_makePositionVisible(tf, st);
    dkTextField_killSelection(tf);
    ((struct dkWindow *)tf)->flags |= FLAG_CHANGED;
    if (((struct dkWindow *)tf)->target)
      ((struct dkWindow *)tf)->target->handle(((struct dkWindow *)tf)->target, pthis, SEL_CHANGED, ((struct dkWindow *)tf)->message, tf->contents->str);
  } else {
    dkAppBeep(((struct dkWindow *)tf)->app);
  }
  return 1;
}
