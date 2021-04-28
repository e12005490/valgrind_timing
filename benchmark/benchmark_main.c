#include "benchmark_histogram.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/slab.h>

#define SAMPLE_SIZE ((uint64_t)100000)

#define MEASURE(CALL, LO_START, HI_START, LO_END, HI_END, CTR) do { \
		asm volatile ("CPUID\n\t" \
			"mov %2, %%ecx\n\t" \
			"RDPMC\n\t" \
			"mov %%edx, %0\n\t" \
			"mov %%eax, %1\n\t": "=r" (HI_START), "=r" (LO_START): "i" (CTR): "%rax", "%rbx", "%rcx", "%rdx"); \
		CALL; \
		asm volatile ("RDPMC\n\t" \
			"mov %%edx, %0\n\t" \
			"mov %%eax, %1\n\t" \
			"CPUID\n\t": "=r" (HI_END), "=r" (LO_END): "i" (CTR): "%rax", "%rbx", "%rcx", "%rdx"); \
	} while(0)

#define WARMUP(CALL, CTR) do { \
		unsigned int lo_s, hi_s, lo_e, hi_e; \
		MEASURE(CALL, lo_s, hi_s, lo_e, hi_e, CTR); \
		MEASURE(CALL, lo_s, hi_s, lo_e, hi_e, CTR); \
	} while(0)

static void inline fill_times(histogram *const hist) {
	unsigned long flags;
	uint64_t i;
	uint64_t start, end;
	unsigned int lo_start, hi_start, lo_end, hi_end;
	const volatile int mul = 1;
	const unsigned int counter = (1 << 30) + 1;

	preempt_disable();
	raw_local_irq_save(flags);

	WARMUP(asm volatile ("mul %0\n\t":: "r" (mul) : "%rax"), counter);

	for(i = 0; i < SAMPLE_SIZE; ++i) {
		MEASURE(asm volatile ("mul %0\n\t":: "r" (mul) : "%rax"), lo_start, hi_start, lo_end, hi_end, counter);

		start = ((uint64_t)hi_start << 32) | lo_start;
		end = ((uint64_t)hi_end << 32) | lo_end;

		if(!histogram_insert(hist, end - start)) {
			printk(KERN_ERR "\n\ncould not insert time\n");
		}
	}

	raw_local_irq_restore(flags);
	preempt_enable();
}

static int __init benchmark_start(void) {
	histogram hist;
	uint64_t mean, variance, median, mad;

	printk(KERN_INFO "Loading benchmark module\n");

	histogram_init(&hist);

	fill_times(&hist);

	mean = histogram_mean(&hist);
	variance = histogram_variance(&hist, mean);
	median = histogram_median(&hist);
	mad = histogram_mad(&hist, median);

	printk(KERN_ERR "mean: %llu", mean);
	printk(KERN_ERR "variance: %llu", variance);
	printk(KERN_ERR "median: %llu", median);
	printk(KERN_ERR "mad: %llu", mad);

	histogram_print(&hist);

	histogram_free(&hist);

	return 0;
}

static void __exit benchmark_end(void) {
	printk(KERN_INFO "Unloading benchmark module\n");
}

module_init(benchmark_start);
module_exit(benchmark_end);
