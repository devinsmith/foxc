/******************************************************************************
 *                                                                            *
 *                      C u r s o r - O b j e c t                             *
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
 * $Id: FXCursor.cpp,v 1.62.2.1 2006/06/09 00:50:16 fox Exp $                 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/cursorfont.h>
#endif

#include "fxapp.h"
#include "fxcursor.h"
#include "fxdc.h"

/*
  Notes:
  - Cursor size should be less than or equal to 32x32; limitation in Windows!
  - Need standard glyph for "invisible" cursor.
  - Keep hotx and hoty INSIDE the cursor glyph!!
  - Thanks Niall Douglas <s_sourceforge@nedprod.com> for the changes for
    alpha-blended cursors.
*/

#define CURSOR_MASK (255)

struct dkCursor *
FxCursorNew(App *app, enum DtkStockCursor curid)
{
	struct dkCursor *cur;

	cur = malloc(sizeof(struct dkCursor));

	DKTRACE((100,"dkCursor::dkCursor %p\n", cur));

	cur->xid = 0;
	cur->app = app;
	cur->data = NULL;
	cur->width = 0;
	cur->height = 0;
	cur->hotx = 0;
	cur->hoty = 0;
	cur->options = curid;

	return cur;
}

void
FxCursorCreate(dkCursor *c)
{
#ifdef WIN32
	LPCTSTR stock[] = {IDC_ARROW, IDC_ARROW, IDC_ARROW, IDC_IBEAM,
		IDC_WAIT, IDC_CROSS, IDC_SIZENS, IDC_SIZEWE, IDC_SIZEALL};
#else
/* Mapping to standard X11 cursors */
	unsigned int stock[] = {XC_left_ptr, XC_left_ptr, XC_right_ptr, XC_xterm,
		XC_watch, XC_crosshair, XC_sb_h_double_arrow, XC_sb_v_double_arrow,
		XC_fleur};
#endif

	if (c->xid)
		return;

	if (!c->app->display_opened)
		return;
#ifdef WIN32
	if (c->options & CURSOR_MASK) {
		c->xid = LoadCursor(NULL, stock[c->options & CURSOR_MASK]);
	}
#else
	/* Building stock cursor */
	if (c->options & CURSOR_MASK) {
		c->xid = XCreateFontCursor((Display *)c->app->display,
		    stock[c->options & CURSOR_MASK]);
	}
#endif
}

