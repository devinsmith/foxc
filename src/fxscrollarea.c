/******************************************************************************
 *                                                                            *
 *                    S c r o l l A r e a   W i d g e t                       *
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
 * $Id: FXScrollArea.cpp,v 1.49 2006/01/22 17:58:40 fox Exp $                 *
 *****************************************************************************/

#include "fxscrollarea.h"


/*
  To do:
  - In new HSCROLLING_OFF mode, default width should be computed
    from contents (new virtual for that), and presence of scrollbars
    (determined by flags, as well as need).
  - The original content size should be returned from getContentWidth(),
    and getContentHeight().
  - When tabbing, we will never put focus on scrollbar.
  - Perhaps scroll windows should observe FRAME_SUNKEN etc.
  - Here's a new idea:- perhaps the scrollbars should be GUI-updated from the
    FXScrollArea.  Then layout() will do nothing but place the bars.
  - What if we want to keep two scrolled windows in sync, i.e. scroll them
    both the same amount.
  - Do we need to be able to map mouse wheel to horizontal scrollbar instead?
    or perhaps even both ways (depending on whether scrolling is possible
    in one or another direction).
*/


#define AUTOSCROLL_FUDGE  11       // Proximity to wall at which we start autoscrolling
#define SCROLLER_MASK     (HSCROLLER_ALWAYS|HSCROLLER_NEVER|VSCROLLER_ALWAYS|VSCROLLER_NEVER|SCROLLERS_DONT_TRACK)

/*******************************************************************************/
#if 0
FXDEFMAP(FXScrollArea) FXScrollAreaMap[]={
  FXMAPFUNC(SEL_MOUSEWHEEL,0,FXScrollArea::onVMouseWheel),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_HSCROLLED,FXScrollArea::onHScrollerChanged),
  FXMAPFUNC(SEL_COMMAND,FXWindow::ID_VSCROLLED,FXScrollArea::onVScrollerChanged),
  FXMAPFUNC(SEL_CHANGED,FXWindow::ID_HSCROLLED,FXScrollArea::onHScrollerDragged),
  FXMAPFUNC(SEL_CHANGED,FXWindow::ID_VSCROLLED,FXScrollArea::onVScrollerDragged),
  FXMAPFUNC(SEL_TIMEOUT,FXWindow::ID_AUTOSCROLL,FXScrollArea::onAutoScroll),
  };


// Object implementation
FXIMPLEMENT(FXScrollArea,FXComposite,FXScrollAreaMap,ARRAYNUMBER(FXScrollAreaMap))
#endif

static int dkScrollArea_getViewportWidth(struct dkWindow *w);
static int dkScrollArea_getViewportHeight(struct dkWindow *w);
static int dkScrollArea_getContentWidth(struct dkWindow *w);
static int dkScrollArea_getContentHeight(struct dkWindow *w);

// Scroll acceleration near edge
static const int acceleration[AUTOSCROLL_FUDGE+1]={1,1,1,2,3,4,6,7,8,16,32,64};

long dkScrollArea_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
  /* FIXME!!! */
  return dkComposite_handle(pthis, obj, selhi, sello, data);
}

// Construct and init
struct dkScrollArea *dkScrollAreaNew(struct dkWindow *p, DKuint opts, int x, int y, int w, int h)
{
  struct dkScrollArea *ret;

  ret = malloc(sizeof(struct dkScrollArea));
  dkScrollAreaInit(ret, p, opts, x, y, w, h);
  return ret;
}

void dkScrollAreaInit(struct dkScrollArea *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h)
{
  DKuint jumpy = 0;
  dkCompositeInit((struct dkWindow *)pthis, p, opts, x, y, w, h);

  /* Setup overloads */
  ((struct dkObject *)pthis)->handle = dkScrollArea_handle;
  ((struct dkWindow *)pthis)->getDefaultWidth = dkScrollArea_getDefaultWidth;
  ((struct dkWindow *)pthis)->getDefaultHeight = dkScrollArea_getDefaultHeight;
  ((struct dkWindow *)pthis)->layout = dkScrollArea_layout;
  pthis->getViewportWidth = dkScrollArea_getViewportWidth;
  pthis->getViewportHeight = dkScrollArea_getViewportHeight;
  pthis->getContentWidth = dkScrollArea_getContentWidth;
  pthis->getContentHeight = dkScrollArea_getContentHeight;

  /* Setup rest of object */
  ((struct dkWindow *)pthis)->flags |= FLAG_SHOWN;
  if (opts & SCROLLERS_DONT_TRACK) jumpy = SCROLLBAR_WHEELJUMP;
  pthis->horizontal = dkScrollBarNew((struct dkWindow *)pthis, (struct dkObject *)pthis, WIN_ID_HSCROLLED, SCROLLBAR_HORIZONTAL | jumpy, 0, 0, 0, 0);
  pthis->vertical = dkScrollBarNew((struct dkWindow *)pthis, (struct dkObject *)pthis, WIN_ID_VSCROLLED, SCROLLBAR_VERTICAL | jumpy, 0, 0, 0, 0);
  pthis->corner = dkScrollCornerNew((struct dkComposite *)pthis);
  ((struct dkWindow *)pthis)->backColor = ((struct dkWindow *)pthis)->app->backColor;
  pthis->viewport_w = 1;
  pthis->viewport_h = 1;
  pthis->pos_x = 0;
  pthis->pos_y = 0;
}

// This should really add the scroll bar size only when required; however,
// that depends on the actual size.  We are potentially being called at
// a moment when this is not known yet, so we return a size which reflects
// the situation when the scrollbars have been placed; this way, we should
// at least have enough space to fully see the contents, and a bit extra
// when the scrollbars turn out to have been unnecessary.

// Get default width
int dkScrollArea_getDefaultWidth(struct dkWindow *win)
{
  int w = 0;
  int t;
  struct dkScrollArea *sa = (struct dkScrollArea *)win;

  if ((win->options & HSCROLLER_NEVER) && (win->options & HSCROLLER_ALWAYS))
    w = sa->getContentWidth((struct dkWindow *)sa);
  if (!(win->options & HSCROLLER_NEVER) &&
      (t = ((struct dkWindow *)sa->horizontal)->getDefaultWidth((struct dkWindow *)sa->horizontal)) > w)
    w=t;
  if (!(win->options & VSCROLLER_NEVER)) w += ((struct dkWindow *)sa->vertical)->getDefaultWidth((struct dkWindow *)sa->vertical);
  return FXMAX(w, 1);
}

// Get default height
int dkScrollArea_getDefaultHeight(struct dkWindow *win)
{
  int h = 0;
  int t;
  struct dkScrollArea *sa = (struct dkScrollArea *)win;

  if ((win->options & VSCROLLER_NEVER) && (win->options & VSCROLLER_ALWAYS))
    h = sa->getContentHeight((struct dkWindow *)sa);
  if (!(win->options & VSCROLLER_NEVER) &&
      (t = ((struct dkWindow *)sa->vertical)->getDefaultHeight((struct dkWindow *)sa->vertical)) >h )
    h=t;
  if (!(win->options & HSCROLLER_NEVER)) h += ((struct dkWindow *)sa->horizontal)->getDefaultHeight((struct dkWindow *)sa->horizontal);
  return FXMAX(h, 1);
}


// Move content
void dkScrollArea_moveContents(struct dkScrollArea *sa, int x, int y)
{
  int dx = x - sa->pos_x;
  int dy = y - sa->pos_y;
  sa->pos_x = x;
  sa->pos_y = y;
  dkWindow_scroll((struct dkWindow *)sa, 0, 0, sa->viewport_w, sa->viewport_h,
      dx, dy);
}
#if 0


// Changed
long FXScrollArea::onHScrollerChanged(FXObject*,FXSelector,void* ptr){
  FXint new_x=-(FXint)(FXival)ptr;
  if(new_x!=pos_x){
    moveContents(new_x,pos_y);
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Changed
long FXScrollArea::onVScrollerChanged(FXObject*,FXSelector,void* ptr){
  FXint new_y=-(FXint)(FXival)ptr;
  if(new_y!=pos_y){
    moveContents(pos_x,new_y);
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Dragged
long FXScrollArea::onHScrollerDragged(FXObject*,FXSelector,void* ptr){
  if(!(options&SCROLLERS_DONT_TRACK)){
    FXint new_x=-(FXint)(FXival)ptr;
    if(new_x!=pos_x){
      moveContents(new_x,pos_y);
      }
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Dragged
long FXScrollArea::onVScrollerDragged(FXObject*,FXSelector,void* ptr){
  if(!(options&SCROLLERS_DONT_TRACK)){
    FXint new_y=-(FXint)(FXival)ptr;
    if(new_y!=pos_y){
      moveContents(pos_x,new_y);
      }
    }
  flags&=~FLAG_TIP;
  return 1;
  }


// Mouse wheel used for vertical scrolling
long FXScrollArea::onVMouseWheel(FXObject* sender,FXSelector sel,void* ptr){
  vertical->handle(sender,sel,ptr);
  return 1;
  }


// Mouse wheel used for horizontal scrolling
long FXScrollArea::onHMouseWheel(FXObject* sender,FXSelector sel,void* ptr){
  horizontal->handle(sender,sel,ptr);
  return 1;
  }


// Timeout
long FXScrollArea::onAutoScroll(FXObject*,FXSelector sel,void* ptr){
  register FXEvent* event=(FXEvent*)ptr;
  register FXint dx=0;
  register FXint dy=0;

  // If scrolling only while inside, and not inside, we stop scrolling
  if((flags&FLAG_SCROLLINSIDE) && !(0<=event->win_x && 0<=event->win_y && event->win_x<viewport_w && event->win_y<viewport_h)) return 0;

  // Figure scroll amount x
  if(event->win_x<AUTOSCROLL_FUDGE) dx=AUTOSCROLL_FUDGE-event->win_x;
  else if(viewport_w-AUTOSCROLL_FUDGE<=event->win_x) dx=viewport_w-AUTOSCROLL_FUDGE-event->win_x;

  // Figure scroll amount y
  if(event->win_y<AUTOSCROLL_FUDGE) dy=AUTOSCROLL_FUDGE-event->win_y;
  else if(viewport_h-AUTOSCROLL_FUDGE<=event->win_y) dy=viewport_h-AUTOSCROLL_FUDGE-event->win_y;

  // Keep autoscrolling
  if(dx || dy){
    FXint oldposx=pos_x;
    FXint oldposy=pos_y;
    if(flags&FLAG_SCROLLINSIDE){
      FXASSERT(FXABS(dx)<=AUTOSCROLL_FUDGE);
      FXASSERT(FXABS(dy)<=AUTOSCROLL_FUDGE);
      dx*=acceleration[FXABS(dx)];
      dy*=acceleration[FXABS(dy)];
      }

    // Scroll a bit
    setPosition(pos_x+dx,pos_y+dy);

    // Setup next timer if we can still scroll some more
    if((pos_x!=oldposx) || (pos_y!=oldposy)){
      getApp()->addTimeout(this,FXSELID(sel),getApp()->getScrollSpeed(),event);
      }
    }

  // Kill tip
  flags&=~FLAG_TIP;
  return 0;
  }


// Start automatic scrolling
FXbool FXScrollArea::startAutoScroll(FXEvent *event,FXbool onlywheninside){
  register FXbool autoscrolling=FALSE;
  flags&=~FLAG_SCROLLINSIDE;
  if(onlywheninside) flags|=FLAG_SCROLLINSIDE;
  if(horizontal->getPage()<horizontal->getRange()){
    if((event->win_x<AUTOSCROLL_FUDGE) && (0<horizontal->getPosition())) autoscrolling=TRUE;
    else if((viewport_w-AUTOSCROLL_FUDGE<=event->win_x) && (horizontal->getPosition()<horizontal->getRange()-horizontal->getPage())) autoscrolling=TRUE;
    }
  if(vertical->getPage()<vertical->getRange()){
    if((event->win_y<AUTOSCROLL_FUDGE) && (0<vertical->getPosition())) autoscrolling=TRUE;
    else if((viewport_h-AUTOSCROLL_FUDGE<=event->win_y) && (vertical->getPosition()<vertical->getRange()-vertical->getPage())) autoscrolling=TRUE;
    }
  if(onlywheninside && (event->win_x<0 || event->win_y<0 || viewport_w<=event->win_x || viewport_h<=event->win_y)) autoscrolling=FALSE;
  if(autoscrolling){
    if(!getApp()->hasTimeout(this,ID_AUTOSCROLL)){
      getApp()->addTimeout(this,ID_AUTOSCROLL,getApp()->getScrollSpeed(),event);
      }
    }
  else{
    getApp()->removeTimeout(this,ID_AUTOSCROLL);
    }
  return autoscrolling;
  }


// Stop automatic scrolling
void FXScrollArea::stopAutoScroll(){
  getApp()->removeTimeout(this,ID_AUTOSCROLL);
  flags&=~FLAG_SCROLLINSIDE;
  }


// Set scroll style
void FXScrollArea::setScrollStyle(FXuint style){
  FXuint opts=(options&~SCROLLER_MASK) | (style&SCROLLER_MASK);
  if(options!=opts){
    if(opts&SCROLLERS_DONT_TRACK){
      horizontal->setScrollBarStyle(horizontal->getScrollBarStyle()|SCROLLBAR_WHEELJUMP);
      vertical->setScrollBarStyle(vertical->getScrollBarStyle()|SCROLLBAR_WHEELJUMP);
      }
    else{
      horizontal->setScrollBarStyle(horizontal->getScrollBarStyle()&~SCROLLBAR_WHEELJUMP);
      vertical->setScrollBarStyle(vertical->getScrollBarStyle()&~SCROLLBAR_WHEELJUMP);
      }
    options=opts;
    recalc();
    }
  }


// Get scroll style
FXuint FXScrollArea::getScrollStyle() const {
  return (options&SCROLLER_MASK);
  }


// True if horizontally scrollable enabled
FXbool FXScrollArea::isHorizontalScrollable() const {
  return !((options&HSCROLLER_NEVER) && (options&HSCROLLER_ALWAYS));
  }


// True if vertically scrollable enabled
FXbool FXScrollArea::isVerticalScrollable() const {
  return !((options&VSCROLLER_NEVER) && (options&VSCROLLER_ALWAYS));
  }

#endif

// Default viewport width
static int dkScrollArea_getViewportWidth(struct dkWindow *w)
{
  return w->width;
}


// Default viewport height
static int dkScrollArea_getViewportHeight(struct dkWindow *w)
{
  return w->height;
}

// Determine minimum content width of scroll area
static int dkScrollArea_getContentWidth(struct dkWindow *w)
{
  return 1;
}

// Determine minimum content height of scroll area
static int dkScrollArea_getContentHeight(struct dkWindow *w)
{
  return 1;
}

// Recalculate layout
void dkScrollArea_layout(struct dkWindow *w)
{
  int new_x, new_y, content_w, content_h;
  int sh_h = 0;
  int sv_w = 0;
  struct dkScrollArea *sa = (struct dkScrollArea *)w;

  // Initial viewport size
  sa->viewport_w = sa->getViewportWidth((struct dkWindow *)sa);
  sa->viewport_h = sa->getViewportHeight((struct dkWindow *)sa);

  // ALWAYS determine content size
  content_w = sa->getContentWidth((struct dkWindow *)sa);
  content_h = sa->getContentHeight((struct dkWindow *)sa);

  // Get dimensions of the scroll bars
  if (!(w->options & HSCROLLER_NEVER)) sh_h = ((struct dkWindow *)sa->horizontal)->getDefaultHeight((struct dkWindow *)sa->horizontal);
  if (!(w->options & VSCROLLER_NEVER)) sv_w = ((struct dkWindow *)sa->vertical)->getDefaultWidth((struct dkWindow *)sa->vertical);

  // Should we disable the scroll bars?  A bit tricky as the scrollbars
  // may influence each other's presence.  Also, we don't allow more than
  // 50% of the viewport to be taken up by scrollbars; when the scrollbars
  // take up more than 50% of the available space we simply turn them off.
  if (!(w->options & (HSCROLLER_ALWAYS | VSCROLLER_ALWAYS)) &&
      (content_w <= sa->viewport_w) && (content_h <= sa->viewport_h)) {
    sh_h = sv_w = 0;
  }
  if (!(w->options & HSCROLLER_ALWAYS) &&
      ((content_w <= sa->viewport_w - sv_w) ||
       (0 >= sa->viewport_h - sh_h - sh_h))) sh_h = 0;
  if (!(w->options & VSCROLLER_ALWAYS) &&
      ((content_h <= sa->viewport_h - sh_h) ||
       (0 >= sa->viewport_w - sv_w - sv_w))) sv_w = 0;
  if (!(w->options & HSCROLLER_ALWAYS) &&
      ((content_w <= sa->viewport_w - sv_w) ||
       (0 >= sa->viewport_h - sh_h - sh_h))) sh_h = 0;

  // Viewport size with scroll bars taken into account
  sa->viewport_w -= sv_w;
  sa->viewport_h -= sh_h;

  // Adjust content size, now that we know about those scroll bars
  if ((w->options & HSCROLLER_NEVER) && (w->options & HSCROLLER_ALWAYS))
    content_w = sa->viewport_w;
  if ((w->options & VSCROLLER_NEVER) && (w->options & VSCROLLER_ALWAYS))
    content_h = sa->viewport_h;

  // Furthermore, content size won't be smaller than the viewport
  if (content_w < sa->viewport_w) content_w = sa->viewport_w;
  if (content_h < sa->viewport_h) content_h = sa->viewport_h;

  // Content size
  dkScrollBar_setRange(sa->horizontal, content_w);
  dkScrollBar_setRange(sa->vertical, content_h);

  // Page size may have changed
  dkScrollBar_setPage(sa->horizontal, sa->viewport_w);
  dkScrollBar_setPage(sa->vertical, sa->viewport_h);

  // Position may have changed
  dkScrollBar_setPosition(sa->horizontal, -sa->pos_x);
  dkScrollBar_setPosition(sa->vertical, -sa->pos_y);

  // Get back the adjusted position
  new_x = -sa->horizontal->pos;
  new_y = -sa->vertical->pos;

  // Scroll to force position back into range
  if (new_x != sa->pos_x || new_y != sa->pos_y) {
    dkScrollArea_moveContents(sa, new_x, new_y);
  }

  // Read back validated position
  sa->pos_x = -sa->horizontal->pos;
  sa->pos_y = -sa->vertical->pos;

  // Hide or show horizontal scroll bar
  if (sh_h) {
    ((struct dkWindow *)sa->horizontal)->position((struct dkWindow *)sa->horizontal, 0,
      ((struct dkWindow *)sa)->height - sh_h,
      ((struct dkWindow *)sa)->width - sv_w, sh_h);
    DtkWindowShow((struct dkWindow *)sa->horizontal);
    dkWindow_raise((struct dkWindow *)sa->horizontal);
  } else {
    dkWindowHide((struct dkWindow *)sa->horizontal);
    }

  // Hide or show vertical scroll bar
  if (sv_w) {
    ((struct dkWindow *)sa->vertical)->position((struct dkWindow *)sa->vertical,
      ((struct dkWindow *)sa)->width - sv_w, 0, sv_w,
      ((struct dkWindow *)sa)->height - sh_h);
    DtkWindowShow((struct dkWindow *)sa->vertical);
    dkWindow_raise((struct dkWindow *)sa->vertical);
  } else {
    dkWindowHide((struct dkWindow *)sa->vertical);
  }

  // Hide or show scroll corner
  if (sv_w && sh_h) {
    ((struct dkWindow *)sa->corner)->position((struct dkWindow *)sa->corner,
      ((struct dkWindow *)sa)->width - sv_w,
      ((struct dkWindow *)sa)->height - sh_h, sv_w, sh_h);
    DtkWindowShow((struct dkWindow *)sa->corner);
    dkWindow_raise((struct dkWindow *)sa->corner);
  }
  else {
    dkWindowHide((struct dkWindow *)sa->corner);
  }

  // No more dirty
  w->flags &= ~FLAG_DIRTY;
}

#if 0

// Set position
void FXScrollArea::setPosition(FXint x,FXint y){
  FXint new_x,new_y;

  // Set scroll bars
  horizontal->setPosition(-x);
  vertical->setPosition(-y);

  // Then read back valid position from scroll bars
  new_x=-horizontal->getPosition();
  new_y=-vertical->getPosition();

  // Move content if there's a change
  if(new_x!=pos_x || new_y!=pos_y){
    moveContents(new_x,new_y);
    }
  }


// Clean up
FXScrollArea::~FXScrollArea(){
  getApp()->removeTimeout(this,ID_AUTOSCROLL);
  horizontal=(FXScrollBar*)-1L;
  vertical=(FXScrollBar*)-1L;
  corner=(FXScrollCorner*)-1L;
  }

#endif
