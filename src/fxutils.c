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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "fxutils.h"

/* Global flag which controls tracing level */
unsigned int dkTraceLevel = 0;

/* Get highlight color */
DKColor makeHiliteColor(DKColor clr)
{
	DKuint r, g, b;
	r = FXREDVAL(clr);
	g = FXGREENVAL(clr);
	b = FXBLUEVAL(clr);
	r = FXMAX(31, r);
	g = FXMAX(31, g);
	b = FXMAX(31, b);
	r = (133 * r) / 100;
	g = (133 * g) / 100;
	b = (133 * b) / 100;
	r = FXMIN(255, r);
	g = FXMIN(255, g);
	b = FXMIN(255, b);
	return FXRGB(r, g, b);
}

/* Get shadow color */
DKColor makeShadowColor(DKColor clr)
{
	DKuint r, g, b;
	r = FXREDVAL(clr);
	g = FXGREENVAL(clr);
	b = FXBLUEVAL(clr);
	r = (66 * r) / 100;
	g = (66 * g) / 100;
	b = (66 * b) / 100;
	return FXRGB(r, g, b);
}

char *xstrdup(char *str)
{
	char *out;

	out = strdup(str);
	if (out == NULL)
		exit(0);
	return out;
}

/* Error routine */
void dkerror(const char* format,...)
{
#ifndef WIN32
  va_list arguments;
  va_start(arguments, format);
  vfprintf(stderr, format, arguments);
  fflush(stderr);
  va_end(arguments);
  abort();
#else
#ifdef _WINDOWS
  char msg[MAXMESSAGESIZE];
  va_list arguments;
  va_start(arguments, format);
  vsnprintf(msg, sizeof(msg), format, arguments);
  va_end(arguments);
  OutputDebugStringA(msg);
  fprintf(stderr, "%s", msg); // if a console is available
  fflush(stderr);
  MessageBoxA(NULL, msg, NULL, MB_OK | MB_ICONEXCLAMATION | MB_APPLMODAL);
  DebugBreak();
#else
  va_list arguments;
  va_start(arguments, format);
  vfprintf(stderr, format, arguments);
  fflush(stderr);
  va_end(arguments);
  abort();
#endif
#endif
}

/* Trace printout routine */
void dktrace(unsigned int level, char *format, ...)
{
  if (dkTraceLevel > level) {
#ifndef WIN32
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    fflush(stderr);
    va_end(arguments);
#else
#ifdef _WINDOWS
    char msg[MAXMESSAGESIZE];
    va_list arguments;
    va_start(arguments, format);
    vsnprintf(msg, sizeof(msg), format, arguments);
    OutputDebugStringA(msg);
    fprintf(stderr, "%s", msg); /* if a console is available */
    fflush(stderr);
    va_end(arguments);
#else
    va_list arguments;
    va_start(arguments, format);
    vfprintf(stderr, format, arguments);
    fflush(stderr);
    va_end(arguments);
#endif
#endif
  }
}
