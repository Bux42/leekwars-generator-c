#include "lw_item.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

static long lw_item_compare_exchange(long *address, long expected, long desired) {
#ifdef _MSC_VER
    return _InterlockedCompareExchange(address, desired, expected);
#else
    __atomic_compare_exchange_n(address, &expected, desired, 0,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return expected;
#endif
}

int lw_item_manage(LwItem *self) {
    if (!self || self->references) return 0;
    if (self->name) {
        size_t length = strlen(self->name);
        char *name = malloc(length + 1);
        if (!name) return 0;
        memcpy(name, self->name, length + 1);
        self->owned_name = name;
        self->name = name;
    }
    self->references = 1;
    return 1;
}

/* A plain atomic read: unlike a compare-exchange, it lets threads share the
 * cache line of items that are never counted (references == 0). */
static long lw_item_load(LwItem *self) {
#if defined(_MSC_VER) && !defined(__clang__)
    return *(volatile long *)&self->references;
#else
    return __atomic_load_n(&self->references, __ATOMIC_ACQUIRE);
#endif
}

void lw_item_retain(LwItem *self) {
    if (self) {
        long count = lw_item_load(self);
        while (count) {
            if (count == LONG_MAX) {
                fputs("item reference count overflow\n", stderr);
                abort();
            }
            long previous = lw_item_compare_exchange(&self->references, count, count + 1);
            if (previous == count) return;
            count = previous;
        }
    }
}

void lw_item_release(LwItem *self) {
    if (self) {
        long count = lw_item_load(self);
        while (count) {
            long previous = lw_item_compare_exchange(&self->references, count, count - 1);
            if (previous == count) {
                if (count == 1) {
                    free(self->owned_name);
                    free(self);
                }
                return;
            }
            count = previous;
        }
    }
}
