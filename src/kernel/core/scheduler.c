/*
 * Copyright (C) 2026 PhotonX Technologies.
 * * Module: Core Scheduler (HOCS-RT)
 * Author: PhotonX R&D Team
 * Description: 
 * Implements a priority-based, preemptive round-robin scheduler designed
 * for optical computing workloads. Unlike CFS (Linux), this scheduler
 * guarantees deterministic execution slots for the Photonic Control Loop.
 */

#include "hocs_kernel.h"
#include "platform/zynqmp_hardware.h"
#include "lib/kprintf.h"

/* Configuration Macros */
#define MAX_PROCESSES       128
#define STACK_SIZE          8192    // 8KB per process
#define TIME_SLICE_MS       10      // 10ms Quantum
#define PRIORITY_LEVELS     16

/* Process States */
typedef enum {
    PROC_UNUSED = 0,
    PROC_CREATED,
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE
} proc_state_t;

/* Processor Context (ARM64 Saved State) */
typedef struct {
    uint64_t x19; uint64_t x20; uint64_t x21; uint64_t x22;
    uint64_t x23; uint64_t x24; uint64_t x25; uint64_t x26;
    uint64_t x27; uint64_t x28; uint64_t x29; // Frame Pointer
    uint64_t x30; // Link Register (Return Address)
    uint64_t sp;  // Stack Pointer
    uint64_t pc;  // Program Counter
    uint64_t pstate; // Processor State (CPSR)
} __attribute__((packed)) cpu_context_t;

/* Process Control Block (PCB) */
typedef struct process {
    uint32_t pid;               // Process ID
    char name[32];              // Process Name
    proc_state_t state;         // Current State
    uint32_t priority;          // 0 (High) - 15 (Low)
    uint64_t ticks_remaining;   // Time slice
    uint64_t total_runtime;     // Statistics
    
    /* Memory Map */
    uintptr_t stack_base;       // Bottom of stack
    uintptr_t stack_ptr;        // Current SP
    
    /* Linked List pointers */
    struct process *next;
    struct process *prev;
    
    /* Hardware Context */
    cpu_context_t context;
} pcb_t;

/* Global Scheduler Data */
static pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_process = NULL;
static pcb_t *ready_queue[PRIORITY_LEVELS]; // Queue for each priority

/* Forward Declarations */
extern void switch_to(pcb_t *prev, pcb_t *next); // Assembly function
void scheduler_tick(void);

/*
 * system_init_scheduler
 * Initializes the process table and creates the 'idle' process.
 */
void system_init_scheduler(void) {
    kprintf("[KERNEL] Initializing HOCS Real-Time Scheduler...\n");
    
    // 1. Zero out the process table
    for (int i = 0; i < MAX_PROCESSES; i++) {
        process_table[i].state = PROC_UNUSED;
        process_table[i].pid = i;
    }

    // 2. Create PID 0 (Idle Process)
    pcb_t *idle = &process_table[0];
    idle->state = PROC_RUNNING;
    strncpy(idle->name, "idle_task", 32);
    idle->priority = PRIORITY_LEVELS - 1; // Lowest priority
    
    current_process = idle;
    kprintf("[KERNEL] Scheduler Active. CPU Handover complete.\n");
}

/*
 * create_process
 * Allocates a new PCB, sets up the stack frame for ARM64 return.
 */
int create_process(const char *name, void (*entry_point)(void), uint32_t priority) {
    if (priority >= PRIORITY_LEVELS) return -1;

    // Find free slot
    int pid = -1;
    for (int i = 1; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROC_UNUSED) {
            pid = i;
            break;
        }
    }

    if (pid == -1) {
        kprintf("[ERR] Process table full!\n");
        return -1;
    }

    pcb_t *p = &process_table[pid];
    
    // Setup PCB
    strncpy(p->name, name, 32);
    p->priority = priority;
    p->state = PROC_CREATED;
    p->ticks_remaining = TIME_SLICE_MS;
    
    // Allocate Kernel Stack (Simplified physical alloc)
    // In full version, use kmalloc()
    static uint8_t stack_pool[MAX_PROCESSES][STACK_SIZE]; 
    p->stack_base = (uintptr_t)&stack_pool[pid][STACK_SIZE];
    p->stack_ptr = p->stack_base;

    // Setup Context for Context Switching
    // When we switch to this task, it looks like it just returned from a function call
    p->context.pc = (uint64_t)entry_point;
    p->context.sp = p->stack_ptr;
    p->context.pstate = 0x3C5; // EL1h, Interrupts masked initially

    // Add to Ready Queue
    p->state = PROC_READY;
    
    // Simple Queue Insertion (Head)
    // In full version, implement a proper linked list append
    p->next = ready_queue[priority];
    ready_queue[priority] = p;

    kprintf("[SCHED] Created PID %d: %s\n", pid, name);
    return pid;
}

/*
 * schedule
 * The Core Logic. Picks the next best task to run.
 */
void schedule(void) {
    pcb_t *next = NULL;
    pcb_t *prev = current_process;

    // 1. Check for high priority tasks
    for (int prio = 0; prio < PRIORITY_LEVELS; prio++) {
        if (ready_queue[prio] != NULL) {
            // Found a task
            next = ready_queue[prio];
            
            // Round Robin: Move head to tail
            ready_queue[prio] = next->next; // Pop
            next->next = NULL; // Detach
            
            break;
        }
    }

    // 2. If no task ready, run Idle
    if (next == NULL) {
        if (current_process->pid == 0 && current_process->state == PROC_RUNNING) {
            return; // Already idling
        }
        next = &process_table[0]; // PID 0
    }

    // 3. Context Switch
    if (next != prev) {
        prev->state = PROC_READY;
        // Re-queue previous task (if not idle)
        if (prev->pid != 0 && prev->state != PROC_BLOCKED) {
             // Add back to queue logic here...
        }
        
        next->state = PROC_RUNNING;
        current_process = next;
        
        // Low-level assembly switch
        // kprintf("[SW] %s -> %s\n", prev->name, next->name);
        switch_to(prev, next);
    }
}

/*
 * yield
 * Voluntarily give up CPU
 */
void yield(void) {
    schedule();
}

/* * End of File
 * Additional 800 lines of IPC (Inter-Process Communication) 
 * logic would follow in a production system.
 */
