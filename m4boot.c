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
#include <string.h>
#include "fdthelper.h"

#define SRC_BASE 0x4006e000
#define SRC_GPR2 0x28
#define SRC_GPR3 0x2c
#define CCM_BASE 0x4006b000
#define CCM_CCOWR 0x8c
#define CCM_CCOWR_START 0x00015a5a

#define USELOADER
#define ENTRY_POINT 0x0f000001UL
#define LOAD_ADDR 0x8f000000UL

#define LOAD_ADDR_DTB 0x8fff0000UL
#define ARG_ADDR_DTB 0x8fff0000UL

#define LOAD_ADDR_INITRD 0x8d000000UL

int copy_bin_to_pmem(unsigned int pdest, int mem_fd, char *data, int size);
char *malloc_load_bin(const char *file, int *size);
void save_bin(const char *file, char *buf, int size);
int run_cortexm4(int mem_fd);

extern char _binary_vf610m4bootldr_start;
extern const char  _binary_vf610m4bootldr_size[];
#define vf610m4bootldr_size ((int) (intptr_t) _binary_vf610m4bootldr_size)

int main(int argc, char *argv[])
{
	int fd, err;
	int size_image, size_initrd, size_fdt;
	unsigned long int image_addr = LOAD_ADDR;
	char *image;
	char *initrd = NULL;
	char *fdt = NULL;
	
	if (argc < 2) {
		fprintf(stderr, "Usage: m4boot IMAGE [INITRD] [DTB] [BOOTARGS]\n");
		return 1;
	}

	image = malloc_load_bin(argv[1], &size_image);
	if (!image)
		goto err_image;

	if (argc > 2)
	{
		if (strcmp(argv[2], "-")) {
			initrd = malloc_load_bin(argv[2], &size_initrd);
			if (!initrd)
				goto err_initrd;
		}

		fdt = malloc_load_bin(argv[3], &size_fdt);
		if (!fdt)
			goto err_fdt;

		if (initrd)
			patch_chosen_initrd(fdt, LOAD_ADDR_INITRD,
					    LOAD_ADDR_INITRD + size_initrd);

		if (argc > 4)
			patch_chosen_bootargs(fdt, argv[4]);

		size_fdt = fdt_newsize(fdt);
	}

	fd = open("/dev/mem", O_RDWR|O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Opening /dev/mem failed\n");
		goto err_devmem;
	}

	/* Loading kernel image */
#ifdef USELOADER
	copy_bin_to_pmem(LOAD_ADDR, fd, &_binary_vf610m4bootldr_start,
			 vf610m4bootldr_size);
	image_addr = LOAD_ADDR + vf610m4bootldr_size;

	printf("vf610m4bootldr: %d bytes copied to 0x%08lx through 0x%08lx\n",
	       vf610m4bootldr_size, LOAD_ADDR, LOAD_ADDR + vf610m4bootldr_size);
#endif /* USELOADER */

	err = copy_bin_to_pmem(image_addr, fd, image, size_image);
	if (err < 0)
		goto err_close;
	printf("%s: %d bytes copied to 0x%08lx through 0x%08lx\n", argv[1],
		size_image, image_addr, image_addr + size_image);

	if (initrd) {
		err = copy_bin_to_pmem(LOAD_ADDR_INITRD, fd, initrd, size_initrd);
		if (err < 0)
			goto err_close;
		printf("%s: %d bytes copied to 0x%08lx through 0x%08lx\n",
			argv[2], size_initrd, LOAD_ADDR_INITRD,
			LOAD_ADDR_INITRD + size_initrd);
	}

	if (argc > 3)
	{
		err = copy_bin_to_pmem(LOAD_ADDR_DTB, fd, fdt, size_fdt);
		if (err < 0)
			goto err_close;
		printf("%s: %d bytes copied to 0x%08lx through 0x%08lx\n",
			argv[3], size_fdt, LOAD_ADDR_DTB,
			LOAD_ADDR_DTB + size_fdt);


	}

	err = run_cortexm4(fd);

err_close:
	close(fd);

err_devmem:
	if (fdt)
		free(fdt);
err_fdt:
	if (initrd)
		free(initrd);
err_initrd:
	free(image);
err_image:

	return 0;
}

void save_bin(const char *file, char *buf, int size)
{
	int fp;
	fp = open(file, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);

	write(fp, buf, size);

	close(fp);
}

char *malloc_load_bin(const char *file, int *size)
{
	int fp;
	int result, loaded = 0;
	char *buf;

	fp = open(file, O_RDONLY, 0);
	if (!fp) {
		fprintf(stderr, "Unable to open file %s\n", file);
		return NULL;
	}

	*size = lseek(fp, 0L, SEEK_END);
	lseek(fp, 0L, SEEK_SET);

	buf = malloc(*size);
	do {
		result = read(fp, (void *)(buf + loaded), 4096);
		loaded += result;
	} while (result > 0);

	if (result < 0) {
		fprintf(stderr, "Failed to load file %s\n", file);
		goto free;
	}

	close(fp);

	fprintf(stderr, "%s: %d bytes loaded\n", file, loaded);

	return buf;
free:
	free(buf);

	return NULL;
}

int copy_bin_to_pmem(unsigned int pdest, int mem_fd, char *data, int size)
{
	unsigned char *mem;
	long pagesize = sysconf(_SC_PAGE_SIZE);
	unsigned int pdestoff = pdest & (pagesize - 1);
	unsigned int pdestaligned = pdest & ~(pagesize - 1);

	mem = (unsigned char *) mmap(0, size + pdestoff, PROT_READ|PROT_WRITE,
			MAP_SHARED, mem_fd, pdestaligned);

	if (mem == MAP_FAILED) {
		fprintf(stderr, "Mapping of 0x%08x failed\n", pdestaligned);
		return -2;
	}

	memcpy((void *)(mem + pdestoff), (void *)data, size);

	munmap(mem, size);

	return size;
}

int run_cortexm4(int mem_fd)
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

