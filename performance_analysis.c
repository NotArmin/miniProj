#include "performance_analysis.h"
#include "dtekv-lib.h"
#include "background.h"
#include "vga.h"

/* --- GLOBAL COUNTERS --- */
struct perf_counters before, after, delta;

/* --- INLINE ASM HELPERS --- */

/* Reset all machine performance counters */
void clear_counters(void) {
    asm volatile (
        "csrw mcycle, zero\n\t"
        "csrw minstret, zero\n\t"
        "csrw mhpmcounter3, zero\n\t"
        "csrw mhpmcounter4, zero\n\t"
        "csrw mhpmcounter5, zero\n\t"
        "csrw mhpmcounter6, zero\n\t"
        "csrw mhpmcounter7, zero\n\t"
        "csrw mhpmcounter8, zero\n\t"
        "csrw mhpmcounter9, zero\n\t"
    );
}

/* Read all counters manually */
void read_counters(struct perf_counters *c) {
    unsigned int lo, hi;

    asm volatile ("csrr %0, mcycle" : "=r"(lo));
    asm volatile ("csrr %0, mcycleh" : "=r"(hi));
    c->mcycle.lo = lo; c->mcycle.hi = hi;

    asm volatile ("csrr %0, minstret" : "=r"(lo));
    asm volatile ("csrr %0, minstreth" : "=r"(hi));
    c->minstret.lo = lo; c->minstret.hi = hi;

    asm volatile ("csrr %0, mhpmcounter3" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter3h" : "=r"(hi));
    c->mhpmcounter3.lo = lo; c->mhpmcounter3.hi = hi;

    asm volatile ("csrr %0, mhpmcounter4" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter4h" : "=r"(hi));
    c->mhpmcounter4.lo = lo; c->mhpmcounter4.hi = hi;

    asm volatile ("csrr %0, mhpmcounter5" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter5h" : "=r"(hi));
    c->mhpmcounter5.lo = lo; c->mhpmcounter5.hi = hi;

    asm volatile ("csrr %0, mhpmcounter6" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter6h" : "=r"(hi));
    c->mhpmcounter6.lo = lo; c->mhpmcounter6.hi = hi;

    asm volatile ("csrr %0, mhpmcounter7" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter7h" : "=r"(hi));
    c->mhpmcounter7.lo = lo; c->mhpmcounter7.hi = hi;

    asm volatile ("csrr %0, mhpmcounter8" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter8h" : "=r"(hi));
    c->mhpmcounter8.lo = lo; c->mhpmcounter8.hi = hi;

    asm volatile ("csrr %0, mhpmcounter9" : "=r"(lo));
    asm volatile ("csrr %0, mhpmcounter9h" : "=r"(hi));
    c->mhpmcounter9.lo = lo; c->mhpmcounter9.hi = hi;
}

/* Compute delta between two 64-bit counters */
void delta_counter(struct counter64 *res, struct counter64 *after, struct counter64 *before) {
    if (after->lo >= before->lo) {
        res->lo = after->lo - before->lo;
        res->hi = after->hi - before->hi;
    } else {
        res->lo = (0xFFFFFFFF - before->lo) + after->lo + 1;
        res->hi = after->hi - before->hi - 1;
    }
}

/* Call before measuring */
void before_perf(void) {
    clear_counters();
    read_counters(&before);
}

void present_data(const char* filter_name) {
    read_counters(&after);
    
    /* Compute deltas */
    delta_counter(&delta.mcycle, &after.mcycle, &before.mcycle);
    delta_counter(&delta.minstret, &after.minstret, &before.minstret);
    delta_counter(&delta.mhpmcounter3, &after.mhpmcounter3, &before.mhpmcounter3);
    delta_counter(&delta.mhpmcounter4, &after.mhpmcounter4, &before.mhpmcounter4);
    delta_counter(&delta.mhpmcounter5, &after.mhpmcounter5, &before.mhpmcounter5);
    delta_counter(&delta.mhpmcounter6, &after.mhpmcounter6, &before.mhpmcounter6);
    delta_counter(&delta.mhpmcounter7, &after.mhpmcounter7, &before.mhpmcounter7);
    delta_counter(&delta.mhpmcounter8, &after.mhpmcounter8, &before.mhpmcounter8);
    delta_counter(&delta.mhpmcounter9, &after.mhpmcounter9, &before.mhpmcounter9);

    /* Use only low 32 bits */
    unsigned int cycles = delta.mcycle.lo;
    unsigned int instructions = delta.minstret.lo;
    unsigned int memory_ops = delta.mhpmcounter3.lo;
    unsigned int icache_misses = delta.mhpmcounter4.lo;
    unsigned int dcache_misses = delta.mhpmcounter5.lo;
    unsigned int icache_stalls = delta.mhpmcounter6.lo;
    unsigned int dcache_stalls = delta.mhpmcounter7.lo;
    unsigned int data_hazard_stalls = delta.mhpmcounter8.lo;
    unsigned int alu_stalls = delta.mhpmcounter9.lo;

    /* Print raw counter values */
    print("\n=== Performance: ");
    print(filter_name);
    print(" ===\n");
    
    print("Cycles: "); print_dec(cycles); printc('\n');
    print("Instructions: "); print_dec(instructions); printc('\n');
    print("Memory Instructions: "); print_dec(memory_ops); printc('\n');
    print("I-cache Misses: "); print_dec(icache_misses); printc('\n');
    print("D-cache Misses: "); print_dec(dcache_misses); printc('\n');
    print("I-cache Stalls: "); print_dec(icache_stalls); printc('\n');
    print("D-cache Stalls: "); print_dec(dcache_stalls); printc('\n');
    print("Data Hazard Stalls: "); print_dec(data_hazard_stalls); printc('\n');
    print("ALU Stalls: "); print_dec(alu_stalls); printc('\n');
    
    // Optional: Add execution time since it's simple and useful
    unsigned int execution_time_ms = cycles / 30000;
    print("Execution Time (ms): "); print_dec(execution_time_ms); printc('\n');
}

/* Test performance of a specific filter */
void test_filter_performance(const char* filter_name, 
    void (*filter_func)(const unsigned char[][320], volatile unsigned char*)) {
    before_perf();
    filter_func(Bliss, BUF0);
    present_data(filter_name);
}