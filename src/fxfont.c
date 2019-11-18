/******************************************************************************
 *                                                                            *
 *                            F o n t   O b j e c t                           *
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
 * $Id: FXFont.cpp,v 1.184.2.5 2009/02/07 05:42:01 fox Exp $                  *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifndef WIN32
#include "config.h"
#endif

#if defined(WIN32)
#include <windows.h>
#elif defined(HAVE_XFT_H)
#include <X11/Xft/Xft.h>
//#include <fontconfig.h>
#endif

#include "fxfont.h"
#include "fxapp.h"
#include "fxstring.h"
#include "fxutils.h"

extern int LEAD_OFFSET;

static void dkFont_setFont(struct dkFont *f, char *name);

struct dkFont *
dkFontNew(struct dkApp *app, char *name)
{
	struct dkFont *f;

	f = malloc(sizeof(struct dkFont));

	f->xid = 0;
	f->app = app;

	dkFont_setFont(f, name);

	return f;
}

/* Change font description from a string */
static void dkFont_setFont(struct dkFont *f, char *name)
{
	int len;
	char *pch;

	// Raw X11 font is only the name
	f->wantedName = xstrdup(name);
	f->wantedSize = 0;
	f->wantedWeight = 0;
	f->wantedSlant = 0;
	f->wantedSetwidth = 0;
	f->wantedEncoding = 0;
	f->hints = dkFont_X11;

	len = dkStringFind(f->wantedName, ',');
	if (len < 0) return;

	pch = strtok(f->wantedName, ",");
	if (pch != NULL)
		pch = strtok(NULL, ",");
	if (pch != NULL)
		f->wantedSize = strtoul(pch, NULL, 10);
}

#if defined(WIN32)
static void *dkFontMatch(struct dkFont *f, char *wantfamily, char *wantforge, DKuint wantsize, DKuint wantweight, int res)
{
	TEXTMETRIC *font;
	LOGFONT lf;
	char buffer[256];

	/* Hang on to this for text metrics functions */
	f->dc = CreateCompatibleDC(NULL);

	/* Now fill in the fields */
	lf.lfHeight = -MulDiv(wantsize, GetDeviceCaps((HDC)f->dc, LOGPIXELSY), 720);
	lf.lfWidth = 0;

	lf.lfEscapement = 0;
	lf.lfOrientation = 0;

	lf.lfWeight = wantweight * 10;
  /* TODO: Handle slant */
  lf.lfItalic = FALSE;
	lf.lfUnderline = FALSE;
	lf.lfStrikeOut = FALSE;

	lf.lfCharSet = DEFAULT_CHARSET;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;

	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = 0;
	lf.lfPitchAndFamily |= DEFAULT_PITCH;

	strncpy(lf.lfFaceName, wantfamily, sizeof(lf.lfFaceName) - 1);
  lf.lfFaceName[sizeof(lf.lfFaceName) - 1] = '\0';

	/* Here we go! */
	f->xid = CreateFontIndirect(&lf);

	/* Uh-oh, we failed */
	if (!f->xid) return NULL;

	/* Obtain text metrics */
	if ((font = calloc(sizeof(TEXTMETRIC), 1)) == NULL) return NULL;

	SelectObject((HDC)f->dc, f->xid);
	GetTextMetrics((HDC)f->dc, (TEXTMETRIC*)font);

	/* Get actual face name */
	GetTextFaceA((HDC)f->dc, sizeof(buffer), buffer);

	/* Return it */
	return font;
}
#elif defined(HAVE_XFT_H)       ///// XFT /////
static void *dkFontMatch(struct dkFont *f, char *wantfamily, char *wantforge, DKuint wantsize, DKuint wantweight, int res)
{
	FcPattern *pattern, *p;
	FcChar8   *fam,*fdy;
	//FcCharSet *charset;
	XftFont   *font;
	FcResult   result;

	/* Create pattern object */
	pattern = FcPatternCreate();

	/* Set family */
	if (wantfamily) {
		FcPatternAddString(pattern, FC_FAMILY, (const FcChar8*)wantfamily);
	}

	/* Set foundry */
	if (wantforge) {
		FcPatternAddString(pattern, FC_FOUNDRY, (const FcChar8*)wantforge);
	}

	/* Set pixel size, based on given screen res and desired point size */
	if (wantsize != 0) {
		FcPatternAddDouble(pattern, FC_PIXEL_SIZE, (res * wantsize) / 720.0);
	}

	/* Set font weight */
	if (wantweight != 0){
		FcPatternAddInteger(pattern, FC_WEIGHT, FC_WEIGHT_NORMAL);
	}

	// Pattern substitutions
	FcConfigSubstitute(0, pattern, FcMatchPattern);
	FcDefaultSubstitute(pattern);

	// Find pattern matching a font
	p = FcFontMatch(0, pattern, &result);
	if(!p) return NULL;

	// Get name and foundry
	if (FcPatternGetString(p, FC_FAMILY, 0, &fam) == FcResultMatch) {
		f->actualName = strdup((const char*)fam);
		if (FcPatternGetString(p, FC_FOUNDRY, 0, &fdy) == FcResultMatch) {
			free(f->actualName);
			asprintf(&f->actualName, "%s [%s]", (const char*)fam, (const char*)fdy);
		}
	}
//	printf("actualName: %s\n", f->actualName);

	/* Open font */
	font = XftFontOpenPattern((Display *)f->app->display, p);
	f->xid = (DKID)font;

	/* Destroy pattern */
	FcPatternDestroy(pattern);

	return font;
}
#else

#endif

void dkFontCreate(struct dkFont *f)
{
  if (f->xid)
    return;

#if defined(WIN32)              ///// WIN32 /////
	f->font = dkFontMatch(f, f->wantedName, NULL, f->wantedSize, f->wantedWeight, 100);
#elif defined(HAVE_XFT_H)       ///// XFT /////
	f->font = dkFontMatch(f, f->wantedName, NULL, f->wantedSize, f->wantedWeight, 100);
#else                           ///// XLFD /////
#endif

  if (!f->xid)
    printf("Unable to create font!\n");
}

/* Is it a mono space font */
int dkFontIsFontMono(struct dkFont *f)
{
  if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
    return !(((TEXTMETRIC*)f->font)->tmPitchAndFamily & TMPF_FIXED_PITCH);
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo i_extents, m_extents;
    XftTextExtents8((Display *)f->app->display, (XftFont*)f->font, (const FcChar8*)"i", 1, &i_extents);
    /* FIXME better than before but no cigar yet */
    XftTextExtents8((Display *)f->app->display, (XftFont*)f->font, (const FcChar8*)"M", 1, &m_extents);
    return i_extents.xOff == m_extents.xOff;
#else                           ///// XLFD /////
    return ((XFontStruct*)f->font)->min_bounds.width == ((XFontStruct*)f->font)->max_bounds.width;
#endif
  }
  return 1;
}

/* Get font width */
int dkFontGetFontWidth(struct dkFont *f)
{
  if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)f->font)->tmMaxCharWidth;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)f->font)->max_advance_width;
#else                           ///// XLFD /////
    return ((XFontStruct*)f->font)->max_bounds.width;
#endif
  }
  return 1;
}

/* Get font height */
int dkFontGetFontHeight(struct dkFont *f)
{
  if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
    return ((TEXTMETRIC*)f->font)->tmHeight;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    return ((XftFont*)f->font)->ascent + ((XftFont*)f->font)->descent;
#else                           ///// XLFD /////
    return ((XFontStruct *)f->font)->ascent + ((XFontStruct *)f->font)->descent;
#endif
  }
  return 1;
}

/* Calculate width of single wide character in this font */
int dkFontGetCharWidth(struct dkFont *f, DKwchar ch)
{
  if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
    DKnchar sbuffer[2];
    SIZE size;
    sbuffer[0] = ch;
    if (0xFFFF < ch) {          /* Deal with surrogate pair */
      sbuffer[0] = (ch >> 10) + LEAD_OFFSET;
      sbuffer[1] = (ch & 0x3FF) + 0xDC00;
      GetTextExtentPoint32W((HDC)f->dc, sbuffer, 2, &size);
      return size.cx;
    }
    GetTextExtentPoint32W((HDC)f->dc, sbuffer, 1, &size);
    return size.cx;
#elif defined(HAVE_XFT_H)       ///// XFT /////
    XGlyphInfo extents;
    XftTextExtents32(f->app->display, (XftFont *)f->font, (const FcChar32*)&ch, 1, &extents);
    return extents.xOff;
#else                           ///// XLFD /////
#endif
  }
  return 1;
}

/* Text width */
int dkFontGetTextWidth(struct dkFont *f, char *string, DKuint length)
{
	if (!string && length) {
		printf("%s: NULL string argument\n", __func__);
	}
	if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
		DKnchar sbuffer[4096];
		SIZE size;
		int count = dkString_utf2ncs(sbuffer, string, DTKMIN(length, 4096));
		GetTextExtentPoint32W((HDC)f->dc, sbuffer, count, &size);
		return size.cx;
#elif defined(HAVE_XFT_H)       ///// XFT /////
		XGlyphInfo extents;
		// This returns rotated metrics; FOX likes to work with unrotated metrics, so if angle
		// is not 0, we calculate the unrotated baseline; note however that the calculation is
		// not 100% pixel exact when the angle is not a multiple of 90 degrees.
		XftTextExtentsUtf8((Display *)f->app->display, (XftFont*)f->font, (const FcChar8*)string, length, &extents);
		if (f->angle) { return (int)(0.5 + sqrt(extents.xOff * extents.xOff + extents.yOff * extents.yOff)); }
		return extents.xOff;
#else                           ///// XLFD /////
#endif
	}
	return length;
}

/* Text height */
int dkFontGetTextHeight(struct dkFont *f, char *string, DKuint length)
{
	if (!string && length) {
		printf("%s: NULL string argument\n", __func__);
	}
	if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
		return ((TEXTMETRIC*)f->font)->tmHeight;
#elif defined(HAVE_XFT_H)       ///// XFT /////
		return ((XftFont*)f->font)->ascent + ((XftFont*)f->font)->descent;
#else                           ///// XLFD /////
#endif
	}
	return 1;
}

/* Get font ascent */
int dkFontGetFontAscent(struct dkFont *f)
{
	if (f->font) {
#if defined(WIN32)              ///// WIN32 /////
		return ((TEXTMETRIC*)f->font)->tmAscent;
#elif defined(HAVE_XFT_H)       ///// XFT /////
		return ((XftFont*)f->font)->ascent;
#else                           ///// XLFD /////
    return ((XFontStruct*)f->font)->ascent;
#endif
	}
	return 1;
}

void dkFontSetDesc(struct dkFont *f, struct dkFontDesc *fontdesc)
{
  free(f->wantedName);
  f->wantedName = strdup(fontdesc->face);
  f->wantedSize = fontdesc->size;
  f->wantedWeight = fontdesc->weight;
  f->wantedSlant = fontdesc->slant;
  f->wantedSetwidth = fontdesc->setwidth;
  f->wantedEncoding = fontdesc->encoding;
  f->hints = fontdesc->flags;
}
