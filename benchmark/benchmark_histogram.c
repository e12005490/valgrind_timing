#include "benchmark_histogram.h"
#include <linux/slab.h>
#include <linux/types.h>

int histogram_init(histogram *const hist) {
	hist->allocated_size = 10;
	hist->size = 0;
	hist->times = kmalloc(hist->allocated_size * sizeof(time_pair), GFP_KERNEL);
	return hist->times != NULL;
}

int histogram_insert(histogram *const hist, const uint64_t time) {
	uint64_t i;
	uint64_t new_size;
	time_pair *tmp;
	time_pair swp;

	for(i = 0; i < hist->size; ++i) {
		if(hist->times[i].time == time) {
			++hist->times[i].occurrences;
			return 1;
		}
	}

	// not in histogram yet
	if(hist->size == hist->allocated_size) {
		new_size = 2 * hist->allocated_size;
		tmp = krealloc(hist->times, new_size * sizeof(time_pair), GFP_KERNEL);
		if(tmp == NULL) {
			return 0;
		}
		hist->times = tmp;
		hist->allocated_size = new_size;
	}

	// insert new pair
	hist->times[hist->size].time = time;
	hist->times[hist->size].occurrences = 1;
	++hist->size;

	// (insertion) sort times
	for(i = hist->size - 1; i > 0 && hist->times[i - 1].time > hist->times[i].time; --i) {
		swp.time = hist->times[i - 1].time;
		swp.occurrences = hist->times[i - 1].occurrences;
		hist->times[i - 1].time = hist->times[i].time;
		hist->times[i - 1].occurrences = hist->times[i].occurrences;
		hist->times[i].time = swp.time;
		hist->times[i].occurrences = swp.occurrences;
	}

	return 1;
}

void histogram_print(const histogram *const hist) {
	uint64_t i;

	printk("histogram size %llu", hist->size);
	for(i = 0; i < hist->size; ++i) {
		printk("%llu, %llu", hist->times[i].time, hist->times[i].occurrences);
	}
}

void histogram_free(histogram *const hist) {
	hist->allocated_size = 0;
	hist->size = 0;
	kfree(hist->times);
}

uint64_t histogram_mean(const histogram *const hist) {
	uint64_t i;
	uint64_t result = 0, n = 0;

	for(i = 0; i < hist->size; ++i) {
		result += hist->times[i].time * hist->times[i].occurrences;
		n += hist->times[i].occurrences;
	}

	return result / n;
}

uint64_t histogram_variance(const histogram *const hist, const uint64_t mean) {
	uint64_t i;
	uint64_t result = 0, n = 0;

	for(i = 0; i < hist->size; ++i) {
		result += (hist->times[i].time - mean) * (hist->times[i].time - mean) * hist->times[i].occurrences;
		n += hist->times[i].occurrences;
	}

	return result / n;
}

uint64_t histogram_median(const histogram *const hist) {
	uint64_t length = 0, mid;
	uint64_t i;

	for(i = 0; i < hist->size; ++i) {
		length += hist->times[i].occurrences;
	}

	mid = length / 2;

	// find the mid point by iterating on groups
	if(length % 2 == 0) {
		for(i = 0; i < hist->size; ++i) {
			if(mid >= hist->times[i].occurrences) { // not in this group, go to next
				mid -= hist->times[i].occurrences;
			} else if(mid == hist->times[i].occurrences - 1) { // between this one and the next
				return (hist->times[i].time + hist->times[i + 1].time) / 2;
			} else { // in this one
				return hist->times[i].time;
			}
		}
	} else {
		for(i = 0; i < hist->size; ++i) {
			if(mid >= hist->times[i].occurrences) { // not in this one, go to next
				mid -= hist->times[i].occurrences;
			} else { // in this one
				return hist->times[i].time;
			}
		}
	}

	// should never get there
	printk(KERN_ERR "UNREACHABLE STATE IN HISTOGRAM_MEDIAN");
	return 0;
}

uint64_t histogram_mad(const histogram *const hist, const uint64_t median) {
	uint64_t i, j;
	histogram mad_hist;
	uint64_t mad;
	uint64_t t;

	if(!histogram_init(&mad_hist)) {
		return 0;
	}

	for(i = 0; i < hist->size; ++i) {
		for(j = 0; j < hist->times[i].occurrences; ++j) {
			t = hist->times[i].time;
			if(!histogram_insert(&mad_hist, t > median ? t - median : median - t)) {
				histogram_free(&mad_hist);
				return 0;
			}
		}
	}

	mad = histogram_median(&mad_hist);

	histogram_free(&mad_hist);

	return mad;
}
