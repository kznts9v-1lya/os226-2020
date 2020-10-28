#define _GNU_SOURCE

#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#include "vm.h"
#include "sched.h"

static int g_memfd = -1;
static unsigned g_memsize;

static off_t offset;
static struct vmem_region vmem_buf[16];
static int taskn;

int vmbrk(void *addr) {
	if (MAP_FAILED == mmap(USERSPACE_START,
			addr - USERSPACE_START,
			PROT_READ | PROT_WRITE,
			MAP_FIXED | MAP_SHARED,
			g_memfd, offset)) {
		perror("mmap g_memfd");
		return -1;
	}

	vmem_buf[taskn].start = offset;
	vmem_buf[taskn].end = offset + addr - USERSPACE_START - 1;
	vmem_set(&vmem_buf[taskn]);
	taskn++;

	int page_q = ((addr - USERSPACE_START) % VM_PAGESIZE == 0)? 
                (addr - USERSPACE_START) / VM_PAGESIZE :
                (addr - USERSPACE_START) / VM_PAGESIZE + 1;

	offset += (page_q * VM_PAGESIZE);

	return 0;
}

int get_fd()
{
	return g_memfd;
}

int vmprotect(void *start, unsigned len, int prot) {
	int osprot = (prot & VM_EXEC  ? PROT_EXEC  : 0) |
		     (prot & VM_WRITE ? PROT_WRITE : 0) |
		     (prot & VM_READ  ? PROT_READ  : 0);
	if (mprotect(start, len, osprot)) {
		perror("mprotect");
		return -1;
	}
	return 0;
}

int vminit(unsigned size) {
	int fd = memfd_create("mem", 0);
	if (fd < 0) {
		perror("memfd_create");
		return -1;
	}

	if (ftruncate(fd, size) < 0) {
		perror("ftrucate");
		return -1;
	}

	g_memfd = fd;
	g_memsize = size;
	return 0;
}