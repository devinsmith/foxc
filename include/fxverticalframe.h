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
 * $Id: FXVerticalFrame.h,v 1.15 2006/01/22 17:58:12 fox Exp $                *
 *****************************************************************************/
#ifndef FX_VERTICALFRAME_H
#define FX_VERTICALFRAME_H

#include "fxpacker.h"

/**
* Vertical frame layout manager widget is used to automatically
* place child-windows vertically from top-to-bottom, or bottom-to-top,
* depending on the child window's layout hints.
*/

/* Inherits from dkPacker */

struct dkWindow *dkVerticalFrameNew(struct dkWindow *p, DKuint opts);
struct dkWindow *dkVerticalFrameNewEx(struct dkWindow *p, DKuint opts, int x, int y, int w, int h, int pl, int pr, int pt, int pb, int hs, int vs);

#endif /* FX_VERTICALFRAME_H */
