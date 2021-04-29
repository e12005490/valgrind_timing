#ifndef BENCHMARK_UTIL_H
#define BENCHMARK_UTIL_H

#include "benchmark_histogram.h"
#include <linux/types.h>
#include <linux/hardirq.h>
#include <linux/kernel.h>
#include <linux/preempt.h>
#include <linux/sched.h>

#define SAMPLE_SIZE ((uint64_t)10000)

// these are all macros to force code inlining

/*
 * see https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf
 */
#define MEASURE(CALL, LO_START, HI_START, LO_END, HI_END, CTR) \
	do { \
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

#define WARMUP(CALL, CTR) \
	do { \
		unsigned int lo_s, hi_s, lo_e, hi_e; \
		MEASURE(CALL, lo_s, hi_s, lo_e, hi_e, CTR); \
		MEASURE(CALL, lo_s, hi_s, lo_e, hi_e, CTR); \
	} while(0)

/*
 * see https://community.intel.com/t5/Software-Tuning-Performance/How-to-read-performance-counters-by-rdpmc-instruction/td-p/1009043
 * for use of rdmsr, wrmsr
 */
#define FILL_TIMES(HIST, BENCH_CALL) \
	do { \
		unsigned long flags; \
		uint64_t i; \
		uint64_t start, end; \
		unsigned int lo_start, hi_start, lo_end, hi_end; \
		const unsigned int counter = (1 << 30) + 1; \
		const unsigned int msr_38F = 0x38F, msr_38D = 0x38D; \
		unsigned int old_msr_38F_lo, old_msr_38F_hi, new_msr_38F_hi; \
		const unsigned int new_msr_38D_lo = 0x111; \
		unsigned int old_msr_38D_lo, old_msr_38D_hi; \
			\
		preempt_disable(); \
		raw_local_irq_save(flags); \
			\
		asm volatile ("mov %2, %%ecx\n\t" /* read IA32_PERF_GLOBAL_CTRL */ \
			"rdmsr\n\t" \
			"mov %%edx, %0\n\t" \
			"mov %%eax, %1\n\t": "=r" (old_msr_38F_hi), "=r" (old_msr_38F_lo): "i" (msr_38F): "%rax", "%rcx", "%rdx"); \
		new_msr_38F_hi = old_msr_38F_hi | 0x7; \
		asm volatile ("mov %0, %%ecx\n\t" /* enable fixed function counters */ \
			"mov %1, %%edx\n\t" \
			"mov %2, %%eax\n\t" \
			"wrmsr":: "i" (msr_38F), "r" (new_msr_38F_hi), "r" (old_msr_38F_lo): "%rax", "%rcx", "%rdx"); \
			\
		asm volatile ("mov %2, %%ecx\n\t" /* read IA32_FIXED_CTR_CTRL */ \
			"rdmsr\n\t" \
			"mov %%edx, %0\n\t" \
			"mov %%eax, %1\n\t": "=r" (old_msr_38D_hi), "=r" (old_msr_38D_lo): "i" (msr_38D): "%rax", "%rcx", "%rdx"); \
		asm volatile ("mov %0, %%ecx\n\t" /* enable counters in kernel mode */ \
			"mov %1, %%edx\n\t" \
			"mov %2, %%eax\n\t" \
			"wrmsr":: "i" (msr_38D), "r" (old_msr_38D_hi), "r" (new_msr_38D_lo): "%rax", "%rcx", "%rdx"); \
			\
		WARMUP(BENCH_CALL, counter); \
			\
		for(i = 0; i < SAMPLE_SIZE; ++i) { \
			MEASURE(BENCH_CALL, lo_start, hi_start, lo_end, hi_end, counter); \
				\
			start = ((uint64_t)hi_start << 32) | lo_start; \
			end = ((uint64_t)hi_end << 32) | lo_end; \
				\
			if(!histogram_insert((HIST), end - start)) { \
				printk(KERN_ERR "\n\ncould not insert time\n"); \
			} \
		} \
			\
		asm volatile ("mov %0, %%ecx\n\t" /* restore fixed function counters control */ \
			"mov %1, %%edx\n\t" \
			"mov %2, %%eax\n\t" \
			"wrmsr":: "i" (msr_38F), "r" (old_msr_38F_hi), "r" (old_msr_38F_lo): "%rax", "%rcx", "%rdx"); \
		asm volatile ("mov %0, %%ecx\n\t" /* restore fixed function counters permissions */ \
			"mov %1, %%edx\n\t" \
			"mov %2, %%eax\n\t" \
			"wrmsr":: "i" (msr_38D), "r" (old_msr_38D_hi), "r" (old_msr_38D_lo): "%rax", "%rcx", "%rdx"); \
			\
		raw_local_irq_restore(flags); \
		preempt_enable(); \
	} while(0)

#endif
