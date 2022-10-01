#include <stdlib.h>
#include <string.h>

#include <muteki/threading.h>
#include <mutekix/threading.h>

thread_t *mutekix_thread_get_current() {
    critical_section_t cs;

#if (defined(MUTEKIX_USE_ONLY_PUBLIC_INTERFACES) && MUTEKIX_USE_ONLY_PUBLIC_INTERFACES != 0)
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
    cs.magic = 0x202;
    cs.thr = NULL;
    cs.refcount = 0;
    OSEnterCriticalSection(&cs);
    return cs.thr;
#endif
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

int mutekix_tls_set(thread_t *thr, unsigned int key, void *value) {
    void **tls_ptrs = (void **) &(thr->unk_0x34);
    if (key <= MUTEKIX_TLS_KEY_MAX) {
        *(tls_ptrs + key) = value;
        return 0;
    }
    return -1;
}

int mutekix_tls_init_self() {
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

int mutekix_tls_set_self(unsigned int key, void *value) {
    thread_t *thr = mutekix_thread_get_current();
    if (thr == NULL) {
        return -1;
    }
    return mutekix_tls_set(thr, key, value);
}

// NOTE: __tdata_end, __tbss_start and __tbss_end are not available in the default EABI ld script and needs to be defined.
extern char __tdata_start;
extern char __tdata_end;
extern char __tbss_start;
extern char __tbss_end;

void *__aeabi_read_tp_real() {
    void **tls_area_p = mutekix_tls_get_self(MUTEKIX_TLS_KEY_TLS);
    if (tls_area_p == NULL) {
        return NULL;
    }

    // TODO do we move the init routine to outside?
    char *tls_area = (char *) *tls_area_p;
    if (tls_area == NULL) {
        size_t tdata_size = &__tdata_end - &__tdata_start;
        size_t tbss_size = &__tbss_end - &__tbss_start;
        // TODO what's with this 8-byte padding? Is it related to dynamic linking?
        // Seems like musl just calls it "gap"
        tls_area = malloc(8 + tdata_size + tbss_size);

        if (tls_area != NULL) {
            memset(tls_area, 0, 8);
            if (tdata_size > 0) {
                memcpy(tls_area + 8, &__tdata_start, tdata_size);
            }
            if (tbss_size > 0) {
                memset(tls_area + 8 + tdata_size, 0, tbss_size);
            }
        }
        *tls_area_p = tls_area;
    }

    return tls_area;
}

__attribute__((naked))
void __aeabi_read_tp() {
    // Save registers that are normally scratch registers except r0 to satisfy the no clobber requirements
    asm volatile (
        "push {r1, r2, r3, r4, ip, lr}\n\t"
        "bl __aeabi_read_tp_real\n\t"
        "pop {r1, r2, r3, r4, ip, lr}\n\t"
        "bx lr"
    );
}
