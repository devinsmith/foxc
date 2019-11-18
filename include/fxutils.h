/******************************************************************************
 *                                                                            *
 *                  FOX Definitions, Types, and Macros                        *
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
 * $Id: fxdefs.h,v 1.178.2.2 2006/11/09 23:21:43 fox Exp $                    *
 *****************************************************************************/

#ifndef FX_UTILS_H
#define FX_UTILS_H

#include "fxdefs.h"

/* Get highlight color */
DKColor makeHiliteColor(DKColor clr);
/* Get shadow color */
DKColor makeShadowColor(DKColor clr);

char *xstrdup(char *str);

#endif /* FX_UTILS_H */
