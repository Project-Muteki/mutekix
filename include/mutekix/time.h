/**
 * @file time.h
 * @brief Utilities for time measurement.
 * @details
 * There is no standard way to measure time elapsed at millisecond-level on Besta RTOS. There is a millis field in the
 * datetime_t struct but its behavior is not standard across devices. The purpose of this component is to provide a
 * standard way of time measurement by choosing the best measurement method for each device, handling the
 * device-specific behavior required by each known device type, and trying to fall back to a safe default if unexpected
 * behavior is detected.
 */

#ifndef __MUTEKIX_TIME_H__
#define __MUTEKIX_TIME_H__

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize component, allocate required resources and reset the counters.
 * @x_void_param
 * @retval true @x_term ok
 * @retval false @x_term ng
 */
bool mutekix_time_init();

/**
 * @brief Deinitialize component.
 * @details Must be called before the applet quits.
 * @x_void_param
 * @x_void_return
 */
void mutekix_time_fini();

/**
 * @brief Get microseconds.
 * @return Microseconds elapsed since mutekix_time_init() was called.
 */
unsigned long long mutekix_time_get_usecs();

/**
 * @brief Get ticks.
 * @details Resolution of each tick may differ across devices.
 * @return Ticks elapsed since mutekix_time_init() was called.
 */
unsigned long long mutekix_time_get_ticks();

/**
 * @brief Get quantum (microseconds elapsed of each tick).
 * @return Microsecond of each tick.
 */
unsigned int mutekix_time_get_quantum();

#ifdef __cplusplus
} // extern "C"
#endif

#endif // __MUTEKIX_TIME_H__
