/**
 * @file soft_timer_adapter.h
 *
 * @brief Software timer adapter.
 *
 * @author Daniel Nery <daniel.nery@thunderatz.org>
 *
 * @date 07/2019
 */

#if !defined(__SOFT_TIMER_H__)
#define __SOFT_TIMER_H__

#include <stdbool.h>

#include "tim.h"

/*****************************************
 * Public Constants
 *****************************************/

#define SOFT_TIMER_MAX_TIMERS 10

/*****************************************
 * Public Types
 *****************************************/

/**
 * @brief Forward declaration of software timer instance.
 *
 * @note This struct is private to the soft_timer. Internals are
 *       not known by the user.
 */
typedef struct soft_timer soft_timer_t;

/**
 * @brief Timer timeout callback.
 *
 * @note Type for a function that is called when the countdown value for a
 *       timer reaches 0.
 *
 * @param soft_timer_t Pointer to timer triggering timeout callback.
 */
typedef void (* soft_timer_callback_t)(soft_timer_t*);

/**
 * @brief Status codes for software timer functions.
 */
typedef enum soft_timer_status {
    SOFT_TIMER_STATUS_SUCCESS = 0,
    SOFT_TIMER_STATUS_INVALID_PARAMETER,
    SOFT_TIMER_STATUS_INVALID_STATE,
} soft_timer_status_t;

/*****************************************
 * Public Functions Prototypes
 *****************************************/

/**
 * @brief Initialize software timer module.
 *
 * @param htim          Pointer to HAL Timer handler
 * @param max_reload_ms Maximum timer value, usually 0xFFFF or 0xFFFFFFFF
 */
void soft_timer_init(TIM_HandleTypeDef* htim, uint32_t max_reload_ms);

/**
 * @brief Allocates and initializes a software timer instance.
 *
 * @return Pointer to allocated timer.
 * @retval NULL Returns null if no free timer is available.
 */
soft_timer_t* soft_timer_create(void);

/**
 * @brief Dellocates software timer instance.
 *
 * @note Timers must be stopped before being deallocated.
 *
 * @param timer Pointer to software timer instance to be destroyed.
 *              This pointer is set to NULL if it is deallocated successfully.
 */
void soft_timer_destroy(soft_timer_t** timer);

/**
 * @brief Configures timer.
 *
 * @note This function will configure a timer instance that is stopped.
 *       Only configured timers may be started.
 *
 * @param timer     Pointer to timer instance to be configured.
 * @param callback  Pointer to timeout callback function.
 * @param reload_ms Value to reload timer in milliseconds (min 2).
 * @param repeat    Boolean flag signalling if timer should repeat after timeout.
 *
 * @return Operation status.
 */
soft_timer_status_t soft_timer_set(soft_timer_t* timer, soft_timer_callback_t callback, uint32_t reload_ms,
                                   bool repeat);

/**
 * @brief Starts timer.
 *
 * @note Timer must be previously configured with @ref soft_timer_set
 *       and must be stopped.
 *
 * @param timer Pointer to timer instance to be started.
 *
 * @return Operation status.
 */
soft_timer_status_t soft_timer_start(soft_timer_t* timer);

/**
 * @brief Stops timer.
 *
 * @note Timer must be started.
 *
 * @param timer Pointer to timer instance to be stopped.
 *
 * @return Operation status.
 */
soft_timer_status_t soft_timer_stop(soft_timer_t* timer);

/**
 * @brief Checks if a timer is stopped.
 *
 * @param timer Pointer to timer instance to be stopped.
 *
 * @return true if timer is stopped, false otherwise.
 */
bool soft_timer_is_stopped(soft_timer_t* timer);

void soft_timer_period_elapsed_callback(void);

#endif // __SOFT_TIMER_H__
