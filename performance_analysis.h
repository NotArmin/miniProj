// performance_analysis.h

#ifndef PERFORMANCE_ANALYSIS_H
#define PERFORMANCE_ANALYSIS_H

struct counter64 {
    unsigned int lo;
    unsigned int hi;
};

struct perf_counters {
    struct counter64 mcycle;
    struct counter64 minstret;
    struct counter64 mhpmcounter3;
    struct counter64 mhpmcounter4;
    struct counter64 mhpmcounter5;
    struct counter64 mhpmcounter6;
    struct counter64 mhpmcounter7;
    struct counter64 mhpmcounter8;
    struct counter64 mhpmcounter9;
};

// Global counters
extern struct perf_counters before, after, delta;

// Function declarations
void clear_counters(void);
void read_counters(struct perf_counters *c);
void before_perf(void);
void present_data(const char* filter_name);
void test_filter_performance(const char* filter_name, void (*filter_func)(const unsigned char[][320], volatile unsigned char*));

#endif