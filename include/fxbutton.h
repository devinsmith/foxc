/******************************************************************************
 *                                                                            *
 *                        B u t t o n   W i d g e t                           *
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
 * $Id: FXButton.h,v 1.38 2006/01/22 17:57:59 fox Exp $                       *
 *****************************************************************************/

#ifndef FX_BUTTON_H
#define FX_BUTTON_H

#include "fxapp.h"
#include "fxdefs.h"

#include "fxlabel.h"

/* Button state bits */
enum {
  STATE_UP        = 0,		  /// Button is up
  STATE_DOWN      = 1,		  /// Button is down
  STATE_ENGAGED   = 2,		  /// Button is engaged
  STATE_UNCHECKED = STATE_UP,	  /// Same as STATE_UP (used for check buttons or radio buttons)
  STATE_CHECKED   = STATE_ENGAGED /// Same as STATE_ENGAGED (used for check buttons or radio buttons)
};


/* Button flags */
enum {
  BUTTON_AUTOGRAY  = 0x00800000,  /// Automatically gray out when not updated
  BUTTON_AUTOHIDE  = 0x01000000,  /// Automatically hide button when not updated
  BUTTON_TOOLBAR   = 0x02000000,  /// Toolbar style button [flat look]
  BUTTON_DEFAULT   = 0x04000000,  /// May become default button when receiving focus
  BUTTON_INITIAL   = 0x08000000,  /// This button is the initial default button
  BUTTON_NORMAL    = (FRAME_RAISED|FRAME_THICK|JUSTIFY_NORMAL|ICON_BEFORE_TEXT)
};

/**
* A button provides a push button, with optional icon and/or text label.
* When pressed, the button widget sends a SEL_COMMAND to its target.
* Passing the BUTTON_TOOLBAR style option gives buttons a "flat" look, and
* causes the edge of the button to be raised when the cursor moves over it.
* Passing BUTTON_DEFAULT allows the button to become the default button in
* a dialog, when the focus moves to it.  The default widget in a dialog
* is the widget which will accept the RETURN key when it is pressed.
* The BUTTON_INITIAL flag makes the button the default widget when the
* focus moves to a widget which can not itself be a default widget.
* There should be only a single button in the dialog which is the
* initial default; typically this is the OK or CLOSE button.
* The option BUTTON_AUTOGRAY (BUTTON_AUTOHIDE) causes the button to be grayed
* out (hidden) if its handler does not respond to the SEL_UPDATE message.
* This is useful when messages are delegated, for example when using a
* multiple document interface, where the ultimaye destination of a message
* can be changed.
*/

/* Forward declarations */
struct dtkEvent;
struct dkDrawable;

/* Inherits from dkLabel */
struct dkButton {
  struct dkLabel base;
  DKuchar state;
};

/* Constructor for a dkButton */
struct dkButton * dkButtonNew(struct dkWindow *parent, char *text);
void dkButtonInit(struct dkButton *pthis, struct dkWindow *p, char *text, DKuint opts);
void dkButtonSetState(struct dkButton *b, DKuint s);

#endif /* FX_BUTTON_H */