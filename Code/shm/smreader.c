/*
 * smreader.c
 *
 * This process maps and reads from an area of physical memory
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/mman.h>
//This code uses the VGA video memory address: b8000

char *progname = "smreader";

int main(int argc, char *argv[]) {
	int mem_size, i, len;
	char *ptr;
	uint64_t mem_addr;
	if (argc != 3) {
		printf("Use: smreader mem_address(hex) mem_size\n");
		printf("Example: smreader b8000 28\n");
		exit(EXIT_FAILURE);
	}
	mem_addr = atoh(argv[1]);
	len = atoi(argv[2]);

	/* map in the physical memory */
	mem_size = sysconf(_SC_PAGE_SIZE);
	ptr = mmap_device_memory(0, mem_size, PROT_READ | PROT_WRITE, 0, mem_addr);
	if (ptr == MAP_FAILED) {
		printf("%s: mmap memory for address %llx failed: %s\n",
				progname, mem_addr, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* read from the shared memory */
	printf("%s: reading the text: ", progname);
	for (i = 0; i < len; i++)
		printf("%c", ptr[i]);
	printf("\n");
	munmap(ptr, mem_size);
}
