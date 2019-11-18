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
 * $Id: FXObject.cpp,v 1.42 2006/01/22 17:58:36 fox Exp $                     *
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "fxobject.h"

struct dkMapEntry *
DKMetaClassSearch(struct dkMetaClass *mc, DKSelector key)
{
	struct dkMapEntry *lst = mc->assoc;
	int n = mc->nassocs;
	while (n--) {
		if (lst->keylo <= key && key <= lst->keyhi) return lst;
		lst = (struct dkMapEntry *) (((char*)lst) + mc->assocsz);
	}
	return NULL;
}

/* Unhandled function */
long
dkObject_onDefault(void *pthis, struct dkObject *obj, DKSelector sel, void *data)
{
  printf("unhandled message\n");
	return 0;
}

/* Handle message */
long
dkObject_handle(void *pthis, struct dkObject *obj, DKSelector selhi, DKSelector sello, void *data)
{
	return dkObject_onDefault(pthis, obj, DKSEL(selhi, sello), data);
}

/* dkObject constructor */
void
dkObjectInit(struct dkObject *obj)
{
	obj->handle = dkObject_handle;
}

void *
fx_alloc(size_t bytes)
{
	void *ret;

	ret = malloc(bytes);
	if (ret == NULL) {
		printf("memory allocation failed\n");
		exit(1);
	}
	return ret;
}

int
fx_resize(void **ptr, unsigned long size)
{
	void *p = NULL;
	if (size != 0) {
		if ((p = realloc(*ptr, size)) == NULL)
			return 0;
	} else {
		if(*ptr)
			free(*ptr);
	}
	*ptr=p;
	return 1;
}
