/******************************************************************************
 *                                                                            *
 *                    H a s h   T a b l e   C l a s s                         *
 *                                                                            *
 ******************************************************************************
 * Copyright (C) 2003,2006 by Jeroen van der Zijp.   All Rights Reserved.     *
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
 * $Id: FXHash.cpp,v 1.25 2006/01/22 17:58:31 fox Exp $                       *
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fxapp.h"

/*
  Notes:
  - The members used and free keep track of the number of slots
    in the table which are used and which are free.
  - When an item is inserted, used is incremented if the item isn't in the table
    yet, and free is decremented if a free slot is used; if an empty slot is
    used, free stays the same.  If the table exceeds the load factor, its
    size is doubled.
  - When an item is removed, used is decremented but free stays the same
    because the slot remains marked as empty instead of free; when the
    number of used items drops below some minimum, the table's size is
    halved.
  - If the table is resized, the empty slots all become free slots since
    the empty holes are not copied into the table; only used items will
    be rehashed into the new table.
*/

#define HASH1(x,m) (((unsigned int)((Duval)(x)^(((Duval)(x))>>13)))&((m)-1))
#define HASH2(x,m) (((unsigned int)((Duval)(x)^((((Duval)(x))>>17)|1)))&((m)-1))


struct dtkEntry {
	void *key;
	void *value;
};

/* Make empty table */
struct dtkHash *
DtkHashNew(void)
{
	struct dtkHash *h;

	h = fx_alloc(sizeof(struct dtkHash));
	h->table = malloc(sizeof(struct dtkEntry) * 2);
	h->table[0].key = NULL;
	h->table[0].value = NULL;
	h->table[1].key = NULL;
	h->table[1].value = NULL;
	h->total = 2;
	h->used = 0;
	h->free = 2;

	return h;
}

/* Destroy table */
void
DtkHashFree(struct dtkHash *h)
{
	free(h->table);
	free(h);
}

/* Resize hash table, and rehash old stuff into it */
void
DtkHashResize(struct dtkHash *h, unsigned int m)
{
	void *key, *value;
	unsigned int q, x, i;
	struct dtkEntry *newtable;
	newtable = calloc(sizeof(struct dtkEntry), m);
	for (i = 0; i < h->total; i++) {
		key = h->table[i].key;
		value = h->table[i].value;
		if (key == NULL || key == (void*)-1L) continue;
		q = HASH1(key, m);
		x = HASH2(key, m);
		while (newtable[q].key)
			q = (q + x) & (m-1);
		newtable[q].key = key;
		newtable[q].value = value;
	}
	free(h->table);
	h->table = newtable;
	h->total = m;
	h->free = m - h->used;
}

/* Insert key into the table */
void *
DtkHashInsert(struct dtkHash *h, void *key, void *value)
{
	unsigned int p, q, x;

	if (key) {
		if ((h->free << 1) <= h->total)
			DtkHashResize(h, h->total << 1);
		p = HASH1(key, h->total);
		x = HASH2(key, h->total);
		q = p;
		while (h->table[q].key) {
			if (h->table[q].key == key) goto y;             // Return existing
			q = (q + x) & (h->total - 1);
		}
		q = p;
		while (h->table[q].key) {
			if (h->table[q].key == (void*)-1L) goto x;      // Put it in empty slot
			q = (q + x) & (h->total - 1);
		}
		h->free--;
x:
		h->used++;
		h->table[q].key = key;
		h->table[q].value = value;
y:
		return h->table[q].value;
	}
	return NULL;
}

/* Replace key in the table */
void *
DtkHashReplace(struct dtkHash *h, void *key, void *value)
{
	unsigned int p, q, x;

	if (key) {
		if ((h->free << 1) <= h->total)
			DtkHashResize(h, h->total << 1);
		p = HASH1(key, h->total);
		x = HASH2(key, h->total);
		q = p;
		while (h->table[q].key) {
			if (h->table[q].key == key) goto y;             // Replace existing
			q = (q + x) & (h->total - 1);
		}
		q = p;
		while (h->table[q].key) {
			if (h->table[q].key == (void*)-1L) goto x;      // Put it in empty slot
			q = (q + x) & (h->total - 1);
		}
		h->free--;
x:
		h->used++;
		h->table[q].key = key;
y:
		h->table[q].value = value;
		return h->table[q].value;
	}
	return NULL;
}

/* Remove association from the table */
void *
DtkHashRemove(struct dtkHash *h, void* key)
{
	unsigned int q, x;
	void *val;

	if (key) {
		q = HASH1(key, h->total);
		x = HASH2(key, h->total);
		while (h->table[q].key != key) {
			if (h->table[q].key == NULL)
				return NULL;
			q = (q + x) & (h->total - 1);
		}
		val = h->table[q].value;
		h->table[q].key=(void*)-1L;                    // Empty but not free
		h->table[q].value=NULL;
		h->used--;
		if (h->used < (h->total >> 2))
			DtkHashResize(h, h->total >> 1);
		return val;
	}
	return NULL;
}

/* Return true if association in table */
void *
DtkHashFind(struct dtkHash *h, void *key)
{
	unsigned int q, x;

	if (key) {
		q = HASH1(key, h->total);
		x = HASH2(key, h->total);
		while (h->table[q].key != key) {
			if (h->table[q].key == NULL)
				return NULL;
		q = (q + x) & (h->total - 1);
		}
		return h->table[q].value;
	}
	return NULL;
}

/* Clear hash table */
void
DtkHashClear(struct dtkHash *h)
{
	fx_resize((void**)&h->table, sizeof(struct dtkEntry) * 2);
	h->table[0].key = NULL;
	h->table[0].value = NULL;
	h->table[1].key = NULL;
	h->table[1].value = NULL;
	h->total = 2;
	h->used = 0;
	h->free = 2;
}

