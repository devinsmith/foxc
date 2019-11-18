/******************************************************************************
 *                                                                            *
 *                         V i s u a l   C l a s s                            *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 1999,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: FXVisual.cpp,v 1.79.2.1 2007/05/15 05:23:43 fox Exp $                 *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#endif

#include "fxapp.h"
#include "fxdc.h"
#include "fxdrv.h"

FXVisual *
DtkNewVisual(App *app, unsigned int flags)
{
	FXVisual *v;

	v = calloc(1, sizeof(FXVisual));

  DKTRACE((100, "dkVisual::dkVisual %p\n", v));

	v->app = app;
	v->xid = 0;
	v->flags = flags;
	v->depth = 32;
	return v;
}

void
DtkCreateVisual(FXVisual *v)
{
	if (v->xid)
		return;
	if (!v->app->display_opened)
		return;

	DKTRACE((100, "dkVisual::create %p\n", v));
	DtkDrvCreateVisual(v);
}

#ifdef WIN32
/* WIN32 internal helper structures */
struct LOGPALETTE256 {
	WORD palVersion;
	WORD palNumEntries;
	PALETTEENTRY palPalEntry[257];
};

struct BITMAPINFO256 {
	BITMAPINFOHEADER bmiHeader;
	DWORD bmiColors[256];
};

/* Get number of bits in n */
static inline unsigned int
findnbits(DWORD n)
{
	unsigned int nb = 0;
	while (n) {
		nb += (n & 1);
		n >>= 1;
	}
	return nb;
}

/* Make palette */
static HPALETTE
createAllPurposePalette()
{
	struct LOGPALETTE256 palette;
	HPALETTE hPalette, hStockPalette;
	int num, r, g, b;

	/* We will use the stock palette */
	hStockPalette = (HPALETTE)GetStockObject(DEFAULT_PALETTE);

	/* Fill in first 20 entries from system color palette */
	num = GetPaletteEntries(hStockPalette, 0, 20, palette.palPalEntry);

	/* Calculate remaining 216 colors, 8 of which match the standard
	 * 20 colors and 4 of which match the gray shades above */
	for (r = 0; r < 256; r += 51) {
		for (g = 0; g < 256; g += 51) {
			for (b = 0; b < 256; b += 51) {
				palette.palPalEntry[num].peRed = r;
				palette.palPalEntry[num].peGreen = g;
				palette.palPalEntry[num].peBlue = b;
				palette.palPalEntry[num].peFlags = 0;
				num++;
			}
		}
	}

	/* Fill in the rest */
	palette.palVersion = 0x300;
	palette.palNumEntries = num;

	/* Create palette and we're done */
	hPalette = CreatePalette((const LOGPALETTE*)&palette);

	/* Return palette */
	return hPalette;
}

void
DtkDrvCreateVisual(FXVisual *v)
{
	HDC hdc;
	HBITMAP hbm;
	struct BITMAPINFO256 bmi;
	unsigned int redbits, greenbits, bluebits;
	unsigned int redmask, greenmask, bluemask;

	/* Check for palette support */
	hdc = GetDC(GetDesktopWindow());

	/* Check for palette mode; assume 8-bit for now */
	if (GetDeviceCaps(hdc, RASTERCAPS) & RC_PALETTE) {
		v->colormap = createAllPurposePalette();
		v->depth = 8;
		v->numred = 6;    /* We have a 6x6x6 ramp, at least... */
		v->numgreen = 6;
		v->numblue = 6;
		v->numcolors = 256;
		v->type = VISUALTYPE_INDEX;
		v->freemap = 1;
	}
	/* True color mode; find out how deep */
	else {
		memset(&bmi, 0, sizeof(struct BITMAPINFO256));
		bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		/* Get a device-dependent bitmap that's compatible with the
		 * screen, then convert the DDB to a DIB.  We need to call
		 * GetDIBits twice: the first call just fills in the
		 * BITMAPINFOHEADER; the second fills in the bitfields or
		 * palette */
		hbm = CreateCompatibleBitmap(hdc, 1, 1);
		GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS);
		GetDIBits(hdc, hbm, 0, 1, NULL, (LPBITMAPINFO)&bmi, DIB_RGB_COLORS);
		DeleteObject(hbm);
		if (bmi.bmiHeader.biCompression == BI_BITFIELDS) {
			redmask = bmi.bmiColors[0];
			greenmask = bmi.bmiColors[1];
			bluemask = bmi.bmiColors[2];

			redbits = findnbits(redmask);
			greenbits = findnbits(greenmask);
			bluebits = findnbits(bluemask);
			v->numred = 1 << redbits;
			v->numgreen = 1 << greenbits;
			v->numblue = 1 << bluebits;
			v->depth = redbits + greenbits + bluebits;
			v->numcolors = v->numred * v->numgreen * v->numblue;
			v->type = VISUALTYPE_TRUE;
		} else {
			v->type = VISUALTYPE_UNKNOWN;
		}
	}
	ReleaseDC(GetDesktopWindow(), hdc);

	/* This is just a placeholder */
	v->xid = (void*)1;
}

unsigned long
dtkDrvVisualGetPixel(FXVisual *v, unsigned int clr)
{
	return PALETTERGB(FXREDVAL(clr), FXGREENVAL(clr), FXBLUEVAL(clr));
}
#else

/* Standard dither kernel */
static const int dither[16] = {
	 0*16,  8*16,  2*16, 10*16,
	12*16,  4*16, 14*16,  6*16,
	 3*16, 11*16,  1*16,  9*16,
	15*16,  7*16, 13*16,  5*16,
};

/* Find shift amount */
static inline unsigned int
findshift(unsigned long mask)
{
	unsigned int sh = 0;
	while (!(mask&(1 << sh))) sh++;
	return sh;
}

/* Apply gamma correction to an intensity value in [0..max]. */
static unsigned int
gamma_adjust(double gamma, unsigned int value, unsigned int max)
{
	double x = (double)value / (double)max;
	return (unsigned int) (((double)max * pow(x,1.0/gamma))+0.5);
}

/* Setup for true color */
static void
setuptruecolor(FXVisual *v)
{
	unsigned int  redshift,greenshift,blueshift;
	unsigned long redmask,greenmask,bluemask;
	unsigned long redmax,greenmax,bluemax;
	unsigned int i,c,d,r,g,b;
	double gamma;

	/* Ideally we would read from the registry */
	gamma = 1.0;

	/* Arrangement of pixels */
	redmask = ((Visual*)v->visual)->red_mask;
	greenmask = ((Visual*)v->visual)->green_mask;
	bluemask = ((Visual*)v->visual)->blue_mask;
	redshift = findshift(redmask);
	greenshift = findshift(greenmask);
	blueshift = findshift(bluemask);
	redmax = redmask >> redshift;
	greenmax = greenmask >> greenshift;
	bluemax = bluemask>>blueshift;
	v->numred = redmax + 1;
	v->numgreen = greenmax + 1;
	v->numblue = bluemax + 1;
	v->numcolors = v->numred * v->numgreen * v->numblue;

	/* Make the dither tables */
	for (d = 0; d < 16; d++) {
		for(i=0; i<256; i++) {
			c = gamma_adjust(gamma, i, 255);
			r = (redmax * c + dither[d]) / 255;
			g = (greenmax * c + dither[d]) / 255;
			b = (bluemax * c + dither[d]) / 255;
			v->rpix[d][i] = r << redshift;
			v->gpix[d][i] = g << greenshift;
			v->bpix[d][i] = b << blueshift;
		}
	}

	/* Set type */
	v->type = VISUALTYPE_TRUE;
//	printf("Visual type is true color\n");
}

static void
setupdirectcolor(FXVisual *v)
{
	printf("direct\n");
}

static void
setuppseudocolor(FXVisual *v)
{
}

static void
setupstaticcolor(FXVisual *v)
{
}

static void
setupgrayscale(FXVisual *v)
{
}

static void
setupstaticgray(FXVisual *v)
{
}

/* Determine colormap, then initialize it */
static void
setupcolormap(FXVisual *v)
{
	Display *disp;

	disp = v->app->display;
	//XStandardColormap stdmap;
	if (v->flags & VISUAL_MONOCHROME) {
		v->colormap = None;
		//FXTRACE((150,"%s::create: need no colormap\n",getClassName()));
		//setuppixmapmono();
	} else {
		if ((v->flags & VISUAL_OWNCOLORMAP) || (v->visual != DefaultVisual(disp, DefaultScreen(disp)))) {
			v->colormap = XCreateColormap(disp, RootWindow(disp, DefaultScreen(disp)), ((Visual*)v->visual), AllocNone);

			//FXTRACE((150,"%s::create: allocate colormap\n",getClassName()));
			v->freemap = 1;
		} else {
			//getstdcolormap(DISPLAY(getApp()),((Visual*)visual)->visualid,stdmap);
			v->colormap = DefaultColormap(disp, DefaultScreen(disp));
			//FXTRACE((150,"%s::create: use default colormap\n",getClassName()));
		}
		switch (((Visual*)v->visual)->class) {
		case TrueColor:   setuptruecolor(v); break;
		case DirectColor: setupdirectcolor(v); break;
		case PseudoColor: setuppseudocolor(v); break;
		case StaticColor: setupstaticcolor(v); break;
		case GrayScale:   setupgrayscale(v); break;
		case StaticGray:  setupstaticgray(v); break;
		}
	}
}

static void *
setupgc(FXVisual *v, int gex)
{
	XGCValues gval;
	DKID drawable;
	GC gg;
	Display *d;

	gval.fill_style = FillSolid;
	gval.graphics_exposures = gex;
	d = v->app->display;

	/* For default visual; this is easy as we already have a matching window */
	if ((Visual*)v->visual == DefaultVisual(d, DefaultScreen(d) && v->depth == DefaultDepth(d, DefaultScreen(d)))) {
		gg = XCreateGC(d, XDefaultRootWindow(d), GCFillStyle|GCGraphicsExposures, &gval);
	}
	/* For arbitrary visual; create a temporary pixmap of the same depth as the visual */
	else {
		drawable = XCreatePixmap(d, XDefaultRootWindow(d), 1, 1, v->depth);
		gg = XCreateGC(d, drawable, GCFillStyle|GCGraphicsExposures, &gval);
		XFreePixmap(d, drawable);
	}
	return gg;
}

unsigned long
dtkDrvVisualGetPixel(FXVisual *v, unsigned int clr)
{
	switch (v->type) {
	case VISUALTYPE_TRUE:    return v->rpix[1][FXREDVAL(clr)] | v->gpix[1][FXGREENVAL(clr)] | v->bpix[1][FXBLUEVAL(clr)];
//	case VISUALTYPE_INDEX:   return lut[rpix[1][FXREDVAL(clr)]+gpix[1][FXGREENVAL(clr)]+bpix[1][FXBLUEVAL(clr)]];
//	case VISUALTYPE_GRAY:    return gpix[1][(77*FXREDVAL(clr)+151*FXGREENVAL(clr)+29*FXBLUEVAL(clr))>>8];
//	case VISUALTYPE_MONO:    return gpix[1][(77*FXREDVAL(clr)+151*FXGREENVAL(clr)+29*FXBLUEVAL(clr))>>8];
	case VISUALTYPE_UNKNOWN: return 0;
	}

	return 0;
}

void
DtkDrvCreateVisual(FXVisual *v)
{
	/*
	XVisualInfo vitemplate;
	XVisualInfo *vi;
	int nvi,i,d,dbest;
	*/
	Display *disp;

	disp = v->app->display;
	/* Assume the default */
	v->visual = DefaultVisual(disp, DefaultScreen(disp));
	v->depth = DefaultDepth(disp, DefaultScreen(disp));

	/* True color */
	if (v->flags & VISUAL_TRUECOLOR) {
		printf("flags = TrueColor\n");
	}

	/* Initialize colormap */
	setupcolormap(v);

	/* Make GC's for this visual */
	v->gc = setupgc(v, 0);
	v->scrollgc = setupgc(v, 1);

	v->xid = 1;

}
#endif

