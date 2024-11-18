/*
 * smwriter.c
 *
 * This process maps and writes to an area of physical memory
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <sys/mman.h>
//This code uses the VGA video memory address: b8000

char *progname = "smwriter";

int main(int argc, char *argv[]) {
	char *text = "Hello Shared Memory Object!";
	int mem_size;
	char *ptr;
	uint64_t mem_addr;
	if (argc != 2) {
		printf("Use: smwriter mem_address (hex)\n");
		printf("Example: smwriter b8000\n");
		exit(EXIT_FAILURE);
	}
	mem_addr = atoh(argv[1]);

	/* map in the physical memory */
	mem_size = sysconf(_SC_PAGE_SIZE);
	ptr = mmap_device_memory(0, mem_size, PROT_READ | PROT_WRITE, 0, mem_addr);
	if (ptr == MAP_FAILED) {
		printf("%s: mmap memory for address %llx failed: %s\n",
				progname, mem_addr, strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* write to the shared memory */
	strcpy(ptr, text);
	printf("%s: Wrote to physical memory: %s\n", progname, ptr);
	printf("%s: Sleeping for 20 seconds. \n",progname);
	printf(	"%s: run smreader %s %d.\n", progname, argv[1], strlen(text) + 1);
	sleep(20);

	munmap(ptr, mem_size);
}
