/*
 * Copyright (C) 2014 Stefan Agner
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

#ifndef __FDTHELPER_H__
#define __FDTHELPER_H__

int fdt_newsize(char *fdt);
int patch_chosen_initrd(char *fdt, unsigned int initrd_start,
			unsigned int initrd_end);
int patch_chosen_bootargs(char *fdt, char *args);

#endif /* __FDTHELPER_H__ */
