#ifndef __MUTEKIX_THREAD_WRAPPER_H__
#define __MUTEKIX_THREAD_WRAPPER_H__
#include <muteki/threading.h>

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
void mutekix_thread_wrapper(void *arg);
#endif // __MUTEKIX_THREAD_WRAPPER_H__
