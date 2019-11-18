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
 * $Id: FXString.cpp,v 1.218.2.1 2006/08/15 05:03:16 fox Exp $                *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fxstring.h"

/* The string buffer is always rounded to a multiple of ROUNDVAL
 * which must be 2^n.  Thus, small size changes will not result in any
 * actual resizing of the buffer except when ROUNDVAL is exceeded. */
#define ROUNDVAL    16

/* Round up to nearest ROUNDVAL */
#define ROUNDUP(n)  (((n)+ROUNDVAL-1)&-ROUNDVAL)

/* This will come in handy */
#define EMPTY       ((char*)&emptystring[1])

/* For conversion from UTF16 to UTF32 */
const int SURROGATE_OFFSET = 0x10000 - (0xD800 << 10) - 0xDC00;

/* For conversion of UTF32 to UFT16 */
const int LEAD_OFFSET = 0xD800 - (0x10000 >> 10);

// Length of a utf8 character representation
const signed char utfBytes[256] = {
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
  3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
};

/* Empty string */
static const int emptystring[2] = {0,0};

static int dstr_nlen(DKnchar *src);
static int dstr_wlen(DKwchar *src);

/* Copy wide character substring of length n to dst */
int dstr_wc2utfs2(char* dst, DKwchar *src, int n)
{
  int len=0;
  int p=0;
  DKwchar w;
  while(p<n){
    w=src[p++];
    if(w<0x80){
      dst[len++]=w;
      continue;
      }
    if(w<0x800){
      dst[len++]=(w>>6)|0xC0;
      dst[len++]=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x10000){
      dst[len++]=(w>>12)|0xE0;
      dst[len++]=((w>>6)&0x3F)|0x80;
      dst[len++]=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x200000){
      dst[len++]=(w>>18)|0xF0;
      dst[len++]=((w>>12)&0x3F)|0x80;
      dst[len++]=((w>>6)&0x3F)|0x80;
      dst[len++]=(w&0x3F)|0x80;
      continue;
    }
    if(w<0x4000000){
      dst[len++]=(w>>24)|0xF8;
      dst[len++]=((w>>18)&0x3F)|0x80;
      dst[len++]=((w>>12)&0x3F)|0x80;
      dst[len++]=((w>>6)&0x3F)|0x80;
      dst[len++]=(w&0x3F)|0x80;
      continue;
      }
    dst[len++]=(w>>30)|0xFC;
    dst[len++]=((w>>24)&0X3F)|0x80;
    dst[len++]=((w>>18)&0X3F)|0x80;
    dst[len++]=((w>>12)&0X3F)|0x80;
    dst[len++]=((w>>6)&0X3F)|0x80;
    dst[len++]=(w&0X3F)|0x80;
    }
  return len;
  }

/* Copy wide character string to dst */
int dstr_wc2utfs(char* dst, DKwchar *src)
{
  return dstr_wc2utfs2(dst, src, dstr_wlen(src) + 1);
}

/* Copy narrow character substring of length n to dst
 * Test for surrogates is deferred till code possibly exceeds 0xD800 */
int dstr_nc2utfs2(char* dst, DKnchar *src, int n)
{
  int len = 0;
  int p = 0;
  DKwchar w;

  while (p < n) {
    w = src[p++];
    if(w < 0x80){
      dst[len++] = w;
      continue;
    }
    if (w < 0x800) {
      dst[len++] = (w >> 6) | 0xC0;
      dst[len++] = (w & 0x3F) | 0x80;
      continue;
    }
    if (0xD800 <= w && w < 0xDC00 && p < n) {
      w = (w << 10) + src[p++] + SURROGATE_OFFSET;
    }
    if (w < 0x10000) {
      dst[len++] = (w >> 12) | 0xE0;
      dst[len++] = ((w >> 6) & 0x3F) | 0x80;
      dst[len++] = (w & 0x3F) | 0x80;
      continue;
    }
    if(w<0x200000){
      dst[len++]=(w>>18)|0xF0;
      dst[len++]=((w>>12)&0x3F)|0x80;
      dst[len++]=((w>>6)&0x3F)|0x80;
      dst[len++]=(w&0x3F)|0x80;
      continue;
      }
    if(w<0x4000000){
      dst[len++]=(w>>24)|0xF8;
      dst[len++]=((w>>18)&0x3F)|0x80;
      dst[len++]=((w>>12)&0x3F)|0x80;
      dst[len++]=((w>>6)&0x3F)|0x80;
      dst[len++]=(w&0x3F)|0x80;
      continue;
      }
    dst[len++]=(w>>30)|0xFC;
    dst[len++]=((w>>24)&0X3F)|0x80;
    dst[len++]=((w>>18)&0X3F)|0x80;
    dst[len++]=((w>>12)&0X3F)|0x80;
    dst[len++]=((w>>6)&0X3F)|0x80;
    dst[len++]=(w&0X3F)|0x80;
  }
  return len;
}

/* Copy narrow character string to dst */
int dstr_nc2utfs(char* dst, DKnchar *src)
{
  return dstr_nc2utfs2(dst, src, dstr_nlen(src) + 1);
}

struct dstr *dstr_new_empty(void)
{
  struct dstr *s;

  s = malloc(sizeof(struct dstr));
  dstr_init(s);
  return s;
}

void dstr_init(struct dstr *s)
{
  s->str = EMPTY;
}

/* Length of ansi character string */
int dstr_len(char *src)
{
  return strlen(src);
}

/* Length of wide character string */
static int dstr_wlen(DKwchar *src)
{
  int i = 0;
  while (src[i]) i++;
  return i;
}

/* Length of narrow character string */
static int dstr_nlen(DKnchar *src)
{
  int i = 0;
  while (src[i]) i++;
  return i;
}

/* Length of utf8 representation of narrow characters string str of length n
 * Test for surrogates is deferred till code possibly exceeds 0xD800 */
static int dstr_utfslen_n2(DKnchar *str, int n)
{
  int len = 0;
  int p = 0;
  DKwchar w;

  while (p < n) {
    w = str[p++];
    len++;
    if (0x80 <= w) {
      len++;
      if (0x800 <= w) {
        len++;
        if (0xD800 <= w && w < 0xDC00 && p < n) {
          w = (w << 10) + str[p++] + SURROGATE_OFFSET;
        }
        if (0x10000 <= w) {
          len++;
          if (0x200000 <= w) {
            len++;
            if (0x4000000 <= w) {
              len++;
            }
          }
        }
      }
    }
  }
  return len;
}

/* Length of utf8 representation of narrow characters string str */
int dstr_utfslen_n(DKnchar *str)
{
  return dstr_utfslen_n2(str, dstr_nlen(str));
}

/* Length of utf8 representation of wide characters string str of length n */
static int dstr_utfslen_w2(DKwchar *str, int n)
{
  int len = 0;
  int p = 0;
  DKwchar w;
  while (p < n) {
    w = str[p++];
    len++;
    if (0x80 <= w) { len++;
    if (0x800 <= w) { len++;
    if (0x10000 <= w) { len++;
    if (0x200000 <= w) { len++;
    if (0x4000000 <= w) { len++; }}}}}
  }
  return len;
}

/* Length of utf8 representation of wide character string str */
int dstr_utfslen_w(DKwchar *str)
{
  return dstr_utfslen_w2(str, dstr_wlen(str));
}

/* Construct and init with narrow character substring */
void dstr_initn2(struct dstr *str, DKnchar *s, int m)
{
  str->str = EMPTY;
  if (s && m > 0) {
    int n = dstr_utfslen_n2(s, m);
    dstr_setlength(str, n);
    dstr_nc2utfs2(str->str, s, m);
  }
}

/* Construct and init with wide character substring */
void dstr_initw2(struct dstr *str, DKwchar *s, int m)
{
  str->str = EMPTY;
  if (s && m > 0) {
    int n = dstr_utfslen_w2(s,m);
    dstr_setlength(str, n);
    dstr_wc2utfs2(str->str, s, m);
  }
}

void dstr_copy(struct dstr *dest, struct dstr *src)
{
  int n;

  dest->str = EMPTY;
  n = dstr_getlength(src);
  if (n > 0) {
    dstr_setlength(dest, n);
    memcpy(dest->str, src->str, n);
  }
}

/* Change the length of the string to len */
void dstr_setlength(struct dstr *s, int len)
{
  if (*(((int*)s->str) - 1) != len) {
    if (0 < len) {
      if (s->str == EMPTY)
        s->str = sizeof(int) + (char*)malloc(ROUNDUP(1 + len) + sizeof(int));
      else
        s->str = sizeof(int) + (char*)realloc(s->str - sizeof(int), ROUNDUP(1+len) + sizeof(int));
      s->str[len] = 0;
      *(((int*)s->str)-1) = len;
    }
    else if (s->str != EMPTY) {
      free(s->str - sizeof(int));
      s->str = EMPTY;
    }
  }
}

/* Length of text in bytes */
int dstr_getlength(struct dstr *s)
{
  return *(((int*)s->str)-1);
}

/* Count number of utf8 characters in subrange */
int dstr_count2(struct dstr *s, int pos, int len)
{
  int cnt = 0;
  while (pos < len) {
    pos += utfBytes[(DKuchar)s->str[pos]];
    cnt++;
  }
  return cnt;
}

/* Count number of utf8 characters */
int dstr_count(struct dstr *s)
{
  return dstr_count2(s, 0, dstr_getlength(s));
}

/* Return index of utf8 character at byte offset */
int dstr_index(struct dstr *s, int offs)
{
  int len = dstr_getlength(s);
  int i = 0;
  int p = 0;
  while (p < offs && p < len) {
    p += utfBytes[(DKuchar)s->str[p]];
    i++;
  }
  return i;
}

/* Return byte offset of utf8 character at index */
int dstr_offset(struct dstr *s, int indx)
{
  int len = dstr_getlength(s);
  int i = 0;
  int p = 0;
  while (i < indx && p < len) {
    p += utfBytes[(DKuchar)s->str[p]];
    i++;
  }
  return p;
}

/* Replace part of string */
void dstr_replace(struct dstr *str, int pos, int m, char *s, int n)
{
  int len;

  len = dstr_getlength(str);
  if (pos < 0) {
    m += pos;
    if (m < 0) m = 0;
    pos = 0;
  }
  if (pos + m > len) {
    if (pos > len) pos = len;
    m = len - pos;
  }
  if (m < n) {
    dstr_setlength(str, len + n - m);
    memmove(str->str + pos + n, str->str + pos + m, len - pos - m);
  }
  else if (m > n) {
    memmove(str->str + pos + n, str->str + pos + m, len - pos - m);
    dstr_setlength(str, len + n - m);
  }
  memcpy(str->str + pos, s, n);
}

/* Return number of FXchar's of wide character at ptr */
int dk_wclen(char *ptr)
{
  return utfBytes[(int)ptr[0]];
}

int dkStringFind(char *str, char c)
{
	int len = strlen(str);
	int p = 0; /* pos; */
	int cc = c;

	if (p < 0) p = 0;
	while (p < len) { if (str[p] == cc) { return p; } ++p; }
	return -1;
}

char * dkStringLeft(char *str, int n)
{
	if (n > 0) {
		int len = strlen(str);
		if (n > len) n = len;
		return str + n;
	}
	return NULL;
}

/* Copy utf8 string of length n to narrow character string dst
 * Assume surrogates are needed if utf8 code is more than 16 bits */
int dkString_utf2ncs(DKnchar *dst, char *src, int n)
{
	int len = 0;
	int p = 0;
	DKwchar w;

	while (p < n) {
		w = (DKuchar)src[p++];
		if (0xC0 <= w) { w = (w << 6) ^ (DKuchar)src[p++] ^ 0x3080;
		if (0x800 <= w) { w = (w << 6) ^ (DKuchar)src[p++] ^ 0x20080;
		if (0x10000 <= w) { w = (w << 6) ^ (DKuchar)src[p++] ^ 0x400080;
		if (0x200000 <= w) { w = (w << 6) ^ (DKuchar)src[p++] ^ 0x8000080;
		if (0x4000000 <= w) { w = (w << 6) ^ (DKuchar)src[p++] ^ 0x80; }} dst[len++] = (w >> 10) + LEAD_OFFSET; w = (w & 0x3FF) + 0xDC00; }}}
		dst[len++] = w;
	}
	return len;
}

/* Return start of utf8 character containing position */
int wcvalidate(char* string, int pos)
{
  return (void)(pos <= 0 || DKISUTF(string[pos]) || --pos <= 0 || DKISUTF(string[pos]) || --pos <= 0 ||
    DKISUTF(string[pos]) || --pos <= 0 || DKISUTF(string[pos]) || --pos <= 0 || DKISUTF(string[pos]) || --pos), pos;
}

/* Return start of utf16 character containing position */
int wcvalidateW(DKnchar *string, int pos)
{
  return (void)(pos <= 0 || !(0xDC00 <= string[pos] && string[pos] <= 0xDFFF) || --pos), pos;
}

/* Advance to next utf8 character start */
int wcinc(char* string, int pos) {
  return (void)(string[pos++] == 0 || DKISUTF(string[pos]) || string[pos++]==0 || DKISUTF(string[pos]) ||
    string[pos++]==0 || DKISUTF(string[pos]) || string[pos++]==0 || DKISUTF(string[pos]) || string[pos++]==0
    || DKISUTF(string[pos]) || ++pos), pos;
}

/* Advance to next utf16 character start */
int wcincW(DKnchar *string, int pos) {
  return (void)((0xDC00 <= string[++pos] && string[pos] <= 0xDFFF) || ++pos), pos;
}

/* Retreat to previous utf8 character set */
int wcdec(char *string, int pos)
{
  if (--pos <= 0 || DKISUTF(string[pos]) || --pos <= 0 || DKISUTF(string[pos]) ||
      --pos <= 0 || DKISUTF(string[pos]) || --pos <= 0 || DKISUTF(string[pos]) ||
      --pos <= 0 || DKISUTF(string[pos]) || --pos)
    return pos;
  return pos;
}

int wcdec16(DKnchar *string, int pos)
{
  if (--pos <= 0 || !(0xDC00 <= string[pos] && string[pos] <= 0xDFFF) || --pos)
    return pos;
  return pos;
}

int dstr_dec(struct dstr *s, int p)
{
  return wcdec(s->str, p);
}

/* Remove section from buffer (n = number of characters) */
void dstr_erase(struct dstr *s, int pos, int n)
{
  if (n > 0) {
    int len = dstr_getlength(s);
    if (pos < len && pos + n > 0) {
      if (pos < 0) { n += pos; pos = 0; }
      if (pos + n > len) { n = len - pos; }
      memmove(s->str + pos, s->str + pos + n, len - pos - n);
      dstr_setlength(s, len - n);
    }
  }
}

/* Return start of utf8 character containing position */
int dkString_validate(char *str, int p)
{
  return wcvalidate(str, p);
}

int dkString_validateW(DKnchar *str, int p)
{
  return wcvalidateW(str, p);
}

/* Advance to next utf8 character start */
int dstr_inc(struct dstr *s, int p)
{
  return wcinc(s->str, p);
}

int dkString_incW(DKnchar *str, int p)
{
  return wcincW(str, p);
}

/// Return extent of utf8 character at position
int ds_extent(struct dstr *s, int p)
{
  return utfBytes[(DKuchar)s->str[p]];
}

/* Delete */
void dstr_destroy(struct dstr *d)
{
  if (d->str != EMPTY) {
    free(d->str - sizeof(int));
  }
}
