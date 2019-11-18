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
 * $Id: FXVisual.h,v 1.41 2006/01/22 17:58:12 fox Exp $                       *
 *****************************************************************************/

#ifndef FXVISUAL_H
#define FXVISUAL_H

#include "fxdefs.h"

/* Visual construction flags */
#define VISUAL_DEFAULT      0     /* Default visual */
#define VISUAL_MONOCHROME   1     /* Must be monochrome visual */
#define VISUAL_BEST         2     /* Best (deepest) visual */
#define VISUAL_INDEXCOLOR   4     /* Palette visual */
#define VISUAL_GRAYSCALE    8     /* Gray scale visual */
#define VISUAL_TRUECOLOR    16    /* Must be true color visual */
#define VISUAL_OWNCOLORMAP  32    /* Allocate private colormap */
#define VISUAL_DOUBLEBUFFER 64    /* Double-buffered [FXGLVisual] */
#define VISUAL_STEREO       128   /* Stereo [FXGLVisual] */
#define VISUAL_NOACCEL      256   /* No hardware acceleration [for broken h/w] */
#define VISUAL_SWAP_COPY    512   /* Buffer swap by copying [FXGLVisual] */

/* Visual type */
#define VISUALTYPE_UNKNOWN 0      /* Undetermined visual type */
#define VISUALTYPE_MONO    1      /* Visual for drawing into 1-bpp surfaces */
#define VISUALTYPE_TRUE    2      /* True color */
#define VISUALTYPE_INDEX   3      /* Index [palette] color */
#define VISUALTYPE_GRAY    4      /* Gray scale */

typedef struct FXVisual FXVisual;

struct FXVisual {
	struct dkApp *app;    /* Back link to application object */
	DKID xid;

	unsigned int        numred;                 /* Number of reds */
	unsigned int        numgreen;               /* Number of greens */
	unsigned int        numblue;                /* Number of blues */
	unsigned int        numcolors;              /* Total number of colors */
	unsigned int        maxcolors;              /* Maximum number of colors */

	unsigned long rpix[16][256];          /* Mapping from red -> pixel */
	unsigned long gpix[16][256];          /* Mapping from green -> pixel */
	unsigned long bpix[16][256];          /* Mapping from blue -> pixel */
	void         *gc;                     /* Drawing GC */
	void         *scrollgc;               /* Scrolling GC */


	int type;
  DKID colormap;
	int freemap;
	int flags;
	unsigned int depth;
	void *visual;
};

#endif /* FXVISUAL_H */
