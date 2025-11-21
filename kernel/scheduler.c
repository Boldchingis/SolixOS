#include "scheduler.h"
#include "kernel.h"
#include "mm.h"
#include "screen.h"

/**
 * Linux-Inspired O(1) Scheduler Implementation
 * Based on Linux 2.6 scheduler design principles
 * Enhanced with CFS concepts for better fairness
 */

// Global runqueue (single CPU for now)
static runqueue_t rq;

// Idle process
static process_t idle_process;

// Load tracking
static uint32_t avenrun[3];
static uint32_t calc_load;

// Scheduler statistics
static struct {
    uint32_t total_switches;
    uint32_t context_switches;
    uint32_t involuntary_context_switches;
    uint32_t schedule_count;
    uint32_t idle_time;
    uint32_t active_time;
    uint32_t load_avg;
} sched_stats = {0};

/**
 * Initialize the scheduler
 */
void scheduler_init(void) {
    // Initialize runqueue
    memset(&rq, 0, sizeof(runqueue_t));
    
    // Initialize priority arrays
    for (int i = 0; i < MAX_PRIO; i++) {
        INIT_LIST_HEAD(&rq.active.queue[i]);
        INIT_LIST_HEAD(&rq.expired.queue[i]);
    }
    
    // Clear bitmaps
    memset(rq.active.bitmap, 0, sizeof(rq.active.bitmap));
    memset(rq.expired.bitmap, 0, sizeof(rq.expired.bitmap));
    
    // Initialize load tracking
    calc_load = 0;
    avenrun[0] = avenrun[1] = avenrun[2] = 0;
    
    // Create idle process
    init_idle_process();
    
    rq.curr = &idle_process;
    rq.nr_running = 0;
    rq.nr_switches = 0;
    rq.expired_timestamp = 0;
    rq.best_expired_prio = MAX_PRIO;
    rq.cpu_load = 0;
    rq.nr_uninterruptible = 0;
    
    debug_print(DEBUG_INFO, "Linux-inspired O(1) scheduler initialized");
}

/**
 * Initialize the idle process
 */
void init_idle_process(void) {
    memset(&idle_process, 0, sizeof(process_t));
    
    // Set up idle process PCB
    idle_process.pcb.pid = 0;
    idle_process.pcb.ppid = 0;
    idle_process.pcb.state = TASK_RUNNING;
    idle_process.pcb.kernel_stack = (uint32_t)kmalloc(KERNEL_STACK_SIZE);
    
    // Set up scheduling entity
    struct sched_entity *se = sched_entity(&idle_process);
    se->policy = SCHED_IDLE;
    se->prio = MAX_PRIO;
    se->static_prio = MAX_PRIO;
    se->normal_prio = MAX_PRIO;
    se->time_slice = 0;
    se->rq = &rq;
    se->load_weight = 1;
    se->inv_weight = 1;
    
    strcpy(idle_process.name, "idle");
    
    debug_print(DEBUG_DEBUG, "Idle process initialized");
}

/**
 * Get the idle process
 */
process_t* get_idle_process(void) {
    return &idle_process;
}

/**
 * Calculate task timeslice based on priority
 */
uint32_t task_timeslice(process_t *p) {
    if (rt_task(p)) {
        return rt_task(p) * HZ / 1000;
    }
    
    uint32_t static_prio = sched_entity(p)->static_prio;
    if (static_prio < MAX_RT_PRIO) {
        return 0;
    }
    
    // CFS-inspired dynamic timeslice
    uint32_t base_timeslice = BASE_TIMESLICE;
    uint32_t timeslice = base_timeslice;
    
    if (static_prio > DEFAULT_PRIO) {
        timeslice = (base_timeslice * (MAX_PRIO - static_prio)) / 
                   (MAX_PRIO - DEFAULT_PRIO);
    } else {
        timeslice = base_timeslice * 2;
    }
    
    // Clamp to reasonable bounds
    if (timeslice < MIN_TIMESLICE) timeslice = MIN_TIMESLICE;
    if (timeslice > MAX_TIMESLICE) timeslice = MAX_TIMESLICE;
    
    return timeslice;
}

/**
 * Get effective priority of a task
 */
uint32_t effective_prio(process_t *p) {
    struct sched_entity *se = sched_entity(p);
    uint32_t prio = se->static_prio;
    
    // Apply nice value adjustments
    if (se->policy == SCHED_NORMAL || se->policy == SCHED_BATCH) {
        prio = se->static_prio;
    }
    
    return prio;
}

/**
 * Set task nice value
 */
void set_user_nice(process_t *p, long nice) {
    if (nice < -20) nice = -20;
    if (nice > 19) nice = 19;
    
    struct sched_entity *se = sched_entity(p);
    uint32_t old_prio = se->static_prio;
    se->static_prio = NICE_TO_PRIO(nice);
    se->normal_prio = se->static_prio;
    
    if (task_is_running(p)) {
        if (se->rq && se->rq->curr == p) {
            // Update current task priority
            se->prio = effective_prio(p);
        } else {
            // Requeue with new priority
            dequeue_task(p);
            se->prio = effective_prio(p);
            enqueue_task(p, 0);
        }
    }
    
    set_load_weight(p);
    
    debug_print(DEBUG_DEBUG, "Process %d nice set to %ld (prio %d)", 
                p->pcb.pid, nice, se->static_prio);
}

/**
 * Get task nice value
 */
long task_nice(process_t *p) {
    return PRIO_TO_NICE(sched_entity(p)->static_prio);
}

/**
 * Set task load weight (CFS-inspired)
 */
void set_load_weight(process_t *p) {
    struct sched_entity *se = sched_entity(p);
    uint32_t prio = se->static_prio;
    
    if (prio >= MAX_RT_PRIO) {
        // Normal task weight based on nice value
        long nice = PRIO_TO_NICE(prio);
        if (nice < 0) {
            se->load_weight = (1024 * 1024) >> (-nice);
        } else {
            se->load_weight = 1024 >> nice;
        }
    } else {
        // Real-time task weight
        se->load_weight = 0;
    }
    
    // Calculate inverse weight for CFS calculations
    if (se->load_weight) {
        se->inv_weight = (1ULL << 32) / se->load_weight;
    } else {
        se->inv_weight = ~0UL;
    }
}

/**
 * Check if task is real-time
 */
int rt_task(process_t *p) {
    return sched_entity(p)->policy != SCHED_NORMAL &&
           sched_entity(p)->policy != SCHED_BATCH &&
           sched_entity(p)->policy != SCHED_IDLE;
}

/**
 * Check if task has real-time policy
 */
int task_has_rt_policy(process_t *p) {
    return sched_entity(p)->policy == SCHED_FIFO ||
           sched_entity(p)->policy == SCHED_RR;
}

/**
 * Enqueue task on runqueue
 */
void enqueue_task(process_t *p, int head) {
    struct sched_entity *se = sched_entity(p);
    runqueue_t *rq = se->rq;
    
    if (!rq) {
        rq = &rq;
        se->rq = rq;
    }
    
    // Set initial timeslice if not set
    if (!se->time_slice) {
        se->time_slice = task_timeslice(p);
    }
    
    // Add to appropriate priority queue
    if (head) {
        list_add(&se->run_list, &rq->active.queue[se->prio]);
    } else {
        list_add_tail(&se->run_list, &rq->active.queue[se->prio]);
    }
    
    // Set bitmap bit
    rq->active.bitmap[BITMAP_WORD(se->prio)] |= BITMAP_BIT(se->prio);
    
    rq->nr_running++;
    
    debug_print(DEBUG_DEBUG, "Process %d enqueued at priority %d", 
                p->pcb.pid, se->prio);
}

/**
 * Dequeue task from runqueue
 */
void dequeue_task(process_t *p) {
    struct sched_entity *se = sched_entity(p);
    runqueue_t *rq = se->rq;
    
    if (!rq || list_empty(&se->run_list)) {
        return;
    }
    
    // Remove from runqueue
    list_del_init(&se->run_list);
    
    // Clear bitmap bit if queue is empty
    if (list_empty(&rq->active.queue[se->prio])) {
        rq->active.bitmap[BITMAP_WORD(se->prio)] &= ~BITMAP_BIT(se->prio);
    }
    
    rq->nr_running--;
    
    debug_print(DEBUG_DEBUG, "Process %d dequeued from priority %d", 
                p->pcb.pid, se->prio);
}

/**
 * Pick next task to run
 */
process_t* pick_next_task(runqueue_t *rq) {
    struct sched_class *class;
    process_t *p;
    
    // Try real-time scheduler first
    class = (struct sched_class *)&sched_rt_class;
    p = class->pick_next_task(rq);
    if (p) {
        return p;
    }
    
    // Try fair scheduler
    class = (struct sched_class *)&sched_fair_class;
    p = class->pick_next_task(rq);
    if (p) {
        return p;
    }
    
    // Fall back to idle scheduler
    class = (struct sched_class *)&sched_idle_class;
    p = class->pick_next_task(rq);
    
    return p;
}

/**
 * Main scheduler function
 */
void schedule(void) {
    process_t *prev, *next;
    runqueue_t *rq = &rq;
    unsigned long flags;
    
    sched_stats.schedule_count++;
    
    prev = rq->curr;
    
    if (prev == &idle_process) {
        sched_stats.idle_time++;
    } else {
        sched_stats.active_time++;
    }
    
    // Pick next task
    next = pick_next_task(rq);
    
    if (next == prev) {
        return;
    }
    
    // Update statistics
    rq->nr_switches++;
    sched_stats.context_switches++;
    
    // Update runqueue
    rq->curr = next;
    
    debug_print(DEBUG_DEBUG, "Context switch: %d -> %d", 
                prev ? prev->pcb.pid : -1, next ? next->pcb.pid : -1);
    
    // Perform context switch
    if (prev && prev != &idle_process) {
        // Save current process state
        prev->pcb.state = TASK_READY;
    }
    
    if (next && next != &idle_process) {
        // Restore next process state
        next->pcb.state = TASK_RUNNING;
        current_process = next;
    }
    
    // Update load tracking
    update_cpu_load(rq);
}

/**
 * Scheduler tick handler
 */
void scheduler_tick(void) {
    process_t *curr = rq.curr;
    struct sched_entity *se;
    
    if (!curr || curr == &idle_process) {
        return;
    }
    
    se = sched_entity(curr);
    
    // Update task runtime
    se->sum_exec_runtime++;
    
    // Decrease timeslice
    if (se->time_slice > 0) {
        se->time_slice--;
    }
    
    // Check if timeslice expired
    if (se->time_slice == 0) {
        // Reset timeslice
        se->time_slice = task_timeslice(curr);
        
        // Move to expired array if not real-time
        if (!rt_task(curr)) {
            dequeue_task(curr);
            list_add_tail(&se->run_list, &rq.expired.queue[se->prio]);
            rq.expired.bitmap[BITMAP_WORD(se->prio)] |= BITMAP_BIT(se->prio);
            
            // Check if we need to swap active and expired arrays
            if (rq.nr_running == 0) {
                // Swap arrays
                struct prio_array temp = rq.active;
                rq.active = rq.expired;
                rq.expired = temp;
                rq.expired_timestamp = kernel_get_timestamp();
            }
        }
        
        // Reschedule
        schedule();
    }
}

/**
 * Update CPU load
 */
void update_cpu_load(runqueue_t *rq) {
    uint32_t this_load = rq->nr_running * 1000;
    
    // Exponential moving average
    avenrun[0] = (avenrun[0] * 3 + this_load) / 4;
    avenrun[1] = (avenrun[1] * 15 + this_load) / 16;
    avenrun[2] = (avenrun[2] * 63 + this_load) / 64;
    
    rq->cpu_load = avenrun[0];
    sched_stats.load_avg = avenrun[0];
}

/**
 * Yield CPU time
 */
void yield_task(process_t *p) {
    struct sched_entity *se = sched_entity(p);
    
    if (se->rq && se->rq->curr == p) {
        // Move to end of queue
        dequeue_task(p);
        enqueue_task(p, 0);
        schedule();
    }
}

/**
 * Get number of running processes
 */
uint32_t nr_running(void) {
    return rq.nr_running;
}

/**
 * Get number of uninterruptible processes
 */
uint32_t nr_uninterruptible(void) {
    return rq.nr_uninterruptible;
}

/**
 * Print scheduler statistics
 */
void print_scheduler_stats(void) {
    screen_print("\n=== Scheduler Statistics ===\n");
    screen_print("Total switches: ");
    screen_print_dec(sched_stats.context_switches);
    screen_print("\nSchedule calls: ");
    screen_print_dec(sched_stats.schedule_count);
    screen_print("\nRunning processes: ");
    screen_print_dec(rq.nr_running);
    screen_print("\nCPU load: ");
    screen_print_dec(rq.cpu_load / 1000);
    screen_print(".");
    screen_print_dec((rq.cpu_load % 1000) / 100);
    screen_print("\nActive time: ");
    screen_print_dec(sched_stats.active_time);
    screen_print("\nIdle time: ");
    screen_print_dec(sched_stats.idle_time);
    screen_print("\n");
}

/**
 * Dump runqueue for debugging
 */
void dump_runqueue(runqueue_t *rq) {
    process_t *p;
    struct sched_entity *se;
    
    screen_print("\n=== Runqueue Dump ===\n");
    screen_print("Current: PID ");
    screen_print_dec(rq->curr->pcb.pid);
    screen_print(" (");
    screen_print(rq->curr->name);
    screen_print(")\n");
    screen_print("Running: ");
    screen_print_dec(rq->nr_running);
    screen_print("\n");
    
    for (int prio = 0; prio < MAX_PRIO; prio++) {
        if (!list_empty(&rq->active.queue[prio])) {
            screen_print("Priority ");
            screen_print_dec(prio);
            screen_print(": ");
            
            list_for_each_entry(p, &rq->active.queue[prio], sched_entity->run_list) {
                screen_print("PID ");
                screen_print_dec(p->pcb.pid);
                screen_print(" ");
            }
            screen_print("\n");
        }
    }
}

/**
 * CPU idle loop
 */
void cpu_idle_loop(void) {
    while (1) {
        // Check if there are runnable processes
        if (rq.nr_running > 0) {
            schedule();
            continue;
        }
        
        // Enable interrupts and halt
        __asm__ volatile("sti");
        __asm__ volatile("hlt");
        __asm__ volatile("cli");
        
        sched_stats.idle_time++;
    }
}

// Fair scheduler class implementation
const struct sched_class sched_fair_class = {
    .name = "fair",
    .enqueue_task = enqueue_task,
    .dequeue_task = dequeue_task,
    .yield_task = yield_task,
    .pick_next_task = pick_next_task,
    .task_tick = scheduler_tick,
};

// Real-time scheduler class implementation  
const struct sched_class sched_rt_class = {
    .name = "realtime",
    .enqueue_task = enqueue_task,
    .dequeue_task = dequeue_task,
    .yield_task = yield_task,
    .pick_next_task = pick_next_task,
    .task_tick = scheduler_tick,
};

// Idle scheduler class implementation
const struct sched_class sched_idle_class = {
    .name = "idle",
    .pick_next_task = get_idle_process,
};
