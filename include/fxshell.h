/******************************************************************************
 *                                                                            *
 *                  S h e l l   W i n d o w   W i d g e t                     *
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
 * $Id: FXShell.h,v 1.31 2006/01/22 17:58:09 fox Exp $                        *
 *****************************************************************************/

#ifndef FX_SHELL_H
#define FX_SHELL_H

#include "fxapp.h"
#include "fxdefs.h"

#include "fxcomposite.h"

struct FXVisual;

/* Constructor for dkShell */
void DtkTopWindowShellCtor(struct dkWindow *pthis, struct dkApp *app, DKuint opts, int x, int y, int w, int h);
void DtkCreateShellWindow(void *pthis);

void dkShellRecalc(struct dkWindow *pthis);

long dkShell_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

#endif /* FX_SHELL_H */
