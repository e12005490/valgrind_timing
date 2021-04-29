#include "benchmark_histogram.h"
#include "benchmark_util.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define LOOP_SIZE ((uint64_t)100)

static int __init benchmark_start(void) {
	histogram hist;
	uint64_t i, j;
	uint64_t median, mad;
	unsigned int accumulator;
	unsigned int multiplier;

	printk(KERN_INFO "Loading benchmark module\n");

	for(i = 0; i < LOOP_SIZE; ++i) {
		accumulator = i;

		for(j = 0; j < LOOP_SIZE; ++j) {
			histogram_init(&hist);

			multiplier = j;

			FILL_TIMES(&hist, asm volatile ("mov %0, %%eax\n\tmul %1\n\t":: "r" (accumulator), "r" (multiplier) : "%rax"));

			median = histogram_median(&hist);
			mad = histogram_mad(&hist, median);

			printk("%llu *= %llu median %llu, mad %llu", i, j, median, mad);

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
