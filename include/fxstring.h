/******************************************************************************
 *                                                                            *
 *                        S t r i n g   O b j e c t                           *
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
 * $Id: FXString.h,v 1.120 2006/02/20 03:32:12 fox Exp $                      *
 *****************************************************************************/

#ifndef FX_STRING_H
#define FX_STRING_H

#include "fxdefs.h"

struct dstr {
  char *str;
};

struct dstr *dstr_new_empty(void);
void dstr_init(struct dstr *str);
void dstr_initn2(struct dstr *str, DKnchar *s, int m);
void dstr_initw2(struct dstr *str, DKwchar *s, int m);
void dstr_setlength(struct dstr *s, int len);
int dstr_getlength(struct dstr *s);
int dk_wclen(char *ptr);
int dkStringFind(char *str, char c);
char *dkStringLeft(char *str, int len);
int dkString_utf2ncs(DKnchar *dst, char *src, int n);
int dkString_validate(char *str, int p);
int dkString_validateW(DKnchar *str, int p);
int dstr_inc(struct dstr *, int p);
int dstr_dec(struct dstr *, int p);
int dkString_incW(DKnchar *str, int p);
int ds_extent(struct dstr *, int p);
void dstr_erase(struct dstr *s, int pos, int num);
int dstr_count(struct dstr *s);
int dstr_count2(struct dstr *s, int pos, int len);
int dstr_index(struct dstr *s, int offs);
int dstr_offset(struct dstr *s, int indx);
void dstr_copy(struct dstr *dest, struct dstr *src);
void dstr_destroy(struct dstr *d);
void dstr_replace(struct dstr *str, int pos, int m, char *s, int n);

#endif /* FX_STRING_H */
