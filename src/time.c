#include "mutekix/time.h"
#include "mutekix/threading.h"

#include <stdlib.h>
#include <time.h>

#include <muteki/datetime.h>
#include <muteki/threading.h>
#include <muteki/utils.h>

typedef enum preferred_timer_type_e {
    TIMER_TYPE_SCHED,
    TIMER_TYPE_RTC,
    TIMER_TYPE_RTC_DOUBLE,
} preferred_timer_type_t;

static volatile unsigned int scheduler_tick_timer = 0;
static unsigned long long accumulator = 0;

static preferred_timer_type_t timer_type = TIMER_TYPE_SCHED;
static datetime_t previous_date = {0};
static thread_t *timer_thread = NULL;
static critical_section_t cs;
static mutekix_thread_arg_t *thrarg = NULL;

static int _scheduler_tick_hook_count(void *user_data) {
    (void) user_data;

    while (true) {
        OSSleep(1);
        scheduler_tick_timer++;
    }

    return 0;
}

static int _scheduler_tick_hook_double_rtc(void *user_data) {
    (void) user_data;
    datetime_t dt = {0};

    while (true) {
        OSSleep(200);
        OSEnterCriticalSection(&cs);
        GetSysTime(&dt);
        scheduler_tick_timer += (dt.millis < previous_date.millis) ? (1000 + dt.millis - previous_date.millis) : (dt.millis - previous_date.millis);
        previous_date = dt;
        OSLeaveCriticalSection(&cs);
    }

    return 0;
}

static preferred_timer_type_t _detect_timer_type() {
    datetime_t dt = {0};
    int delta_ms_total = 0;
    for (int i = 0; i < 10; i++) {
        int before_ms = 0;

        GetSysTime(&dt);
        before_ms = dt.millis;
        OSSleep(5);
        GetSysTime(&dt);

        int delta_ms = dt.millis - before_ms;
        if (delta_ms < 0) {
            delta_ms = 1000 + delta_ms;
        }
        delta_ms_total += delta_ms;
    }
    if (delta_ms_total > 95) {
        return TIMER_TYPE_RTC_DOUBLE;
    } else if (delta_ms_total > 55 || delta_ms_total > 105) {
        WriteComDebugMsg("_detect_timer_type(): Unreliable RTC timer. Ignoring.");
        return TIMER_TYPE_SCHED;
    } else if (delta_ms_total > 45) {
        return TIMER_TYPE_RTC;
    } else if (delta_ms_total <= 0) {
        return TIMER_TYPE_SCHED;
    }
    return TIMER_TYPE_SCHED;
}

static time_t convert_date(const datetime_t *dt) {
    struct tm curr = {
        .tm_year = dt->year - 1900,
        .tm_mon = dt->month - 1,
        .tm_mday = dt->day,
        .tm_hour = dt->hour,
        .tm_min = dt->minute,
        .tm_sec = dt->second,
        .tm_isdst = -1,
    };

    return mktime(&curr);
}

bool mutekix_time_init() {
    mutekix_time_fini();

    accumulator = 0;
    scheduler_tick_timer = 0;

    timer_type = _detect_timer_type();
    switch (timer_type) {
        case TIMER_TYPE_RTC_DOUBLE: {
            thrarg = malloc(sizeof(*thrarg));
            if (thrarg == NULL) {
                mutekix_time_fini();
                return false;
            }
            thrarg->func = &_scheduler_tick_hook_double_rtc;
            thrarg->user_data = NULL;
            OSInitCriticalSection(&cs);
            timer_thread = OSCreateThread(mutekix_thread_wrapper, thrarg, 8192, false);
            GetSysTime(&previous_date);
            return true;
        }
        case TIMER_TYPE_RTC: {
            GetSysTime(&previous_date);
            return true;
        }
        case TIMER_TYPE_SCHED: {
            thrarg = malloc(sizeof(*thrarg));
            if (thrarg == NULL) {
                mutekix_time_fini();
                return false;
            }
            thrarg->func = &_scheduler_tick_hook_count;
            thrarg->user_data = NULL;
            timer_thread = OSCreateThread(mutekix_thread_wrapper, thrarg, 8192, false);
            return true;
        }
    }
    return false;
}

void mutekix_time_fini() {
    if (timer_thread != NULL) {
        OSTerminateThread(timer_thread, 0);
        timer_thread = NULL;
    }
    if (thrarg != NULL) {
        free(thrarg);
        thrarg = NULL;
    }
    if (timer_type == TIMER_TYPE_RTC_DOUBLE) {
        OSDeleteCriticalSection(&cs);
    }
}

unsigned long long mutekix_time_get_usecs() {
    return mutekix_time_get_ticks() * mutekix_time_get_quantum();
}

unsigned long long mutekix_time_get_ticks() {
    switch (timer_type) {
        case TIMER_TYPE_SCHED: {
            if (scheduler_tick_timer < (accumulator & 0xffffffff)) {
                accumulator += 0x100000000ull;
            }
            accumulator = (accumulator & 0xffffffff00000000ull) | scheduler_tick_timer;
            break;
        }
        case TIMER_TYPE_RTC_DOUBLE: {
            datetime_t dt = {0};

            OSEnterCriticalSection(&cs);

            accumulator += scheduler_tick_timer;
            scheduler_tick_timer = 0;

            GetSysTime(&dt);
            accumulator += (dt.millis < previous_date.millis) ? (1000 + dt.millis - previous_date.millis) : (dt.millis - previous_date.millis);
            previous_date = dt;

            OSLeaveCriticalSection(&cs);

            break;
        }
        case TIMER_TYPE_RTC: {
            datetime_t dt = {0};
            GetSysTime(&dt);
            if (!(
                dt.year == previous_date.year &&
                dt.month == previous_date.month &&
                dt.day == previous_date.day &&
                dt.hour == previous_date.hour &&
                dt.minute == previous_date.minute &&
                (dt.second - previous_date.second < 2)
            )) {
                time_t current_timestamp = convert_date(&dt);
                time_t previous_timestamp = convert_date(&previous_date);
                time_t sec_delta = current_timestamp - previous_timestamp;
                if (sec_delta > 1) {
                    accumulator += sec_delta * 1000;
                }
            }

            accumulator += (dt.millis < previous_date.millis) ? (1000 + dt.millis - previous_date.millis) : (dt.millis - previous_date.millis);
            previous_date = dt;
            break;
        }
    }
    return accumulator;
}

unsigned int mutekix_time_get_quantum() {
    switch (timer_type) {
        case TIMER_TYPE_SCHED:
        case TIMER_TYPE_RTC:
            return 1000;
        case TIMER_TYPE_RTC_DOUBLE:
            return 500;
    }
    return 1000;
}
