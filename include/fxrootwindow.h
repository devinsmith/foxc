/******************************************************************************
 *                                                                            *
 *                  R o o t   W i n d o w   W i d g e t                       *
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
 * $Id: FXRootWindow.h,v 1.23 2006/01/22 17:58:09 fox Exp $                   *
 *****************************************************************************/

#ifndef FX_ROOTWINDOW_H
#define FX_ROOTWINDOW_H

#include "fxapp.h"
#include "fxdefs.h"

#include "fxcomposite.h"

/* Functions implemented in dkrootwindow.c */

/* This function allocates and initializes a dkRootWindow.  You can think of
 * it as the constructor for the dkRootWindow */
struct dkWindow * DtkNewRootWindow(App *app, struct FXVisual *v);

#endif /* FX_ROOTWINDOW_H */
