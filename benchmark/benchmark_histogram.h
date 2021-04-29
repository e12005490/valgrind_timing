#ifndef BENCHMARK_HISTOGRAM_H
#define BENCHMARK_HISTOGRAM_H

#include <linux/types.h>

typedef struct {
	uint64_t time;
	uint64_t occurrences;
} time_pair;

// times are stored as a histogram
typedef struct {
	uint64_t allocated_size;
	uint64_t size;
	time_pair *times;
} histogram;

/**
 * Returns whether the initialization was successful
 */
int histogram_init(histogram *hist);

/**
 * Returns whether inserting a time into a histogram was successful
 */
int histogram_insert(histogram *hist, uint64_t time);

/**
 * Print histogram size and elements
 */
void histogram_print(const histogram *hist);

/**
 * Free histogram resources
 */
void histogram_free(histogram *hist);

uint64_t histogram_mean(const histogram *hist);

uint64_t histogram_variance(const histogram *hist, uint64_t mean);

uint64_t histogram_median(const histogram *hist);

/**
 * Median Absolute Deviation
 */
uint64_t histogram_mad(const histogram *hist, uint64_t median);

#endif
