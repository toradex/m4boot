/*
 * m4boot - load and execute binary on secondary Cortex-M4 core on
 * Vybrid VF6xx SoCs
 * Copyright (C) 2014 Stefan Agner, Toradex AG
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

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SRC_BASE 0x4006e000
#define SRC_GPR2 0x28
#define SRC_GPR3 0x2c
#define CCM_BASE 0x4006b000
#define CCM_CCOWR 0x8c
#define CCM_CCOWR_START 0x00015a5a

#define ENTRY_POINT 0x8f000001UL
#define LOAD_ADDR 0x8f000000UL

#define LOAD_ADDR_DTB 0x8fff0000UL
#define ARG_ADDR_DTB 0x8fff0000UL

int load_bin(const char *file, unsigned int dest, int mem_fd);
int run_m4(int mem_fd);

int main(int argc, char *argv[])
{
	int fd, err;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: m4boot [file]\n");
		return 1;
	}

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Opening /dev/mem failed\n");
		return 1;
	}

	err = load_bin(argv[1], LOAD_ADDR, fd);
	if (err)
		goto err_close;

	err = load_bin(argv[2], LOAD_ADDR_DTB, fd);
	if (err)
		goto err_close;

	err = run_m4(fd);

err_close:
	close(fd);

	return 0;
}

int load_bin(const char *file, unsigned int dest, int mem_fd)
{
	int fp;
	size_t result;
	int size, loaded = 0;
	unsigned char *mem;

	fp = open(file, O_RDONLY, 0);
	if (!fp) {
		fprintf(stderr, "Unable to open file %s\n", file);
		return -1;
	}

	size = lseek(fp, 0L, SEEK_END);
	lseek(fp, 0L, SEEK_SET);
	fprintf(stderr, "Loading binary file %s, %d bytes\n", file, size);

	mem = (unsigned char *) mmap(0, size, PROT_READ|PROT_WRITE,
			MAP_SHARED, mem_fd, dest);

	if (mem == MAP_FAILED) {
		fprintf(stderr, "Mapping of 0x%08x failed\n", dest);
		return 1;
	}

	do {
		result = read(fp, (void *)(mem + loaded), 4096);
		loaded += result;
	} while (result > 0);

	if (result < 0) {
		fprintf(stderr, "Failed to load file %s\n", file);
		return -1;
	}

	fprintf(stderr, "%s: %d bytes loaded to 0x%08x through 0x%08x\n",
			file, loaded, dest, dest + loaded);

	close(fp);
	munmap(mem, size);

	return 0;
}

int run_m4(int mem_fd)
{
	unsigned char *src_mem;
	unsigned char *ccm_mem;
	unsigned int *src_reg;
	unsigned int *ccm_reg;

	/* Map System Reset Controller */
	src_mem = (unsigned char *) mmap(0, 0x1000, PROT_READ|PROT_WRITE,
			MAP_SHARED, mem_fd, SRC_BASE);

	if (src_mem == MAP_FAILED) {
		fprintf(stderr, "Mapping of %x failed\n", SRC_BASE);
		return 1;
	}

	/* Map Clock Controller Module */
	ccm_mem = (unsigned char *) mmap(0, 0x1000, PROT_READ|PROT_WRITE,
			MAP_SHARED, mem_fd, CCM_BASE);

	if (ccm_mem == MAP_FAILED) {
		fprintf(stderr, "Mapping of %x failed\n", CCM_BASE);
		return 1;
	}

	src_reg = (unsigned int *)(src_mem + SRC_GPR2);
	*src_reg = ENTRY_POINT;
	printf("Entry point set to 0x%08x\n", *src_reg);
	src_reg = (unsigned int *)(src_mem + SRC_GPR3);
	*src_reg = ARG_ADDR_DTB;
	printf("Argument set to 0x%08x\n", *src_reg);
	fflush(stdout);

	ccm_reg = (unsigned int *)(ccm_mem + CCM_CCOWR);
	*ccm_reg = CCM_CCOWR_START;
	printf("Cortex-M4 started...\n");

	munmap(src_mem, 0x1000);
	munmap(ccm_mem, 0x1000);

	return 0;
}

