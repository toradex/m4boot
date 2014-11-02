/*
 * Copyright (C) 2014 Stefan Agner
 *
 * Code taken from fdtput utility, part of
 * libfdt - Flat Device Tree manipulation
 *
 * Copyright (C) 2006 David Gibson, IBM Corporation.
 * Copyright 2011 The Chromium Authors, All Rights Reserved.
 * Copyright 2008 Jon Loeliger, Freescale Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <libfdt.h>

#define ALIGN(x)		(((x) + (FDT_TAGSIZE) - 1) & ~((FDT_TAGSIZE) - 1))

static inline void *xrealloc(void *p, int len)
{
	void *new = realloc(p, len);

	if (!new) {
		fprintf(stderr, "realloc() failed (len=%d)\n", len);
		exit(1);
	}

	return new;
}

static char *_realloc_fdt(char *fdt, int delta)
{
	int new_sz = fdt_totalsize(fdt) + delta;
	fdt = xrealloc(fdt, new_sz);
	fdt_open_into(fdt, fdt, new_sz);
	return fdt;
}

static char *realloc_property(char *fdt, int nodeoffset,
		const char *name, int newlen)
{
	int delta = 0;
	int oldlen = 0;

	if (!fdt_get_property(fdt, nodeoffset, name, &oldlen))
		/* strings + property header */
		delta = sizeof(struct fdt_property) + strlen(name) + 1;

	if (newlen > oldlen)
		/* actual value in off_struct */
		delta += ALIGN(newlen) - ALIGN(oldlen);

	return _realloc_fdt(fdt, delta);
}

static int store_key_value(char *fdt, const char *node_name,
		const char *property, const char *buf, int len)
{
	int node;
	int err;

	node = fdt_path_offset(fdt, node_name);
	if (node < 0) {
		fprintf(stderr, "Node %s not found\n", node_name);
		return -1;
	}

	err = fdt_setprop(fdt, node, property, buf, len);
	if (err == -FDT_ERR_NOSPACE) {
		fdt = realloc_property(fdt, node, property, len);
		err = fdt_setprop(fdt, node, property, buf, len);
	}
	if (err) {
		fprintf(stderr, "Set property %s failed\n", property);
		return -1;
	}
	return 0;
}

int fdt_newsize(char *fdt)
{
	return fdt_totalsize(fdt);
}
int patch_chosen_initrd(char *fdt, unsigned int initrd_start,
			unsigned int initrd_end)
{
	int err = 0;
	unsigned int tmp;

	tmp = cpu_to_fdt32(initrd_start);
	err = store_key_value(fdt, "/chosen", "linux,initrd-start",
			      (char *)&tmp, 4);
	if (err)
		return err;

	tmp = cpu_to_fdt32(initrd_end);
	err = store_key_value(fdt, "/chosen", "linux,initrd-end",
			      (char *)&tmp, 4);

	return err;
}

int patch_chosen_bootargs(char *fdt, char *args)
{
	return store_key_value(fdt, "/chosen", "bootargs", args,
			       strlen(args) + 1);
}
