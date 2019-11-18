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
 * $Id: FXFont.h,v 1.66 2006/01/22 17:58:02 fox Exp $                         *
 *****************************************************************************/

#ifndef FX_FONT_H
#define FX_FONT_H

#include "fxdefs.h"

/// Font character set encoding
enum FXFontEncoding {
  FONTENCODING_DEFAULT,         /// Don't care character encoding

  FONTENCODING_ISO_8859_1   = 1,        /// West European (Latin1)
  FONTENCODING_ISO_8859_2   = 2,        /// Central and East European (Latin2)
  FONTENCODING_ISO_8859_3   = 3,        /// Esperanto (Latin3)
  FONTENCODING_ISO_8859_4   = 4,
  FONTENCODING_ISO_8859_5   = 5,        /// Cyrillic (almost obsolete)
  FONTENCODING_ISO_8859_6   = 6,        /// Arabic
  FONTENCODING_ISO_8859_7   = 7,        /// Greek
  FONTENCODING_ISO_8859_8   = 8,        /// Hebrew
  FONTENCODING_ISO_8859_9   = 9,        /// Turkish (Latin5)
  FONTENCODING_ISO_8859_10  = 10,
  FONTENCODING_ISO_8859_11  = 11,       /// Thai
  FONTENCODING_ISO_8859_13  = 13,       /// Baltic
  FONTENCODING_ISO_8859_14  = 14,
  FONTENCODING_ISO_8859_15  = 15,
  FONTENCODING_ISO_8859_16  = 16,
  FONTENCODING_KOI8         = 17,
  FONTENCODING_KOI8_R       = 18,       /// Russian
  FONTENCODING_KOI8_U       = 19,       /// Ukrainian
  FONTENCODING_KOI8_UNIFIED = 20,

  FONTENCODING_CP437        = 437,      /// IBM-PC code page
  FONTENCODING_CP850        = 850,      /// IBMPC Multilingual
  FONTENCODING_CP851        = 851,      /// IBM-PC Greek
  FONTENCODING_CP852        = 852,      /// IBM-PC Latin2
  FONTENCODING_CP855        = 855,      /// IBM-PC Cyrillic
  FONTENCODING_CP856        = 856,      /// IBM-PC Hebrew
  FONTENCODING_CP857        = 857,      /// IBM-PC Turkish
  FONTENCODING_CP860        = 860,      /// IBM-PC Portugese
  FONTENCODING_CP861        = 861,      /// IBM-PC Iceland
  FONTENCODING_CP862        = 862,      /// IBM-PC Israel
  FONTENCODING_CP863        = 863,      /// IBM-PC Canadian/French
  FONTENCODING_CP864        = 864,      /// IBM-PC Arabic
  FONTENCODING_CP865        = 865,      /// IBM-PC Nordic
  FONTENCODING_CP866        = 866,      /// IBM-PC Cyrillic #2
  FONTENCODING_CP869        = 869,      /// IBM-PC Greek #2
  FONTENCODING_CP870        = 870,      /// Latin-2 Multilingual

  FONTENCODING_CP1250       = 1250,     /// Windows Central European
  FONTENCODING_CP1251       = 1251,     /// Windows Russian
  FONTENCODING_CP1252       = 1252,     /// Windows Latin1
  FONTENCODING_CP1253       = 1253,     /// Windows Greek
  FONTENCODING_CP1254       = 1254,     /// Windows Turkish
  FONTENCODING_CP1255       = 1255,     /// Windows Hebrew
  FONTENCODING_CP1256       = 1256,     /// Windows Arabic
  FONTENCODING_CP1257       = 1257,     /// Windows Baltic
  FONTENCODING_CP1258       = 1258,     /// Windows Vietnam
  FONTENCODING_CP874        = 874,      /// Windows Thai

  FONTENCODING_UNICODE      = 9999,

  FONTENCODING_LATIN1       = FONTENCODING_ISO_8859_1,   /// Latin 1 (West European)
  FONTENCODING_LATIN2       = FONTENCODING_ISO_8859_2,   /// Latin 2 (East European)
  FONTENCODING_LATIN3       = FONTENCODING_ISO_8859_3,   /// Latin 3 (South European)
  FONTENCODING_LATIN4       = FONTENCODING_ISO_8859_4,   /// Latin 4 (North European)
  FONTENCODING_LATIN5       = FONTENCODING_ISO_8859_9,   /// Latin 5 (Turkish)
  FONTENCODING_LATIN6       = FONTENCODING_ISO_8859_10,  /// Latin 6 (Nordic)
  FONTENCODING_LATIN7       = FONTENCODING_ISO_8859_13,  /// Latin 7 (Baltic Rim)
  FONTENCODING_LATIN8       = FONTENCODING_ISO_8859_14,  /// Latin 8 (Celtic)
  FONTENCODING_LATIN9       = FONTENCODING_ISO_8859_15,  /// Latin 9 AKA Latin 0
  FONTENCODING_LATIN10      = FONTENCODING_ISO_8859_16,  /// Latin 10

  FONTENCODING_USASCII      = FONTENCODING_ISO_8859_1,   /// Latin 1
  FONTENCODING_WESTEUROPE   = FONTENCODING_ISO_8859_1,   /// Latin 1 (West European)
  FONTENCODING_EASTEUROPE   = FONTENCODING_ISO_8859_2,   /// Latin 2 (East European)
  FONTENCODING_SOUTHEUROPE  = FONTENCODING_ISO_8859_3,   /// Latin 3 (South European)
  FONTENCODING_NORTHEUROPE  = FONTENCODING_ISO_8859_4,   /// Latin 4 (North European)
  FONTENCODING_CYRILLIC     = FONTENCODING_ISO_8859_5,   /// Cyrillic
  FONTENCODING_RUSSIAN      = FONTENCODING_KOI8,         /// Cyrillic
  FONTENCODING_ARABIC       = FONTENCODING_ISO_8859_6,   /// Arabic
  FONTENCODING_GREEK        = FONTENCODING_ISO_8859_7,   /// Greek
  FONTENCODING_HEBREW       = FONTENCODING_ISO_8859_8,   /// Hebrew
  FONTENCODING_TURKISH      = FONTENCODING_ISO_8859_9,   /// Latin 5 (Turkish)
  FONTENCODING_NORDIC       = FONTENCODING_ISO_8859_10,  /// Latin 6 (Nordic)
  FONTENCODING_THAI         = FONTENCODING_ISO_8859_11,  /// Thai
  FONTENCODING_BALTIC       = FONTENCODING_ISO_8859_13,  /// Latin 7 (Baltic Rim)
  FONTENCODING_CELTIC       = FONTENCODING_ISO_8859_14   /// Latin 8 (Celtic)
};

/// Font style hints
enum {
    dkFont_Decorative     = 4,         /// Fancy fonts
    dkFont_Modern         = 8,         /// Monospace typewriter font
    dkFont_Roman          = 16,        /// Variable width times-like font, serif
    dkFont_Script         = 32,        /// Script or cursive
    dkFont_Swiss          = 64,        /// Helvetica/swiss type font, sans-serif
    dkFont_System         = 128,       /// System font
    dkFont_X11            = 256,       /// Raw X11 font string
    dkFont_Scalable       = 512,       /// Scalable fonts
    dkFont_Polymorphic    = 1024,      /// Polymorphic fonts, e.g. parametric weight, slant, etc.
    dkFont_Rotatable      = 2048       /// Rotatable fonts
};

/* Font slant options */
enum {
  dkFont_ReverseOblique = 1,           /* Reversed oblique */
  dkFont_ReverseItalic  = 2,           /* Reversed italic */
  dkFont_Straight       = 5,           /* Straight, not slanted */
  dkFont_Italic         = 8,           /* Italics */
  dkFont_Oblique        = 9            /* Oblique slant */
};

/* Font style */
struct dkFontDesc {
  char       face[116];    /* Face name */
  DKushort   size;         /* Size in deci-points */
  DKushort   weight;       /* Weight [light, normal, bold, ...] */
  DKushort   slant;        /* Slant [normal, italic, oblique, ...] */
  DKushort   setwidth;     /* Set width [normal, condensed, expanded, ...] */
  DKushort   encoding;     /* Encoding of character set */
  DKushort   flags;        /* Flags */
};

struct dkFont {
	struct dkApp *app;    /* Back link to application object */
	DKID xid;

	/* dkFont members */
	char *wantedName;             // Desired font font name
	char *actualName;             // Matched font font name
	DKushort  wantedSize;         // Font size (points*10)
	DKushort  actualSize;         // Actual size that was matched
	DKushort  wantedWeight;       // Font weight
	DKushort  actualWeight;       // Font weight
	DKushort  wantedSlant;        // Font slant
	DKushort  actualSlant;        // Font slant
	DKushort  wantedSetwidth;     // Relative setwidth
	DKushort  actualSetwidth;     // Relative setwidth
	DKushort  wantedEncoding;     // Character set encoding
	DKushort  actualEncoding;     // Character set encoding
	DKushort  hints;              // Matching hint flags
	DKushort  flags;              // Actual flags
	short   angle;                // Angle
	void     *font;               // Info about the font
#ifdef WIN32
	DKID     dc;
#endif
};

struct dkFont * dkFontNew(struct dkApp *app, char *name);
void dkFontCreate(struct dkFont *f);
int dkFontGetFontHeight(struct dkFont *f);
int dkFontGetFontWidth(struct dkFont *f);
int dkFontGetFontAscent(struct dkFont *f);
int dkFontGetTextWidth(struct dkFont *f, char *string, DKuint length);
int dkFontGetTextHeight(struct dkFont *f, char *string, DKuint length);
void dkFontSetDesc(struct dkFont *f, struct dkFontDesc *fontdesc);
int dkFontIsFontMono(struct dkFont *f);
int dkFontGetCharWidth(struct dkFont *f, DKwchar ch);

#endif /* FX_FONT_H */
