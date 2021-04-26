#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/hardirq.h>
#include <linux/preempt.h>
#include <linux/sched.h>
#include <linux/slab.h>

#define SAMPLE_SIZE ((uint64_t)1000000)

typedef struct {
	uint64_t time;
	uint64_t occurrences;
} time_pair;

typedef struct {
	uint64_t allocated_size;
	uint64_t size;
	time_pair *times;
} time_map;

/**
 * Returns whether the initialization was successful
 */
int time_map_init(time_map *const map) {
	map->allocated_size = 10;
	map->size = 0;
	map->times = kmalloc(map->allocated_size * sizeof(time_pair), GFP_KERNEL);
	if(map->times == NULL) { return 0; }
	return 1;
}

/**
 * Returns whether inserting a time into a time_map was successful
 */
int time_map_insert(time_map *map, uint64_t time) {
	uint64_t i;
	uint64_t new_size;
	time_pair *tmp;
	time_pair swp;

	for(i = 0; i < map->size; ++i) {
		if(map->times[i].time == time) {
			++map->times[i].occurrences;
			return 1;
		}
	}

	// not in map yet
	if(map->size == map->allocated_size) {
		new_size = 2 * map->allocated_size;
		tmp = krealloc(map->times, new_size * sizeof(time_pair), GFP_KERNEL);
		if(tmp == NULL) {
			return 0;
		}
		map->times = tmp;
		map->allocated_size = new_size;
	}

	// insert new pair
	map->times[map->size].time = time;
	map->times[map->size].occurrences = 1;
	++map->size;

	// (insertion) sort times
	for(i = map->size - 1; i > 0 && map->times[i - 1].time > map->times[i].time; --i) {
		swp.time = map->times[i - 1].time;
		swp.occurrences = map->times[i - 1].occurrences;
		map->times[i - 1].time = map->times[i].time;
		map->times[i - 1].occurrences = map->times[i].occurrences;
		map->times[i].time = swp.time;
		map->times[i].occurrences = swp.occurrences;
	}

	return 1;
}

void time_map_free(time_map *map) {
	map->allocated_size = 0;
	map->size = 0;
	kfree(map->times);
}

uint64_t time_map_mean(time_map *map) {
	uint64_t i;
	uint64_t result = 0, n = 0;

	for(i = 0; i < map->size; ++i) {
		result += map->times[i].time * map->times[i].occurrences;
		n += map->times[i].occurrences;
	}

	return result / n;
}

uint64_t time_map_variance(time_map *map, uint64_t mean) {
	uint64_t i;
	uint64_t result = 0, n = 0;

	for(i = 0; i < map->size; ++i) {
		result += (map->times[i].time - mean) * (map->times[i].time - mean) * map->times[i].occurrences;
		n += map->times[i].occurrences;
	}

	return result / n;
}

uint64_t time_map_median(time_map *map) {
	uint64_t length = 0, mid;
	uint64_t i;

	for(i = 0; i < map->size; ++i) {
		length += map->times[i].occurrences;
	}

	mid = length / 2;

	// find the mid point by iterating on groups
	if(length % 2 == 0) {
		for(i = 0; i < map->size; ++i) {
			if(mid >= map->times[i].occurrences) { // not in this group, go to next
				mid -= map->times[i].occurrences;
			} else if(mid == map->times[i].occurrences - 1) { // between this one and the next
				return (map->times[i].time + map->times[i + 1].time) / 2;
			} else { // in this one
				return map->times[i].time;
			}
		}
	} else {
		for(i = 0; i < map->size; ++i) {
			if(mid >= map->times[i].occurrences) { // not in this one, go to next
				mid -= map->times[i].occurrences;
			} else { // in this one
				return map->times[i].time;
			}
		}
	}

	// should never get there
	return 0;
}

/**
 * Median Absolute Deviation
 */
uint64_t time_map_mad(time_map *map, uint64_t median) {
	uint64_t i, j;
	time_map mad_map;
	uint64_t mad;

	time_map_init(&mad_map);

	for(i = 0; i < map->size; ++i) {
		for(j = 0; j < map->times[i].occurrences; ++j) {
			time_map_insert(&mad_map, map->times[i].time > median ? map->times[i].time - median : median - map->times[i].time);
		}
	}

	mad = time_map_median(&mad_map);

	time_map_free(&mad_map);

	return mad;
}

void inline fill_times(time_map *map) {
	unsigned long flags;
	uint64_t i;
	uint64_t start, end;
	unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;
	volatile int mul = 1;

	// warmup

	asm volatile ("CPUID\n\t"
		"RDTSC\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");
	asm volatile ("mul %0\n\t":: "r" (mul) : "%eax");
	asm volatile ("RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx", "%rdx");
	asm volatile ("CPUID\n\t"
		"RDTSC\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");
	asm volatile ("mul %0\n\t":: "r" (mul) : "%eax");
	asm volatile ("RDTSCP\n\t"
		"mov %%edx, %0\n\t"
		"mov %%eax, %1\n\t"
		"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx", "%rdx");

	for(i = 0; i < SAMPLE_SIZE; ++i) {
		preempt_disable();
		raw_local_irq_save(flags);

		asm volatile ("CPUID\n\t"
			"RDTSC\n\t"
			"mov %%edx, %0\n\t"
			"mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax", "%rbx", "%rcx", "%rdx");
		/***********************************/
		/*call the function to measure here*/

		asm volatile ("mul %0\n\t":: "r" (mul) : "%eax");

		/***********************************/
		asm volatile ("RDTSCP\n\t"
			"mov %%edx, %0\n\t"
			"mov %%eax, %1\n\t"
			"CPUID\n\t": "=r" (cycles_high1), "=r" (cycles_low1):: "%rax", "%rbx", "%rcx", "%rdx");

		raw_local_irq_restore(flags);
		preempt_enable();

		start = ((uint64_t)cycles_high << 32) | cycles_low;
		end = ((uint64_t)cycles_high1 << 32) | cycles_low1;

		if(!time_map_insert(map, end - start)) {
			printk(KERN_ERR "\n\ncould not insert time\n");
		}
	}
}

static int __init benchmark_start(void) {
	uint64_t i;
	time_map map;
	uint64_t mean, variance, median;

	printk(KERN_INFO "Loading benchmark module\n");

	time_map_init(&map);

	fill_times(&map);

	mean = time_map_mean(&map);
	variance = time_map_variance(&map, mean);
	median = time_map_median(&map);

	printk(KERN_ERR "mean: %llu", mean);
	printk(KERN_ERR "variance: %llu", variance);
	printk(KERN_ERR "median: %llu", median);
	printk(KERN_ERR "mad: %llu", time_map_mad(&map, median));

	printk("map size: %llu", map.size);
	for(i = 0; i < map.size; ++i) {
		printk("%llu, %llu", map.times[i].time, map.times[i].occurrences);
	}

	time_map_free(&map);

	return 0;
}

static void __exit benchmark_end(void) {
	printk(KERN_INFO "Unloading benchmark module\n");
}

module_init(benchmark_start);
module_exit(benchmark_end);
