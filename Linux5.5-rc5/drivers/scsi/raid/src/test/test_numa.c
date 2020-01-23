#include "ci.h"

#ifndef _WIN32
static void linux_test()
{
	ci_printf("numa_available() = %d\n", 			numa_available());		/* < 0 means error, 0 is OK */
	ci_printf("numa_max_node() = %d\n", 			numa_max_node());		/* = total - 1 */
	ci_printf("numa_preferred() = %d\n", 			numa_preferred());
	ci_printf("\n");
	
	ci_printf("numa_max_possible_node() = %d\n", 	numa_max_possible_node());
	ci_printf("numa_num_possible_nodes() = %d\n", 	numa_num_possible_nodes());
	ci_printf("numa_num_configured_nodes() = %d\n", numa_num_configured_nodes());
	ci_printf("numa_num_configured_cpus() = %d\n", 	numa_num_configured_cpus());
	ci_printf("numa_pagesize() = %d\n", 			numa_pagesize());

	
//	numa_alloc_local(size)
	ci_printf("\n");

	ci_loop(node, numa_max_node() + 1) {
		int size = 128;
		void *ptr = numa_alloc_onnode(size, node);
		ci_panic_if(!ptr);
		ci_printf("node=%d, ptr=%p\n", node, ptr);
	}

#define MAX_BITS		1024
	static unsigned long mask_buf[MAX_BITS / (sizeof(unsigned long) * 8)];
	struct bitmask mask = { MAX_BITS, mask_buf };
	
	ci_loop(node, numa_max_node() + 1) {
		numa_node_to_cpus(node, &mask);
		ci_printf("mask[%d]=%#016lX\n", 0, mask_buf[0]);
		ci_printf("\n");
	}
}
#endif

void test_numa()
{
#ifndef _WIN32
	linux_test();
#endif
}






