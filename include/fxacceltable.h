/******************************************************************************
 *                                                                            *
 *                A c c e l e r a t o r   T a b l e   C l a s s               *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 1998,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: FXAccelTable.h,v 1.27 2006/01/22 17:57:58 fox Exp $                   *
 *****************************************************************************/

#ifndef FXACCELTABLE_H
#define FXACCELTABLE_H

/**
  * Strip hot key combination from the string.
  * For example, stripHotKey("Salt && &Pepper") should
  * yield "Salt & Pepper".
  */
int fx_findHotKey(char *string);
char * fx_stripHotKey(char *string);

#endif /* FXACCELTABLE_H */

