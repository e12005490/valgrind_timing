#include "benchmark_histogram.h"
#include "benchmark_util.h"
#include <asm/fpu/api.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define LOOP_SIZE ((uint64_t)10000)
#define L2 ((uint64_t)1000)

static int __init benchmark_start(void) {
	histogram hist;
	uint64_t i, j;
	uint64_t median, mad;
	uint64_t op1, op2;

	printk(KERN_INFO "Loading benchmark module\n");

	for(i = 0; i < LOOP_SIZE; ++i) {
		op1 = i;
		for(j = 0; j < L2; ++j) {
			op2 = j;
			histogram_init(&hist);

			kernel_fpu_begin();
			FILL_TIMES(&hist, asm volatile (
				"movq %0, %%xmm0\n\tmovq %1, %%xmm1\n\tvpxor %%xmm0, %%xmm1, %%xmm1\n\t":: "r" (op1), "r" (op2) : "%xmm0", "%xmm1"
			));
			kernel_fpu_end();

			median = histogram_median(&hist);
			mad = histogram_mad(&hist, median);

			printk("%llu ^ %llu median %llu, mad %llu", i, j, median, mad);

			histogram_free(&hist);
		}
	}

	return 0;
}

static void __exit benchmark_end(void) {
	printk(KERN_INFO "Unloading benchmark module\n");
}

module_init(benchmark_start);
module_exit(benchmark_end);

MODULE_LICENSE("GPL");
