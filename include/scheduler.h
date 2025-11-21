#ifndef SOLIX_SCHEDULER_H
#define SOLIX_SCHEDULER_H

#include "types.h"
#include "kernel.h"

/**
 * Linux-Inspired Process Scheduler for SolixOS
 * Implements O(1) scheduler with priority arrays and runqueues
 * Based on Linux 2.6 scheduler design principles
 */

// Scheduling policies (Linux-inspired)
#define SCHED_NORMAL    0   // Normal timesharing process
#define SCHED_FIFO      1   // Real-time FIFO
#define SCHED_RR        2   // Real-time round-robin
#define SCHED_BATCH     3   // Batch style (similar to SCHED_IDLE)
#define SCHED_IDLE      4   // Idle process

// Priority ranges (Linux-style)
#define MAX_PRIO        140
#define MIN_PRIO        0
#define NICE_TO_PRIO(nice)    (MAX_PRIO - 20 + (nice))
#define PRIO_TO_NICE(prio)    (MAX_PRIO - (prio) - 20)
#define DEFAULT_PRIO     120
#define MAX_RT_PRIO     100
#define MAX_USER_PRIO   140

// Time slices (Linux jiffies-inspired)
#define HZ              100     // Timer frequency
#define BASE_TIMESLICE  (100 * HZ / 1000)  // 100ms base timeslice
#define MIN_TIMESLICE   (20 * HZ / 1000)   // 20ms minimum
#define MAX_TIMESLICE   (200 * HZ / 1000)  // 200ms maximum

/**
 * Runqueue structure - Linux O(1) scheduler design
 */
typedef struct runqueue {
    // Active and expired priority arrays
    struct prio_array {
        struct list_head queue[MAX_PRIO];
        uint32_t bitmap[BITMAP_SIZE];
    } active, expired;
    
    // Current running task
    process_t* curr;
    
    // Load statistics
    uint32_t nr_running;
    uint32_t nr_switches;
    uint32_t expired_timestamp;
    uint32_t best_expired_prio;
    
    // Load balancing
    uint32_t cpu_load;
    uint32_t nr_uninterruptible;
} runqueue_t;

/**
 * Priority array bitmap helpers
 */
#define BITMAP_SIZE     ((MAX_PRIO + 31) / 32)
#define BITMAP_WORD(bit)    ((bit) / 32)
#define BITMAP_BIT(bit)      (1UL << ((bit) % 32))

/**
 * Task structure enhancements for Linux-style scheduling
 */
struct sched_entity {
    // Virtual runtime for CFS-inspired scheduling
    uint64_t vruntime;
    uint64_t exec_start;
    uint64_t sum_exec_runtime;
    uint64_t prev_sum_exec_runtime;
    uint64_t nr_migrations;
    
    // Load weight
    uint64_t load_weight;
    uint32_t inv_weight;
    
    // Sleep statistics
    uint64_t sleep_start;
    uint64_t block_start;
    uint64_t sleep_runtime;
    
    // Time slice
    uint32_t time_slice;
    uint32_t first_time_slice;
    
    // Scheduling class
    uint32_t policy;
    uint32_t rt_priority;
    
    // Priority
    uint32_t prio;
    uint32_t static_prio;
    uint32_t normal_prio;
    
    // Runqueue placement
    struct list_head run_list;
    runqueue_t* rq;
};

// Extend process structure with scheduling entity
#define sched_entity(proc) ((struct sched_entity*)&(proc)->sched_data)

/**
 * Scheduler classes (Linux-inspired)
 */
struct sched_class {
    const char *name;
    void (*enqueue_task)(process_t *p, int head);
    void (*dequeue_task)(process_t *p);
    void (*yield_task)(process_t *p);
    void (*check_preempt_curr)(process_t *p);
    process_t* (*pick_next_task)(runqueue_t *rq);
    void (*put_prev_task)(runqueue_t *rq, process_t *p);
    void (*task_tick)(runqueue_t *rq, process_t *p);
    void (*task_fork)(process_t *p);
    void (*switched_from)(runqueue_t *rq, process_t *p);
    void (*switched_to)(runqueue_t *rq, process_t *p);
    void (*prio_changed)(runqueue_t *rq, process_t *p);
    void (*set_curr_task)(runqueue_t *rq, process_t *p);
};

// External scheduler class declarations
extern const struct sched_class sched_fair_class;
extern const struct sched_class sched_rt_class;
extern const struct sched_class sched_idle_class;

/**
 * Core scheduler functions
 */
void scheduler_init(void);
void schedule(void);
void preempt_schedule(void);
void scheduler_tick(void);
process_t* pick_next_task(runqueue_t *rq);
void enqueue_task(process_t *p, int head);
void dequeue_task(process_t *p);
void yield_task(process_t *p);

/**
 * Priority and timeslice management
 */
uint32_t task_timeslice(process_t *p);
uint32_t effective_prio(process_t *p);
void set_user_nice(process_t *p, long nice);
long task_nice(process_t *p);
void set_load_weight(process_t *p);

/**
 * Load balancing and statistics
 */
void update_cpu_load(runqueue_t *rq);
uint32_t nr_running(void);
uint32_t nr_uninterruptible(void);
void calc_load_accounting(process_t *p);

/**
 * Real-time scheduling
 */
int rt_task(process_t *p);
int task_has_rt_policy(process_t *p);
int rt_mutex_getprio(process_t *p);
void rt_mutex_setprio(process_t *p, int prio);

/**
 * Completely Fair Scheduler (CFS) inspired functions
 */
void update_curr(runqueue_t *rq, process_t *p);
uint64_t __sched_period(unsigned long nr_running);
uint64_t calc_delta_fair(uint64_t delta, process_t *p);
uint64_t __calc_delta(uint64_t delta_exec, uint64_t weight, struct load_weight *lw);

/**
 * Scheduler debugging and statistics
 */
void dump_runqueue(runqueue_t *rq);
void print_scheduler_stats(void);
void sched_debug_show(void);

/**
 * Idle process management
 */
process_t* get_idle_process(void);
void cpu_idle_loop(void);
void init_idle_process(void);

/**
 * SMP support preparation (for future enhancement)
 */
#ifdef CONFIG_SMP
void set_cpus_allowed(process_t *p, uint32_t new_mask);
uint32_t get_cpus_allowed(process_t *p);
void migrate_task(process_t *p, int dest_cpu);
#endif

/**
 * Scheduler configuration
 */
#define CONFIG_PREEMPT
#define CONFIG_FAIR_GROUP_SCHED
#define CONFIG_RT_GROUP_SCHED

/**
 * Helper macros
 */
#define TASK_RUNNING            0
#define TASK_INTERRUPTIBLE      1
#define TASK_UNINTERRUPTIBLE    2
#define TASK_STOPPED            4
#define TASK_TRACED             8

#define task_is_running(p)      ((p)->pcb.state == TASK_RUNNING)
#define task_is_stopped(p)      ((p)->pcb.state == TASK_STOPPED)
#define task_is_traced(p)       ((p)->pcb.state == TASK_TRACED)

#define list_for_each_entry(pos, head, member) \
    for (pos = list_entry((head)->next, typeof(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, typeof(*pos), member))

#endif
