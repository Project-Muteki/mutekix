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
    memset(thr, 0, sizeof(thr->unk_0x34));
    return 0;
}

void **mutekix_tls_get(thread_t *thr, int key) {
    void **tls_ptrs = (void **) &(thr->unk_0x34);
    if (key <= MUTEKIX_TLS_KEY_MAX) {
        return tls_ptrs + key;
    }
    return NULL;
}

int mutekix_tls_set(thread_t *thr, int key, void *value) {
    void **tls_ptrs = (void **) &(thr->unk_0x34);
    if (key <= MUTEKIX_TLS_KEY_MAX) {
        *(tls_ptrs + key) = value;
        return 0;
    }
    return -1;
}
