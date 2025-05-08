#include <stdlib.h>
#include <string.h>

#include <muteki/threading.h>
#include "mutekix/threading.h"

#ifdef MUTEKIX_PROVIDE_TLS

thread_t *mutekix_thread_get_current(void) {
    critical_section_t cs;

#ifdef MUTEKIX_USE_ONLY_PUBLIC_INTERFACES
    /* Critical section holds a pointer that points to the current thread. This
     * function uses this property to get the descriptor of current running thread
     * by create and acquire a critical section, read out the pointer and clean
     * up. */
    OSInitCriticalSection(&cs);
    OSEnterCriticalSection(&cs);
    thread_t *thr = cs.thr;
    OSLeaveCriticalSection(&cs);
    OSDeleteCriticalSection(&cs);
    return thr;
#else
    /* This is based on the observation that critical sections don't touch any
     * kernel structures when there's nothing else that acquired it. It
     * initializes the critical section descriptor just enough and call
     * OSEnterCriticalSection() to grab the current thread ID. */
    cs.thr = NULL;
    cs.refcount = 0;
    OSEnterCriticalSection(&cs);
    return cs.thr;
#endif // MUTEKIX_USE_ONLY_PUBLIC_INTERFACES
}

// TLS
// TODO Add a USE_ONLY_PUBLIC_INTERFACES version

int mutekix_tls_init(thread_t *thr) {
    memset(&thr->unk_0x34, 0, sizeof(thr->unk_0x34));
    return 0;
}

void **mutekix_tls_get(thread_t *thr, unsigned int key) {
    void **tls_ptrs = (void **) &(thr->unk_0x34);
    if (key <= MUTEKIX_TLS_KEY_MAX) {
        return tls_ptrs + key;
    }
    return NULL;
}

void *mutekix_tls_getvalue(thread_t *thr, unsigned int key) {
    void **tls_ptrs = (void **) &(thr->unk_0x34);
    if (key <= MUTEKIX_TLS_KEY_MAX) {
        return *(tls_ptrs + key);
    }
    return NULL;
}

int mutekix_tls_set(thread_t *thr, unsigned int key, void *value) {
    void **tls_ptrs = (void **) &(thr->unk_0x34);
    if (key <= MUTEKIX_TLS_KEY_MAX) {
        *(tls_ptrs + key) = value;
        return 0;
    }
    return -1;
}

void *mutekix_tls_alloc(thread_t *thr, unsigned int key, size_t bytes) {
    void **tls_area_p = mutekix_tls_get(thr, key);
    if (tls_area_p == NULL) {
        return NULL;
    }
    void *tls_area = *tls_area_p;
    if (tls_area != NULL) {
        return NULL;
    }
    void *allocated = malloc(bytes);
    if (allocated == NULL) {
        return NULL;
    }
    *tls_area_p = allocated;
    return allocated;
}

int mutekix_tls_free(thread_t *thr, unsigned int key) {
    void *tls_area = mutekix_tls_getvalue(thr, key);
    if (tls_area == NULL) {
        return -1;
    }
    free(tls_area);
    mutekix_tls_set(thr, key, NULL);
    return 0;
}

int mutekix_tls_init_self(void) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return -1;
    }
    return mutekix_tls_init(thr);
}

void **mutekix_tls_get_self(unsigned int key) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return NULL;
    }
    return mutekix_tls_get(thr, key);
}

void *mutekix_tls_getvalue_self(unsigned int key) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return NULL;
    }
    return mutekix_tls_getvalue(thr, key);
}

int mutekix_tls_set_self(unsigned int key, void *value) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return -1;
    }
    return mutekix_tls_set(thr, key, value);
}

void *mutekix_tls_alloc_self(unsigned int key, size_t bytes) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return NULL;
    }
    return mutekix_tls_alloc(thr, key, bytes);
}

int mutekix_tls_free_self(unsigned int key) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return -1;
    }
    return mutekix_tls_free(thr, key);
}

// __aeabi_read_tp implementation using mutekix_tls
// NOTE: __tdata_end, __tbss_start and __tbss_end are not available in the default EABI ld script and needs to be defined.
extern char __tdata_start;
extern char __tdata_end;
extern char __tbss_start;
extern char __tbss_end;

void *__aeabi_read_tp_real(void) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return NULL;
    }

    char *tls_area = mutekix_tls_getvalue(thr, MUTEKIX_TLS_KEY_TLS);

    // initialize the TLS area if it's not already initialized
    if (tls_area == NULL) {
        size_t tdata_size = &__tdata_end - &__tdata_start;
        size_t tbss_size = &__tbss_end - &__tbss_start;
        // TODO what's with this 8-byte padding? Is it related to dynamic linking?
        // Seems like musl just calls it "gap"
        tls_area = mutekix_tls_alloc(thr, MUTEKIX_TLS_KEY_TLS, 8 + tdata_size + tbss_size);

        if (tls_area != NULL) {
            memset(tls_area, 0, 8);
            if (tdata_size > 0) {
                memcpy(tls_area + 8, &__tdata_start, tdata_size);
            }
            if (tbss_size > 0) {
                memset(tls_area + 8 + tdata_size, 0, tbss_size);
            }
        }
    }

    return tls_area;
}

__attribute__((naked))
void __aeabi_read_tp(void) {
    // Save registers that are normally scratch registers except r0 to satisfy the no clobber requirements
    asm volatile (
        "push {r1, r2, r3, r4, ip, lr}\n\t"
        "bl __aeabi_read_tp_real\n\t"
        "pop {r1, r2, r3, r4, ip, lr}\n\t"
        "bx lr"
    );
}

#endif // MUTEKIX_PROVIDE_TLS
