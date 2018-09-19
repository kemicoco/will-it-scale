#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <assert.h>
#include <numaif.h>
#include <numa.h>

#define PAGE_COUNT 10000

char *testcase_description = "page migration of a single heavily used anonymous page";
void **pages;
int *nodes;
int *status;
int nr_nodes, pagesize;

void testcase_prepare(unsigned long nr_tasks)
{
	int i, rc;
	void *page;
	pagesize = getpagesize();
	nr_nodes = numa_max_node() + 1;
	if (nr_nodes < 2) {
		printf("error: numa node number is %d, less than 2, please use a NUMA \
platform for testing\n", nr_nodes);
		exit(-1);
	}
	char *page_base = mmap(NULL, (PAGE_COUNT + 1) * pagesize, PROT_READ | PROT_WRITE,
							MAP_SHARED | MAP_ANONYMOUS, -1 , 0);
	assert(page_base != MAP_FAILED);
	/* Page size boundary */
	page = (void *)(((long)page_base & ~((long)(pagesize - 1))) + pagesize);
	pages = malloc(sizeof(void *) * PAGE_COUNT);
	nodes = malloc(sizeof(int *) * PAGE_COUNT);
	status = malloc(sizeof(int *) * PAGE_COUNT);
	if (!pages || !nodes || !status) {
		printf("unable to allocate memory, exit\n");
		exit(-1);
	}

	for (i = 0; i < PAGE_COUNT; i++) {
		pages[i] = page + i * pagesize;
		nodes[i] = i % nr_nodes;
		status[i] = -123;
	}

	rc = move_pages(0, 1, pages, NULL, status, MPOL_MF_MOVE_ALL);
	if (rc < 0) {
		printf("error: page %p in node %d\n", pages[0], status[0]);
		exit(-1);
	}
}


void testcase(unsigned long long *iterations, unsigned long nr)
{
	int rc;
	char *buf;
	buf = malloc(pagesize + 1);
	if (nr == 0) {
		while (1) {
			rc = move_pages(0, 1, pages, nodes, status, MPOL_MF_MOVE_ALL);
			if (rc < 0) {
				printf("unable to move page %p to node %d, current status \
of page is %d\n", pages[0],	nodes[0], status[0]);
				exit(-1);
			}
			nodes[0] = (nodes[0] + 1) % nr_nodes;
		}
	}
	else {
		while (1) {
			memcpy(buf, pages[0], pagesize);
			(*iterations)++;
		}
	}
}
