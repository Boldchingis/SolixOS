#ifndef SOLIX_IRQ_H
#define SOLIX_IRQ_H

#include "types.h"
#include "kernel.h"

/**
 * Linux-Inspired IRQ Subsystem for SolixOS
 * Comprehensive interrupt management with proper IRQ handling
 * Based on Linux IRQ design principles
 */

// IRQ definitions
#define NR_IRQS           256     // Maximum number of IRQs
#define IRQ_BITMAP_BITS   NR_IRQS

// IRQ types
#define IRQ_TYPE_NONE     0x00000000
#define IRQ_TYPE_EDGE_RISING  0x00000001
#define IRQ_TYPE_EDGE_FALLING 0x00000002
#define IRQ_TYPE_EDGE_BOTH (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING)
#define IRQ_TYPE_LEVEL_HIGH  0x00000004
#define IRQ_TYPE_LEVEL_LOW   0x00000008

// IRQ flags
#define IRQ_DISABLED      0x01
#define IRQ_PENDING       0x02
#define IRQ_MASKED        0x04
#define IRQ_INPROGRESS    0x08
#define IRQ_REPLAY        0x10
#define IRQ_WAITING       0x20
#define IRQ_AUTODETECT    0x40
#define IRQ_SPURIOUS      0x80

// IRQ trigger types
#define IRQ_TRIGGER_NONE      0x000
#define IRQ_TRIGGER_RISING   0x001
#define IRQ_TRIGGER_FALLING  0x002
#define IRQ_TRIGGER_HIGH     0x004
#define IRQ_TRIGGER_LOW      0x008
#define IRQ_TRIGGER_MASK     (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING | \
                              IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW)

// IRQ affinity masks
#define IRQ_AFFINITY_DEFAULT 0xFFFFFFFF

// IRQ statistics
struct irq_desc_stats {
    unsigned int irqs;          // Total interrupts
    unsigned int spurious;      // Spurious interrupts
    unsigned int unhandled;     // Unhandled interrupts
    unsigned int retriggered;   // Retriggered interrupts
    unsigned int missed;        // Missed interrupts
};

// IRQ descriptor structure
struct irq_desc {
    // IRQ management
    unsigned int irq;           // IRQ number
    unsigned int status;        // IRQ status flags
    unsigned int depth;         // Nested disable depth
    unsigned int wake_depth;    // Wake-up depth
    unsigned int irq_count;     // Number of times IRQ was triggered
    unsigned int irqs_unhandled; // Number of unhandled interrupts
    
    // IRQ handling
    irq_handler_t handle_irq;   // High-level IRQ handler
    void *handler_data;        // Handler-specific data
    void *chip_data;           // Chip-specific data
    
    // Low-level operations
    struct irq_chip *chip;      // Low-level interrupt controller
    
    // IRQ flow control
    struct irq_flow_control *flow_control;
    
    // IRQ affinity
    unsigned int affinity;      // CPU affinity mask
    
    // Threaded IRQ support
    struct task_struct *thread; // IRQ thread
    unsigned int thread_flags;  // Thread flags
    
    // Statistics
    struct irq_desc_stats stats;
    
    // Locking
    spinlock_t lock;
    
    // Name for debugging
    const char *name;
    
    // Device association
    struct device *dev;
    
    // Power management
    unsigned int wake;         // Wake-up capability
    
    // IRQ domain support
    struct irq_domain *domain;
    unsigned int hwirq;        // Hardware IRQ number
};

// IRQ chip structure (interrupt controller)
struct irq_chip {
    const char *name;           // Chip name
    
    // Low-level operations
    void (*startup)(unsigned int irq);
    void (*shutdown)(unsigned int irq);
    void (*enable)(unsigned int irq);
    void (*disable)(unsigned int irq);
    void (*ack)(unsigned int irq);
    void (*mask)(unsigned int irq);
    void (*unmask)(unsigned int irq);
    void (*eoi)(unsigned int irq);
    
    // Set IRQ type
    int (*set_type)(unsigned int irq, unsigned int type);
    
    // Set IRQ affinity
    int (*set_affinity)(unsigned int irq, const struct cpumask *dest);
    
    // Re-trigger
    void (*retrigger)(unsigned int irq);
    
    // Set wake-up
    int (*set_wake)(unsigned int irq, unsigned int on);
    
    // IRQ domain operations
    int (*irq_domain_translate)(struct irq_domain *d, struct irq_fwspec *fwspec,
                               unsigned long *hwirq, unsigned int *type);
    int (*irq_domain_alloc)(struct irq_domain *d, unsigned int virq,
                           unsigned int nr_irqs, void *arg);
    void (*irq_domain_free)(struct irq_domain *d, unsigned int virq,
                           unsigned int nr_irqs);
    
    // Debug support
    void (*print_chip)(struct irq_desc *desc);
    
    // Power management
    int (*suspend)(struct irq_desc *desc);
    int (*resume)(struct irq_desc *desc);
};

// IRQ flow control structure
struct irq_flow_control {
    // Type-specific flow control
    const char *typename;
    
    // Flow control operations
    void (*handle)(struct irq_desc *desc);
    void (*eoi)(struct irq_desc *desc);
    void (*enable)(struct irq_desc *desc);
    void (*disable)(struct irq_desc *desc);
    void (*ack)(struct irq_desc *desc);
    void (*mask)(struct irq_desc *desc);
    void (*unmask)(struct irq_desc *desc);
    void (*set_affinity)(struct irq_desc *desc, const struct cpumask *dest);
    
    // Type-specific operations
    int (*set_type)(struct irq_desc *desc, unsigned int type);
    int (*retrigger)(struct irq_desc *desc);
};

// IRQ handler function type
typedef void (*irq_handler_t)(unsigned int irq, void *dev_id);

// IRQ action structure
struct irqaction {
    irq_handler_t handler;     // IRQ handler function
    void *dev_id;              // Device ID
    struct irqaction *next;     // Next action in chain
    unsigned int irq;          // IRQ number
    unsigned long flags;        // Flags
    const char *name;           // Action name
    struct device *dev;         // Device
};

// IRQ domain structure (for hierarchical IRQ management)
struct irq_domain {
    struct list_head link;      // Link to domain list
    const char *name;           // Domain name
    struct irq_domain_ops *ops; // Domain operations
    void *host_data;           // Host-specific data
    
    // Hardware IRQ range
    unsigned int hwirq_base;    // Base hardware IRQ
    unsigned int hwirq_max;     // Maximum hardware IRQ
    
    // Virtual IRQ range
    unsigned int revmap_size;   // Reverse map size
    unsigned int *revmap;       // Hardware to virtual IRQ mapping
    
    // Parent domain (for hierarchical domains)
    struct irq_domain *parent;
    
    // Domain flags
    unsigned int flags;
    
    // Device tree support
    struct device_node *of_node;
};

// IRQ domain operations
struct irq_domain_ops {
    // Translation
    int (*translate)(struct irq_domain *d, struct irq_fwspec *fwspec,
                    unsigned long *hwirq, unsigned int *type);
    
    // Allocation
    int (*alloc)(struct irq_domain *d, unsigned int virq,
                unsigned int nr_irqs, void *arg);
    void (*free)(struct irq_domain *d, unsigned int virq,
                unsigned int nr_irqs);
    
    // Mapping
    unsigned int (*map)(struct irq_domain *d, unsigned int hwirq);
    void (*unmap)(struct irq_domain *d, unsigned int virq);
    
    // Xlate (device tree)
    int (*xlate)(struct irq_domain *d, struct device_node *node,
                const u32 *intspec, unsigned int intsize,
                unsigned long *hwirq, unsigned int *type);
};

// IRQ management functions
extern int irq_set_chip(unsigned int irq, struct irq_chip *chip);
extern int irq_set_chip_data(unsigned int irq, void *data);
extern int irq_set_handler(unsigned int irq, irq_handler_t handler);
extern int irq_set_handler_data(unsigned int irq, void *data);
extern int irq_set_irq_type(unsigned int irq, unsigned int type);

// IRQ enable/disable
extern void irq_enable(unsigned int irq);
extern void irq_disable(unsigned int irq);
extern void irq_mask(unsigned int irq);
extern void irq_unmask(unsigned int irq);
extern void irq_ack(unsigned int irq);
extern void irq_eoi(unsigned int irq);

// IRQ request/free
extern int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
                      const char *name, void *dev);
extern void free_irq(unsigned int irq, void *dev_id);

// IRQ affinity
extern int irq_set_affinity(unsigned int irq, const struct cpumask *mask);
extern const struct cpumask *irq_get_affinity(unsigned int irq);

// IRQ statistics
extern void irq_stat_add(unsigned int irq);
extern void irq_stat_spurious(unsigned int irq);
extern void irq_stat_unhandled(unsigned int irq);
extern void irq_print_stats(unsigned int irq);

// IRQ domain management
extern struct irq_domain *irq_domain_add_linear(struct device_node *of_node,
                                                unsigned int size,
                                                struct irq_domain_ops *ops,
                                                void *host_data);
extern void irq_domain_remove(struct irq_domain *domain);
extern unsigned int irq_create_mapping(struct irq_domain *domain, unsigned int hwirq);
extern void irq_dispose_mapping(unsigned int virq);

// IRQ flow control
extern void irq_set_flow_handler(unsigned int irq, struct irq_flow_control *flow);
extern void handle_edge_irq(struct irq_desc *desc);
extern void handle_level_irq(struct irq_desc *desc);
extern void handle_simple_irq(struct irq_desc *desc);
extern void handle_percpu_irq(struct irq_desc *desc);

// Built-in flow control types
extern struct irq_flow_control edge_flow_control;
extern struct irq_flow_control level_flow_control;
extern struct irq_flow_control simple_flow_control;
extern struct irq_flow_control percpu_flow_control;

// IRQ initialization
extern void irq_init(void);
extern void early_irq_init(void);
extern void late_irq_init(void);

// IRQ debugging
extern void irq_debug_show(void);
extern void irq_debug_show_desc(struct irq_desc *desc);
extern void irq_debug_show_chip(struct irq_chip *chip);

// Power management
extern int irq_pm_suspend(void);
extern int irq_pm_resume(void);
extern int irq_set_wake(unsigned int irq, unsigned int on);

// IRQ testing and validation
extern int irq_test_irq(unsigned int irq);
extern void irq_self_test(void);

// Common IRQ chips
extern struct irq_chip dummy_irq_chip;
extern struct irq_chip generic_irq_chip;

// Helper macros
#define IRQ_TO_DESC(irq) (&irq_desc[irq])
#define irq_data_get_irq_chip(d) ((d)->chip)
#define irq_data_get_irq_chip_data(d) ((d)->chip_data)
#define irq_data_get_irq_handler_data(d) ((d)->handler_data)

#define for_each_irq_desc(irq, desc) \
    for (irq = 0, desc = irq_desc; irq < NR_IRQS; irq++, desc++)

#define irq_set_status_bit(irq, bit) \
    do { \
        struct irq_desc *d = IRQ_TO_DESC(irq); \
        unsigned long flags; \
        spin_lock_irqsave(&d->lock, flags); \
        d->status |= (bit); \
        spin_unlock_irqrestore(&d->lock, flags); \
    } while (0)

#define irq_clear_status_bit(irq, bit) \
    do { \
        struct irq_desc *d = IRQ_TO_DESC(irq); \
        unsigned long flags; \
        spin_lock_irqsave(&d->lock, flags); \
        d->status &= ~(bit); \
        spin_unlock_irqrestore(&d->lock, flags); \
    } while (0)

// IRQ flags for request_irq
#define IRQF_SHARED        0x00000001
#define IRQF_PROBE_SHARED  0x00000002
#define IRQF_TIMER         0x00000004
#define IRQF_PERCPU        0x00000008
#define IRQF_ONESHOT       0x00000020
#define IRQF_NO_SUSPEND    0x00004000
#define IRQF_FORCE_RESUME  0x00008000
#define IRQF_NO_THREAD     0x00010000
#define IRQF_EARLY_RESUME  0x00020000
#define IRQF_TRIGGER_NONE  0x00000000
#define IRQF_TRIGGER_RISING  0x00000001
#define IRQF_TRIGGER_FALLING 0x00000002
#define IRQF_TRIGGER_HIGH   0x00000004
#define IRQF_TRIGGER_LOW    0x00000008
#define IRQF_TRIGGER_MASK   (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW | \
                             IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING)
#define IRQF_TRIGGER_PROBE  0x00000010

// External IRQ descriptor array
extern struct irq_desc irq_desc[NR_IRQS];

// External declarations
extern void do_IRQ(unsigned int irq, struct pt_regs *regs);
extern void handle_irq(struct irq_desc *desc);

#endif
