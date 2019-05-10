#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>

#include "mmaping.h"
#define PAGE_SIZE sysconf(_SC_PAGESIZE)

static void *
mmap_malloc(int fd, size_t bytes)
{

    void * data;

    data = mmap(0, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        fprintf(stderr, "Could not mmap " DEV_NAME ": %s\n", strerror(errno));
        return NULL;
    }

    return data;

}

int main (int argc, char* argv[])
{
	unsigned i, row, col; 
	unsigned size, doubled_size;
	double *m1, *m2, *output;

	if (argc != 2) {
		printf("Usage: input size of matrix\n");
		exit(-1);
	}

	size = atoi(argv[1]);

    int fd = open(DEV_NAME, O_RDWR);//access read-write
    if (fd == -1) {
        fprintf(stderr, "Could not open " DEV_NAME ": %s\n", strerror(errno));
        return -1;
    }

	doubled_size = size * size;

    m1 = (double *)mmap_malloc(fd, sizeof(double) * doubled_size);
    m2 = (double *)mmap_malloc(fd, sizeof(double) * doubled_size);
    output = (double *)mmap_malloc(fd, sizeof(double) * doubled_size);

	for (row = 0; row < size; row++) {
		for (col = 0; col < size; col++) {
			for (i = 0; i < size; i++){
                output[row*size + col] += m1[row*size + i] *m2[i*size + col];
			}	
		}
	}

    printf("Successful multiplication\n");

    return 0;
}
