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
 * $Id: FXCursor.h,v 1.28 2006/01/22 17:58:00 fox Exp $                       *
 *****************************************************************************/

#ifndef FX_CURSOR_H
#define FX_CURSOR_H

#include "fxdefs.h"

/* Stock cursors */
enum DtkStockCursor {
	CURSOR_ARROW = 1,             /* Default left pointing arrow */
	CURSOR_RARROW,                /* Right arrow */
	CURSOR_IBEAM,                 /* Text I-Beam */
	CURSOR_WATCH,                 /* Stopwatch or hourglass */
	CURSOR_CROSS,                 /* Crosshair */
	CURSOR_UPDOWN,                /* Move up, down */
	CURSOR_LEFTRIGHT,             /* Move left, right */
	CURSOR_MOVE                   /* Move up,down,left,right */
};

struct dkCursor {
	struct dkApp *app;    /* Back link to application object */
	DKID xid;

	void *data;      /* Source data */
	int width;       /* Width */
	int height;      /* Height */
	int hotx;        /* Hot spot x */
	int hoty;        /* Hot spot y */
	unsigned int options;     /* Options */
};

dkCursor *FxCursorNew(App *app, enum DtkStockCursor curid);
void FxCursorCreate(dkCursor *c);

#endif /* FX_CURSOR_H */
