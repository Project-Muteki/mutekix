#ifndef __MUTEKIX_THREAD_WRAPPER_H__
#define __MUTEKIX_THREAD_WRAPPER_H__

#include <muteki/threading.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Input for mutekix_thread_wrapper().
 * When using mutekix_thread_wrapper(), pass this to the user_data argument of OSCreateThread().
 */
typedef struct {
    /** The real thread function/entry point being called. */
    thread_func_t func;
    /** User data for the thread function. */
    void *user_data;
} mutekix_thread_arg_t;

/**
 * @brief Wrap the thread function for ABI and THUMB compatibility.
 *
 * This wrapper ensures the stack alignment and uses BX to invoke the real function to properly set the THUMB bit when jumping to it.
 * @param arg The real function and its user data pointer.
 */
extern void mutekix_thread_wrapper(void *arg);

/**
 * @brief Get descriptor of current running thread.
 *
 * @return The descriptor of current running thread.
 */
extern thread_t *mutekix_thread_get_current();

/** Max TLS key allowed. */
static const unsigned int MUTEKIX_TLS_KEY_MAX = sizeof(((thread_t *) NULL)->unk_0x34) - 1;

#define MUTEKIX_TLS_KEY_TLS 0
#define MUTEKIX_TLS_KEY_TSS 1

/**
 * @brief Initialize Thread Local Storage (TLS) for a thread.
 *
 * This allocates the TLS block (if applicable) and set all values in that block to NULL.
 *
 * This must be called after the thread is created with OSCreateThread() if TLS will be used later.
 *
 * @param thr Thread descriptor the TLS block belongs to.
 * @return 0 if successful.
 */
extern int mutekix_tls_init(thread_t *thr);

/**
 * @brief Initialize Thread Local Storage (TLS) for current thread.
 *
 * This allocates the TLS block (if applicable) and set all values in that block to NULL.
 *
 * @return 0 if successful.
 */
extern int mutekix_tls_init_self();

/**
 * @brief Get a value in the TLS block of a specific thread using a key.
 *
 * @param thr Thread descriptor.
 * @param key TLS key.
 * @return Pointer to the TLS value, or NULL if failed.
 */
extern void **mutekix_tls_get(thread_t *thr, unsigned int key);

/**
 * @brief Get a value in the TLS block of current thread using a key.
 *
 * @param key TLS key.
 * @return Pointer to the TLS value, or NULL if failed.
 */
extern void **mutekix_tls_get_self(unsigned int key);

/**
 * @brief Set a value in the TLS block of a specific thread using a key.
 *
 * @param thr Thread descriptor.
 * @param key TLS key.
 * @param value New value.
 * @return Pointer to the TLS value, or NULL if failed.
 */
extern int mutekix_tls_set(thread_t *thr, unsigned int key, void *value);

/**
 * @brief Set a value in the TLS block of current thread using a key.
 *
 * @param key TLS key.
 * @param value New value.
 * @return Pointer to the TLS value, or NULL if failed.
 */
extern int mutekix_tls_set_self(unsigned int key, void *value);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MUTEKIX_THREAD_WRAPPER_H__
