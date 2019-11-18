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
 * $Id: FXAccelTable.cpp,v 1.51.2.1 2007/03/07 14:30:27 fox Exp $             *
 *****************************************************************************/

#include <string.h>

/*
  Notes:
  - We also dropped the "fx" since we now have namespaces to keep stuff out
    of each other's hair.
  - Turned parseAccel() cum suis to simple global functions.  The rules for
    friend declarations have change in GCC 4.1 so declaring them as friends
    is no longer possible.
  - We need to deal with X11 unicode keysyms (with 0x01000000 flag) in some
    way.
*/

/* Obtain hot key offset in string */
int fx_findHotKey(char *string)
{
  int pos = 0;
  int n = 0;
  int len = strlen(string);
  while (pos < len) {
    if (string[pos] == '&') {
      if (string[pos + 1] != '&') {
        return n;
      }
      pos++;
    }
    pos++;
    n++;
  }
  return -1;
}

/* Strip hot key from string */
char *fx_stripHotKey(char *string)
{
  char *result = strdup(string);
  int len = strlen(result);
  int i, j;

  for(i = j = 0; j < len; j++) {
    if (result[j] == '&') {
      if (result[j + 1] != '&') continue;
      j++;
    }
    result[i++]=result[j];
  }
  result[i] = '\0';
  return result;
}
