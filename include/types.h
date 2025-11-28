#ifndef SOLIX_TYPES_H
#define SOLIX_TYPES_H

/**
 * SolixOS Type Definitions
 * Modernized with enhanced type safety and C99 compatibility
 * Version: 2.0
 */

// Fixed-width integer types
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

// Pointer-sized types
typedef uint32_t uintptr_t;
typedef int32_t intptr_t;
typedef uint32_t size_t;
typedef int32_t ssize_t;
typedef uint32_t ptrdiff_t;

// Boolean type (C99 compatible)
#if __STDC_VERSION__ >= 199901L
#include <stdbool.h>
#else
typedef uint8_t bool;
#define true 1
#define false 0
#endif

// NULL pointer definition
#ifndef NULL
#define NULL ((void*)0)
#endif

// Modern type safety macros
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define offsetof(type, member) ((size_t)&((type*)0)->member)
#define container_of(ptr, type, member) ({ \
    const typeof(((type*)0)->member) *__mptr = (ptr); \
    (type*)((char*)__mptr - offsetof(type, member)); \
})

// Compiler attributes for optimization and debugging
#define __packed __attribute__((packed))
#define __aligned(n) __attribute__((aligned(n)))
#define __noreturn __attribute__((noreturn))
#define __unused __attribute__((unused))
#define __used __attribute__((used))
#define __weak __attribute__((weak))
#define __alias(x) __attribute__((alias(x)))

// Likely/unlikely hints for branch prediction
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

// Memory barriers
#define mb() __asm__ __volatile__("mfence" ::: "memory")
#define rmb() __asm__ __volatile__("lfence" ::: "memory")
#define wmb() __asm__ __volatile__("sfence" ::: "memory")

// Atomic operations (basic)
#define atomic_read(ptr) (*(volatile typeof(*ptr) *)(ptr))
#define atomic_set(ptr, val) (*(volatile typeof(*ptr) *)(ptr) = (val))
#define atomic_inc(ptr) (*(volatile typeof(*ptr) *)(ptr)++)
#define atomic_dec(ptr) (*(volatile typeof(*ptr) *)(ptr)--)

// Debug build support
#ifdef DEBUG
#define ASSERT(x) do { if (unlikely(!(x))) { \
    panic("Assertion failed: %s at %s:%d", #x, __FILE__, __LINE__); \
} } while(0)
#else
#define ASSERT(x) ((void)0)
#endif

// Function attributes for API documentation
#define API __attribute__((visibility("default")))
#define INTERNAL __attribute__((visibility("hidden")))
#define DEPRECATED __attribute__((deprecated))

#endif
