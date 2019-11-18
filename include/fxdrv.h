/*
 * Copyright (c) 2009 Devin Smith <devin@devinsmith.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef FX_DRV_H
#define FX_DRV_H

#include "fxapp.h"
#include "fxwindow.h"

/* drv app.c */
void DtkDrvAppNew(App *app);

int dtkDrvOpenDisplay(App *app, char *dpyname);
int dtkDrvRunOneEvent(App *app, int blocking);

/* drv visual.c */
void DtkDrvCreateVisual(FXVisual *v);
unsigned long dtkDrvVisualGetPixel(FXVisual *v, unsigned int clr);

/* drv window.c */
void DtkDrvCreateWindow(struct dkWindow *w);
void DtkDrvWindowShow(struct dkWindow *w);

#endif /* FX_DRV_H */
