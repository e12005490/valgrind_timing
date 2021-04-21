#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/slab.h>

#define SIZE_OF_STAT ((uint64_t)2000)
#define BOUND_OF_LOOP ((uint64_t)5000)

#define SHIFT_AMOUNT 3

void inline fill_times(uint64_t **times) {
	unsigned long flags;
	uint64_t i, j;
	uint64_t start, end;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	volatile int to_shift;

	asm volatile ("CPUID\n\t"
		"RDTSC\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");
	asm volatile ("RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx", "%rdx");
	asm volatile ("CPUID\n\t"
		"RDTSC\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");
	asm volatile ("RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx", "%rdx");

	for(i = 0; i < BOUND_OF_LOOP; ++i) {
		for(j = 0; j < SIZE_OF_STAT; ++j) {
			preempt_disable();
			raw_local_irq_save(flags);

			asm volatile ("CPUID\n\t"
				"RDTSC\n\t"
				"mov %%edx, %0\n\t"
				"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");
			/***********************************/
			/*call the function to measure here*/
			/***********************************/
			to_shift <<= SHIFT_AMOUNT;

			asm volatile ("RDTSCP\n\t"
				"mov %%edx, %0\n\t"
				"mov %%eax, %1\n\t"
				"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx", "%rdx");

			raw_local_irq_restore(flags);
			preempt_enable();

			start = ((uint64_t)cycles_high << 32) | cycles_low;
			end = ((uint64_t)cycles_high1 << 32) | cycles_low1;

			if((end - start) < 0) {
				printk(KERN_ERR "\n\n>>>>>>>>>>>>>> CRITICAL ERROR IN TAKING THE TIME!!!!!!\n loop(%llu) stat(%llu) start = %llu, end = %llu\n", i, j, start, end);
				times[i][j] = 0;
			} else {
				times[i][j] = end - start;
			}
		}
	}
}

uint64_t mean(uint64_t **times) {
	uint64_t i, j;
	uint64_t res = 0, prev = 0;

	for(i = 0; i < BOUND_OF_LOOP; ++i) {
		for(j = 0; j < SIZE_OF_STAT; ++j) {
			if(res < prev) {
				printk(KERN_ERR "\n\n>>>>>>>>>>>>>> CRITICAL OVERFLOW ERROR IN mean!!!!!!\n\n");
				return -EINVAL;
			}
			res += times[i][j];
			prev = res;
		}
	}

	return res / (BOUND_OF_LOOP * SIZE_OF_STAT);
}

/* using König's formula */
uint64_t variance(uint64_t **times) {
	uint64_t i, j;
	uint64_t e_t_squared = 0, e_squared_t = 0, prev_e_t_squared = 0, prev_e_squared_t = 0;
	uint64_t size = BOUND_OF_LOOP * SIZE_OF_STAT;

	for(i = 0; i < BOUND_OF_LOOP; ++i) {
		for(j = 0; j < SIZE_OF_STAT; ++j) {
			if((e_squared_t < prev_e_squared_t) || (e_t_squared < prev_e_t_squared)) { goto overflow; }
			e_squared_t += times[i][j];
			prev_e_squared_t = e_squared_t;
			e_t_squared += times[i][j] * times[i][j];
			prev_e_t_squared = e_t_squared;
		}
	}

	e_squared_t *= e_squared_t;
	e_t_squared *= size;
	if((e_squared_t < prev_e_squared_t) || (e_t_squared < prev_e_t_squared)) { goto overflow; }

	return (e_t_squared - e_squared_t) / (size * size);

overflow:
	printk(KERN_ERR "\n\n>>>>>>>>>>>>>> CRITICAL OVERFLOW ERROR IN var_calc!!!!!!\n\n");
	return -EINVAL;
}

static int __init benchmark_start(void) {
	uint64_t i = 0, j = 0;
	uint64_t **times;

	printk(KERN_INFO "Loading benchmark module\n");

	times = kmalloc(BOUND_OF_LOOP * sizeof(uint64_t*), GFP_KERNEL);
	if(!times) {
		printk(KERN_ERR "unable to allocate memory for times\n");
		return 0;
	}

	for(i = 0; i < BOUND_OF_LOOP; ++i) {
		times[i] = kmalloc(SIZE_OF_STAT * sizeof(uint64_t), GFP_KERNEL);
		if(!times[i]) {
			printk(KERN_ERR "unable to allocate memory for times[%llu]\n", j);
			for(j = 0; j < i; ++j) {
				kfree(times[j]);
			}
			return 0;
		}
	}

	fill_times(times);

	printk(KERN_ERR "shift %d", SHIFT_AMOUNT);
	printk(KERN_ERR "mean: %llu", mean(times));
	printk(KERN_ERR "variance: %llu", variance(times));

	for(i = 0; i < BOUND_OF_LOOP; ++i) {
		kfree(times[i]);
	}
	kfree(times);
	return 0;
}

static void __exit benchmark_end(void) {
	printk(KERN_INFO "Unloading benchmark module\n");
}

module_init(benchmark_start);
module_exit(benchmark_end);
