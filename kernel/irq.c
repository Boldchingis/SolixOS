#include "irq.h"
#include "kernel.h"
#include "mm.h"
#include "printk.h"
#include "slab.h"

/**
 * Linux-Inspired IRQ Subsystem Implementation
 * Comprehensive interrupt management with proper IRQ handling
 * Based on Linux IRQ design principles
 */

// Global IRQ descriptor array
struct irq_desc irq_desc[NR_IRQS];

// IRQ statistics
static struct {
    unsigned int total_irqs;
    unsigned int spurious_irqs;
    unsigned int unhandled_irqs;
    unsigned int disabled_irqs;
    unsigned int masked_irqs;
} irq_stats = {0};

// IRQ domain list
static LIST_HEAD(irq_domain_list);
static spinlock_t irq_domain_lock = SPIN_LOCK_UNLOCKED;

// Dummy IRQ chip for unassigned IRQs
struct irq_chip dummy_irq_chip = {
    .name = "dummy",
    .startup = NULL,
    .shutdown = NULL,
    .enable = NULL,
    .disable = NULL,
    .ack = NULL,
    .mask = NULL,
    .unmask = NULL,
    .eoi = NULL,
    .set_type = NULL,
    .set_affinity = NULL,
    .retrigger = NULL,
    .set_wake = NULL,
};

// Generic IRQ chip
struct irq_chip generic_irq_chip = {
    .name = "generic",
    .startup = generic_irq_startup,
    .shutdown = generic_irq_shutdown,
    .enable = generic_irq_enable,
    .disable = generic_irq_disable,
    .ack = generic_irq_ack,
    .mask = generic_irq_mask,
    .unmask = generic_irq_unmask,
    .eoi = generic_irq_eoi,
    .set_type = generic_irq_set_type,
    .set_affinity = generic_irq_set_affinity,
    .retrigger = generic_irq_retrigger,
    .set_wake = generic_irq_set_wake,
};

// Edge flow control
struct irq_flow_control edge_flow_control = {
    .typename = "edge",
    .handle = handle_edge_irq,
    .eoi = edge_irq_eoi,
    .enable = edge_irq_enable,
    .disable = edge_irq_disable,
    .ack = edge_irq_ack,
    .mask = edge_irq_mask,
    .unmask = edge_irq_unmask,
    .set_affinity = edge_irq_set_affinity,
    .set_type = edge_irq_set_type,
    .retrigger = edge_irq_retrigger,
};

// Level flow control
struct irq_flow_control level_flow_control = {
    .typename = "level",
    .handle = handle_level_irq,
    .eoi = level_irq_eoi,
    .enable = level_irq_enable,
    .disable = level_irq_disable,
    .ack = level_irq_ack,
    .mask = level_irq_mask,
    .unmask = level_irq_unmask,
    .set_affinity = level_irq_set_affinity,
    .set_type = level_irq_set_type,
    .retrigger = level_irq_retrigger,
};

// Simple flow control
struct irq_flow_control simple_flow_control = {
    .typename = "simple",
    .handle = handle_simple_irq,
    .eoi = simple_irq_eoi,
    .enable = simple_irq_enable,
    .disable = simple_irq_disable,
    .ack = simple_irq_ack,
    .mask = simple_irq_mask,
    .unmask = simple_irq_unmask,
    .set_affinity = simple_irq_set_affinity,
    .set_type = simple_irq_set_type,
    .retrigger = simple_irq_retrigger,
};

// Per-CPU flow control
struct irq_flow_control percpu_flow_control = {
    .typename = "percpu",
    .handle = handle_percpu_irq,
    .eoi = percpu_irq_eoi,
    .enable = percpu_irq_enable,
    .disable = percpu_irq_disable,
    .ack = percpu_irq_ack,
    .mask = percpu_irq_mask,
    .unmask = percpu_irq_unmask,
    .set_affinity = percpu_irq_set_affinity,
    .set_type = percpu_irq_set_type,
    .retrigger = percpu_irq_retrigger,
};

/**
 * Initialize IRQ subsystem
 */
void irq_init(void) {
    pr_info("Initializing Linux-inspired IRQ subsystem\n");
    
    // Initialize all IRQ descriptors
    for (int i = 0; i < NR_IRQS; i++) {
        struct irq_desc *desc = &irq_desc[i];
        
        memset(desc, 0, sizeof(struct irq_desc));
        desc->irq = i;
        desc->status = IRQ_DISABLED;
        desc->depth = 1; // Start disabled
        desc->chip = &dummy_irq_chip;
        desc->handle_irq = NULL;
        desc->handler_data = NULL;
        desc->chip_data = NULL;
        desc->flow_control = NULL;
        desc->affinity = IRQ_AFFINITY_DEFAULT;
        desc->thread = NULL;
        desc->thread_flags = 0;
        desc->name = "unknown";
        desc->dev = NULL;
        desc->wake = 0;
        desc->domain = NULL;
        desc->hwirq = i;
        
        spin_lock_init(&desc->lock);
        
        // Initialize statistics
        memset(&desc->stats, 0, sizeof(struct irq_desc_stats));
    }
    
    pr_info("IRQ subsystem initialized with %d IRQ descriptors\n", NR_IRQS);
}

/**
 * Early IRQ initialization
 */
void early_irq_init(void) {
    // Early initialization for critical IRQs
    // This is called before memory management is fully up
    
    pr_debug("Early IRQ initialization\n");
    
    // Initialize critical IRQ descriptors (timer, keyboard, etc.)
    // These are needed for early boot
}

/**
 * Late IRQ initialization
 */
void late_irq_init(void) {
    // Late initialization after memory management is up
    // This can allocate memory and set up more complex structures
    
    pr_debug("Late IRQ initialization\n");
    
    // Set up default flow control for common IRQ types
    // Initialize IRQ domains
    // Set up interrupt controllers
}

/**
 * Set IRQ chip
 */
int irq_set_chip(unsigned int irq, struct irq_chip *chip) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return -EINVAL;
    if (!chip) return -EINVAL;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    desc->chip = chip;
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: set chip '%s'\n", irq, chip->name ? chip->name : "unknown");
    
    return 0;
}

/**
 * Set IRQ chip data
 */
int irq_set_chip_data(unsigned int irq, void *data) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return -EINVAL;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    desc->chip_data = data;
    spin_unlock_irqrestore(&desc->lock, flags);
    
    return 0;
}

/**
 * Set IRQ handler
 */
int irq_set_handler(unsigned int irq, irq_handler_t handler) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return -EINVAL;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    desc->handle_irq = handler;
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: set handler %p\n", irq, handler);
    
    return 0;
}

/**
 * Set IRQ handler data
 */
int irq_set_handler_data(unsigned int irq, void *data) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return -EINVAL;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    desc->handler_data = data;
    spin_unlock_irqrestore(&desc->lock, flags);
    
    return 0;
}

/**
 * Set IRQ flow control
 */
void irq_set_flow_handler(unsigned int irq, struct irq_flow_control *flow) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    desc->flow_control = flow;
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: set flow control '%s'\n", irq, flow ? flow->typename : "none");
}

/**
 * Enable IRQ
 */
void irq_enable(unsigned int irq) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    if (desc->depth == 0) {
        spin_unlock_irqrestore(&desc->lock, flags);
        return; // Already enabled
    }
    
    if (--desc->depth == 0) {
        desc->status &= ~IRQ_DISABLED;
        
        if (desc->chip && desc->chip->enable) {
            desc->chip->enable(irq);
        }
        
        if (desc->flow_control && desc->flow_control->enable) {
            desc->flow_control->enable(desc);
        }
    }
    
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: enabled\n", irq);
}

/**
 * Disable IRQ
 */
void irq_disable(unsigned int irq) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    if (desc->depth == 0) {
        desc->status |= IRQ_DISABLED;
        
        if (desc->chip && desc->chip->disable) {
            desc->chip->disable(irq);
        }
        
        if (desc->flow_control && desc->flow_control->disable) {
            desc->flow_control->disable(desc);
        }
    }
    
    desc->depth++;
    
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: disabled (depth %d)\n", irq, desc->depth);
}

/**
 * Mask IRQ
 */
void irq_mask(unsigned int irq) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    desc->status |= IRQ_MASKED;
    
    if (desc->chip && desc->chip->mask) {
        desc->chip->mask(irq);
    }
    
    if (desc->flow_control && desc->flow_control->mask) {
        desc->flow_control->mask(desc);
    }
    
    irq_stats.masked_irqs++;
    
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: masked\n", irq);
}

/**
 * Unmask IRQ
 */
void irq_unmask(unsigned int irq) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    desc->status &= ~IRQ_MASKED;
    
    if (desc->chip && desc->chip->unmask) {
        desc->chip->unmask(irq);
    }
    
    if (desc->flow_control && desc->flow_control->unmask) {
        desc->flow_control->unmask(desc);
    }
    
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_debug("IRQ %d: unmasked\n", irq);
}

/**
 * Acknowledge IRQ
 */
void irq_ack(unsigned int irq) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    if (desc->chip && desc->chip->ack) {
        desc->chip->ack(irq);
    }
    
    if (desc->flow_control && desc->flow_control->ack) {
        desc->flow_control->ack(desc);
    }
    
    spin_unlock_irqrestore(&desc->lock, flags);
}

/**
 * End of interrupt
 */
void irq_eoi(unsigned int irq) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    if (desc->chip && desc->chip->eoi) {
        desc->chip->eoi(irq);
    }
    
    if (desc->flow_control && desc->flow_control->eoi) {
        desc->flow_control->eoi(desc);
    }
    
    spin_unlock_irqrestore(&desc->lock, flags);
}

/**
 * Request IRQ
 */
int request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags,
               const char *name, void *dev) {
    struct irq_desc *desc;
    unsigned long irq_flags;
    
    if (irq >= NR_IRQS) return -EINVAL;
    if (!handler) return -EINVAL;
    if (!name) name = "unknown";
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, irq_flags);
    
    // Check if IRQ is already in use (unless shared)
    if (!(flags & IRQF_SHARED) && desc->handle_irq) {
        spin_unlock_irqrestore(&desc->lock, irq_flags);
        return -EBUSY;
    }
    
    // Set handler and data
    desc->handle_irq = handler;
    desc->handler_data = dev;
    desc->name = name;
    
    // Enable IRQ if not already enabled
    if (desc->depth > 0) {
        desc->depth = 1;
        irq_enable(irq);
    }
    
    spin_unlock_irqrestore(&desc->lock, irq_flags);
    
    pr_info("IRQ %d: requested by %s\n", irq, name);
    
    return 0;
}

/**
 * Free IRQ
 */
void free_irq(unsigned int irq, void *dev_id) {
    struct irq_desc *desc;
    unsigned long flags;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    spin_lock_irqsave(&desc->lock, flags);
    
    // Clear handler and data
    desc->handle_irq = NULL;
    desc->handler_data = NULL;
    desc->name = "freed";
    
    // Disable IRQ
    irq_disable(irq);
    
    spin_unlock_irqrestore(&desc->lock, flags);
    
    pr_info("IRQ %d: freed\n", irq);
}

/**
 * Main IRQ handler
 */
void do_IRQ(unsigned int irq, struct pt_regs *regs) {
    struct irq_desc *desc;
    
    if (irq >= NR_IRQS) {
        pr_warn("Spurious IRQ %d\n", irq);
        irq_stats.spurious_irqs++;
        return;
    }
    
    desc = IRQ_TO_DESC(irq);
    
    // Update statistics
    desc->stats.irqs++;
    irq_stats.total_irqs++;
    
    // Check if IRQ is disabled
    if (desc->status & IRQ_DISABLED) {
        desc->stats.unhandled++;
        irq_stats.unhandled_irqs++;
        return;
    }
    
    // Mark IRQ as in progress
    desc->status |= IRQ_INPROGRESS;
    
    // Handle the IRQ
    if (desc->flow_control && desc->flow_control->handle) {
        desc->flow_control->handle(desc);
    } else if (desc->handle_irq) {
        desc->handle_irq(irq, desc->handler_data);
    } else {
        desc->stats.unhandled++;
        irq_stats.unhandled_irqs++;
        pr_warn("Unhandled IRQ %d\n", irq);
    }
    
    // Clear in progress flag
    desc->status &= ~IRQ_INPROGRESS;
}

/**
 * Handle edge-triggered IRQ
 */
void handle_edge_irq(struct irq_desc *desc) {
    unsigned int irq = desc->irq;
    
    // Acknowledge the IRQ
    irq_ack(irq);
    
    // Unmask the IRQ
    irq_unmask(irq);
    
    // Call the handler
    if (desc->handle_irq) {
        desc->handle_irq(irq, desc->handler_data);
    }
    
    // End of interrupt
    irq_eoi(irq);
}

/**
 * Handle level-triggered IRQ
 */
void handle_level_irq(struct irq_desc *desc) {
    unsigned int irq = desc->irq;
    
    // Mask the IRQ to prevent nesting
    irq_mask(irq);
    
    // Acknowledge the IRQ
    irq_ack(irq);
    
    // Call the handler
    if (desc->handle_irq) {
        desc->handle_irq(irq, desc->handler_data);
    }
    
    // End of interrupt
    irq_eoi(irq);
    
    // Unmask the IRQ
    irq_unmask(irq);
}

/**
 * Handle simple IRQ
 */
void handle_simple_irq(struct irq_desc *desc) {
    unsigned int irq = desc->irq;
    
    // Call the handler
    if (desc->handle_irq) {
        desc->handle_irq(irq, desc->handler_data);
    }
}

/**
 * Handle per-CPU IRQ
 */
void handle_percpu_irq(struct irq_desc *desc) {
    unsigned int irq = desc->irq;
    
    // Call the handler
    if (desc->handle_irq) {
        desc->handle_irq(irq, desc->handler_data);
    }
    
    // End of interrupt
    irq_eoi(irq);
}

// Generic IRQ chip implementations
void generic_irq_startup(unsigned int irq) {
    irq_enable(irq);
}

void generic_irq_shutdown(unsigned int irq) {
    irq_disable(irq);
}

void generic_irq_enable(unsigned int irq) {
    // Platform-specific enable
}

void generic_irq_disable(unsigned int irq) {
    // Platform-specific disable
}

void generic_irq_ack(unsigned int irq) {
    // Platform-specific acknowledge
}

void generic_irq_mask(unsigned int irq) {
    // Platform-specific mask
}

void generic_irq_unmask(unsigned int irq) {
    // Platform-specific unmask
}

void generic_irq_eoi(unsigned int irq) {
    // Platform-specific end of interrupt
}

int generic_irq_set_type(unsigned int irq, unsigned int type) {
    struct irq_desc *desc = IRQ_TO_DESC(irq);
    
    if (!desc->chip || !desc->chip->set_type) {
        return -ENOTSUPP;
    }
    
    return desc->chip->set_type(irq, type);
}

int generic_irq_set_affinity(unsigned int irq, const struct cpumask *dest) {
    struct irq_desc *desc = IRQ_TO_DESC(irq);
    
    if (!desc->chip || !desc->chip->set_affinity) {
        return -ENOTSUPP;
    }
    
    return desc->chip->set_affinity(irq, dest);
}

void generic_irq_retrigger(unsigned int irq) {
    struct irq_desc *desc = IRQ_TO_DESC(irq);
    
    if (desc->chip && desc->chip->retrigger) {
        desc->chip->retrigger(irq);
    }
}

int generic_irq_set_wake(unsigned int irq, unsigned int on) {
    struct irq_desc *desc = IRQ_TO_DESC(irq);
    
    if (!desc->chip || !desc->chip->set_wake) {
        return -ENOTSUPP;
    }
    
    return desc->chip->set_wake(irq, on);
}

// Flow control implementations (simplified)
void edge_irq_enable(struct irq_desc *desc) { irq_enable(desc->irq); }
void edge_irq_disable(struct irq_desc *desc) { irq_disable(desc->irq); }
void edge_irq_ack(struct irq_desc *desc) { irq_ack(desc->irq); }
void edge_irq_mask(struct irq_desc *desc) { irq_mask(desc->irq); }
void edge_irq_unmask(struct irq_desc *desc) { irq_unmask(desc->irq); }
void edge_irq_eoi(struct irq_desc *desc) { irq_eoi(desc->irq); }
int edge_irq_set_affinity(struct irq_desc *desc, const struct cpumask *dest) { return 0; }
int edge_irq_set_type(struct irq_desc *desc, unsigned int type) { return 0; }
void edge_irq_retrigger(struct irq_desc *desc) { generic_irq_retrigger(desc->irq); }

void level_irq_enable(struct irq_desc *desc) { irq_enable(desc->irq); }
void level_irq_disable(struct irq_desc *desc) { irq_disable(desc->irq); }
void level_irq_ack(struct irq_desc *desc) { irq_ack(desc->irq); }
void level_irq_mask(struct irq_desc *desc) { irq_mask(desc->irq); }
void level_irq_unmask(struct irq_desc *desc) { irq_unmask(desc->irq); }
void level_irq_eoi(struct irq_desc *desc) { irq_eoi(desc->irq); }
int level_irq_set_affinity(struct irq_desc *desc, const struct cpumask *dest) { return 0; }
int level_irq_set_type(struct irq_desc *desc, unsigned int type) { return 0; }
void level_irq_retrigger(struct irq_desc *desc) { generic_irq_retrigger(desc->irq); }

void simple_irq_enable(struct irq_desc *desc) { irq_enable(desc->irq); }
void simple_irq_disable(struct irq_desc *desc) { irq_disable(desc->irq); }
void simple_irq_ack(struct irq_desc *desc) { irq_ack(desc->irq); }
void simple_irq_mask(struct irq_desc *desc) { irq_mask(desc->irq); }
void simple_irq_unmask(struct irq_desc *desc) { irq_unmask(desc->irq); }
void simple_irq_eoi(struct irq_desc *desc) { irq_eoi(desc->irq); }
int simple_irq_set_affinity(struct irq_desc *desc, const struct cpumask *dest) { return 0; }
int simple_irq_set_type(struct irq_desc *desc, unsigned int type) { return 0; }
void simple_irq_retrigger(struct irq_desc *desc) { generic_irq_retrigger(desc->irq); }

void percpu_irq_enable(struct irq_desc *desc) { irq_enable(desc->irq); }
void percpu_irq_disable(struct irq_desc *desc) { irq_disable(desc->irq); }
void percpu_irq_ack(struct irq_desc *desc) { irq_ack(desc->irq); }
void percpu_irq_mask(struct irq_desc *desc) { irq_mask(desc->irq); }
void percpu_irq_unmask(struct irq_desc *desc) { irq_unmask(desc->irq); }
void percpu_irq_eoi(struct irq_desc *desc) { irq_eoi(desc->irq); }
int percpu_irq_set_affinity(struct irq_desc *desc, const struct cpumask *dest) { return 0; }
int percpu_irq_set_type(struct irq_desc *desc, unsigned int type) { return 0; }
void percpu_irq_retrigger(struct irq_desc *desc) { generic_irq_retrigger(desc->irq); }

/**
 * Print IRQ statistics
 */
void irq_print_stats(unsigned int irq) {
    struct irq_desc *desc;
    
    if (irq >= NR_IRQS) return;
    
    desc = IRQ_TO_DESC(irq);
    
    printk("IRQ %d (%s):\n", irq, desc->name);
    printk("  Total: %u\n", desc->stats.irqs);
    printk("  Spurious: %u\n", desc->stats.spurious);
    printk("  Unhandled: %u\n", desc->stats.unhandled);
    printk("  Retriggered: %u\n", desc->stats.retriggered);
    printk("  Missed: %u\n", desc->stats.missed);
}

/**
 * Debug show all IRQs
 */
void irq_debug_show(void) {
    printk("=== IRQ Statistics ===\n");
    printk("Total IRQs: %u\n", irq_stats.total_irqs);
    printk("Spurious IRQs: %u\n", irq_stats.spurious_irqs);
    printk("Unhandled IRQs: %u\n", irq_stats.unhandled_irqs);
    printk("Disabled IRQs: %u\n", irq_stats.disabled_irqs);
    printk("Masked IRQs: %u\n", irq_stats.masked_irqs);
    printk("\n");
    
    for (int i = 0; i < NR_IRQS; i++) {
        struct irq_desc *desc = &irq_desc[i];
        if (desc->stats.irqs > 0) {
            irq_print_stats(i);
        }
    }
}
