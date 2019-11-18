/******************************************************************************
 *                                                                            *
 *                    C o m p o s i t e   W i d g e t                         *
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
 * $Id: FXComposite.h,v 1.24 2006/01/22 17:57:59 fox Exp $                    *
 *****************************************************************************/

#ifndef FX_COMPOSITE_H
#define FX_COMPOSITE_H

#include "fxapp.h"
#include "fxdefs.h"

#include "fxwindow.h"

struct FXVisual;

/* Inherits from dkWindow */

/* Constructor for dkWindow for Root windows only */
void DtkCompositeRootInit(struct dkWindow *comp, struct dkApp *app, struct FXVisual *v);
void DtkCompositeShellCtor(struct dkWindow *pthis, struct dkApp *app, struct dkWindow *win, DKuint opts, int x, int y, int w, int h);
void dkCompositeInit(struct dkWindow *pthis, struct dkWindow *p, DKuint opts, int x, int y, int w, int h);
void dkCompositeVtblSetup(struct dkWindow *comp);

long dkComposite_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data);

void dkCompositeLayout(struct dkWindow *w);
int dkCompositeMaxChildWidth(struct dkWindow *comp);
int dkCompositeMaxChildHeight(struct dkWindow *comp);

#endif /* FX_COMPOSITE_H */
