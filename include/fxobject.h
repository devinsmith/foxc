/******************************************************************************
 *                                                                            *
 *                      T o p l e v el   O b j e c t                          *
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
 * $Id: FXObject.h,v 1.35.2.1 2006/08/05 00:58:29 fox Exp $                   *
 *****************************************************************************/

#ifndef FX_OBJECT_H
#define FX_OBJECT_H

#include "fxdefs.h"

/* Association key */
typedef DKuint FXSelector;
typedef DKuint DKSelector;

struct dkObject;

struct dkMapEntry {
  DKuint keylo;
  DKuint keyhi;
  long (*func)(void *pthis, struct dkObject *obj, DKuint selhi, DKuint sello,
      void *data);
};

/* Describes a DTK Object */
struct dkMetaClass {
  char *className;
  void *assoc;
  unsigned int nassocs;
  unsigned int assocsz;
};

typedef long (*dkHandleProc)(void *pthis, struct dkObject *obj, FXSelector selhi, FXSelector sello, void *data);

struct dkObject {

  /* Functions */
  dkHandleProc handle;

  /* Members */
  struct dkMetaClass *meta;
};

/* dkMetaClass functions */
struct dkMapEntry *DKMetaClassSearch(struct dkMetaClass *mc, FXSelector key);


/* dkObject Functions */
void dkObjectInit(struct dkObject *obj);

long dkObject_handle(void *pthis, struct dkObject *obj, FXSelector selhi, FXSelector sello, void *data);

#endif /* FX_OBJECT_H */
