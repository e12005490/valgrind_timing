#include "benchmark_histogram.h"
#include "benchmark_util.h"
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

static int __init benchmark_start(void) {
	histogram hist;
	uint64_t i;
	uint64_t median, mad;
	unsigned int multiplier;

	printk(KERN_INFO "Loading benchmark module\n");

	for(i = 1; i < 1000; ++i) {
		histogram_init(&hist);

		multiplier = i;

		FILL_TIMES(&hist, asm volatile ("mul %0\n\t":: "r" (multiplier) : "%rax"));

		median = histogram_median(&hist);
		mad = histogram_mad(&hist, median);

		printk("%llu median %llu, mad %llu", i, median, mad);

		histogram_free(&hist);
	}

	return 0;
}

static void __exit benchmark_end(void) {
	printk(KERN_INFO "Unloading benchmark module\n");
}

module_init(benchmark_start);
module_exit(benchmark_end);
