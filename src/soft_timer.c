/**
 * @file soft_timer.c
 *
 * @brief Software timer adapter.
 *
 * @author Daniel Nery <daniel.nery@thunderatz.org>
 *
 * @date 07/2019
 */

#include "soft_timer.h"

#include "tim.h"
#include "utils.h"

/*****************************************
 * Private Constants
 *****************************************/

#define STOPPED_TIMER_COUNTDOWN_VALUE (0xFFFFFFFF)

#if SOFT_TIMER_MAX_TIMERS > 256
#error SOFT_TIMER_MAX_INSTANCES cannot be greater than 256.
#endif

/*****************************************
 * Private Macros
 *****************************************/

#define HZ_TO_KHZ(f) ((f) / 1e3)
#define HZ_TO_MHZ(f) ((f) / 1e6)

/*****************************************
 * Private Functions Prototypes
 *****************************************/

/**
 * @brief Stops given software timer.
 *
 *  @param timer Pointer to software timer to be stopped.
 */
static void timer_stop(soft_timer_t* timer);

/**
 * @brief Update given software timer with given amount of time.
 *
 * @note Expired timers callbacks are called.
 *
 * @param timer        Pointer to timer to be updated.
 * @param time_step_ms Amount of time to be updated (milliseconds)
 */
static void timer_step(soft_timer_t* timer, uint16_t time_step_ms);

/**
 * @brief Update software timers and configure timer handler accordingly.
 *
 * @param time_since_last_update_ms Time since software timers were last
 *                                  updated (in milliseconds).
 */
static void timers_update(uint32_t time_since_last_update_ms);

/**
 * @brief Update software timers.
 *
 * @note Expired timers callbacks are called.
 *
 * @param time_since_last_update_ms Time since last hardware update was
 *                                  performed (in milliseconds).
 *
 * @return Time until next software timer expires (in milliseconds).
 */
static uint32_t soft_timers_update(uint32_t time_since_last_update_ms);

/**
 * @brief Initializes the timer.
 *
 * @note This functions assumes APBx timer clock is the same as
 *       HCLK to calculate prescaler.
 *
 * @note Timer is configured to 1 millisecond resolution.
 *
 * @param htim Pointer to timer handler to be initilized.
 */
static void hard_timer_init(TIM_HandleTypeDef* htim);

/**
 * @brief Starts the timer.
 *
 * @param htim Pointer to timer to be started.
 */
static void hard_timer_start(TIM_HandleTypeDef* htim);

/**
 * @brief Stops the timer.
 *
 * @param htim Pointer to timer to be stopped.
 */
static void hard_timer_stop(TIM_HandleTypeDef* htim);

/**
 * @brief Gets timer counter value.
 *
 * @param htim Pointer to timer.
 *
 * @return Timer counter value in milliseconds.
 */
static uint32_t hard_timer_counter_get(TIM_HandleTypeDef* htim);

/**
 * @brief Sets timer reload value.
 *
 * @param htim      Pointer to timer.
 * @param reload_ms Timer reload value in milliseconds.
 *                  Caps at hard_timer_MAX_RELOAD_MS
 */
static void hard_timer_reload_set(TIM_HandleTypeDef* htim, uint32_t reload_ms);

/**
 * @brief Gets timer reload value.
 *
 * @param htim Pointer to timer.
 *
 * @return Timer reload value in milliseconds.
 */
static uint32_t hard_timer_reload_get(TIM_HandleTypeDef* htim);

/**
 * @brief Update timer handler..
 *
 * @note Stops timer if reload is 0.
 *
 * @param timer_reload_ms Reload value to be configured in handler (in milliseconds).
 */
static void hard_timer_update(uint32_t timer_reload_ms);

/*****************************************
 * Private Types
 *****************************************/

/**
 * @brief Possible timer states.
 */
typedef enum timer_state {
    TIMER_STATE_FREE = 0, /**< Unallocated timer. */
    TIMER_STATE_STOPPED,  /**< Allocated timer that is not running. */
    TIMER_STATE_RUNNING,  /**< Allocated timer that is running. */
} timer_state_t;

/**
 * @brief Type definition for software timer instance.
 */
struct soft_timer {
    timer_state_t         state;        /**< Current timer state. */
    uint8_t               id;           /**< Sequential timer id */
    uint32_t              reload_ms;    /**< Configured reload value. */
    uint32_t              countdown_ms; /**< Remaining time until timeout. */
    bool                  repeat;       /**< Repeat setting. */
    soft_timer_callback_t callback;     /**< Timeout callback. */
};

/*****************************************
 * Private Variables
 *****************************************/

/**
 * @brief Array of available software timer instances.
 */
static soft_timer_t m_timers[SOFT_TIMER_MAX_TIMERS];

/**
 * @brief Timer handler instance.
 */
static TIM_HandleTypeDef* mp_htim;

/**
 * @brief Flags if this module has already been initialized.
 *
 * @note Avoids multiple initializations that would reset running timers.
 */
static bool m_is_initialized = false;

/**
 * @brief Physical timer max reload value
 *
 * @note Usually 16-bit (0xFFFF) or 32-bit (0xFFFFF), this is capped at
 *       0xFFFFFFFF - 1, because 0xFFFFFFF is reserved for stopped timers.
 */
static uint32_t m_max_reload_ms = 0xFFFF;

/*****************************************
 * Public Functions Bodies Definitions
 *****************************************/

void soft_timer_init(TIM_HandleTypeDef* htim, uint32_t max_reload_ms) {
    mp_htim = htim;
    m_max_reload_ms = min(max_reload_ms, 0xFFFFFFFF - 1);

    if (m_is_initialized) {
        return;
    }

    for (uint8_t i = 0; i < SOFT_TIMER_MAX_TIMERS; i++) {
        timer_stop(&m_timers[i]);
        m_timers[i].state = TIMER_STATE_FREE;
        m_timers[i].id = i;
    }

    hard_timer_init(mp_htim);

    m_is_initialized = true;
}

soft_timer_t* soft_timer_create(void) {
    for (uint8_t i = 0; i < SOFT_TIMER_MAX_TIMERS; i++) {
        if (m_timers[i].state == TIMER_STATE_FREE) {
            m_timers[i].state = TIMER_STATE_STOPPED;
            return &m_timers[i];
        }
    }

    return NULL;
}

void soft_timer_destroy(soft_timer_t** timer) {
    soft_timer_t* first_timer = &(m_timers[0]);
    soft_timer_t* last_timer = &(m_timers[SOFT_TIMER_MAX_TIMERS]);

    if ((*timer >= first_timer) && (*timer < last_timer) && ((*timer)->state == TIMER_STATE_STOPPED)) {
        (*timer)->state = TIMER_STATE_FREE;
        *timer = NULL;
    }
}

soft_timer_status_t soft_timer_set(soft_timer_t* timer, soft_timer_callback_t callback, uint32_t reload_ms,
                                   bool repeat) {
    soft_timer_t* first_timer = &(m_timers[0]);
    soft_timer_t* last_timer = &(m_timers[SOFT_TIMER_MAX_TIMERS]);

    if (!((timer >= first_timer) && (timer < last_timer))) {
        return SOFT_TIMER_STATUS_INVALID_PARAMETER;
    }

    if ((reload_ms <= 1) || (reload_ms > m_max_reload_ms)) {
        return SOFT_TIMER_STATUS_INVALID_PARAMETER;
    }

    if (timer->state != TIMER_STATE_STOPPED) {
        return SOFT_TIMER_STATUS_INVALID_STATE;
    }

    timer->reload_ms = reload_ms;
    timer->repeat = repeat;
    timer->callback = callback;

    return SOFT_TIMER_STATUS_SUCCESS;
}

soft_timer_status_t soft_timer_start(soft_timer_t* timer) {
    soft_timer_t* first_timer = &(m_timers[0]);
    soft_timer_t* last_timer = &(m_timers[SOFT_TIMER_MAX_TIMERS]);

    if (!((timer >= first_timer) && (timer < last_timer))) {
        return SOFT_TIMER_STATUS_INVALID_PARAMETER;
    }

    if (timer->state != TIMER_STATE_STOPPED) {
        return SOFT_TIMER_STATUS_INVALID_STATE;
    }

    uint32_t time_since_last_update_ms = hard_timer_counter_get(mp_htim);

    timers_update(time_since_last_update_ms);

    timer->countdown_ms = timer->reload_ms - 1;
    timer->state = TIMER_STATE_RUNNING;

    timers_update(0);

    return SOFT_TIMER_STATUS_SUCCESS;
}

soft_timer_status_t soft_timer_stop(soft_timer_t* timer) {
    soft_timer_t* first_timer = &(m_timers[0]);
    soft_timer_t* last_timer = &(m_timers[SOFT_TIMER_MAX_TIMERS]);

    if (!((timer >= first_timer) && (timer < last_timer))) {
        return SOFT_TIMER_STATUS_INVALID_PARAMETER;
    }

    if (timer->state != TIMER_STATE_RUNNING) {
        return SOFT_TIMER_STATUS_INVALID_STATE;
    }

    timer_stop(timer);

    uint32_t time_since_last_update_ms = hard_timer_counter_get(mp_htim);

    timers_update(time_since_last_update_ms);

    return SOFT_TIMER_STATUS_SUCCESS;
}

bool soft_timer_is_stopped(soft_timer_t* timer) {
    return (timer->state == TIMER_STATE_STOPPED);
}

void soft_timer_period_elapsed_callback(void) {
    uint16_t elapsed_time_ms = hard_timer_reload_get(mp_htim);

    timers_update(elapsed_time_ms);
}

/*****************************************
 * Private Functions Bodies Definitions
 *****************************************/

void timer_stop(soft_timer_t* timer) {
    timer->state = TIMER_STATE_STOPPED;
    timer->countdown_ms = STOPPED_TIMER_COUNTDOWN_VALUE;
    timer->repeat = false;
}

void timer_step(soft_timer_t* timer, uint16_t time_step_ms) {
    if (timer->state != TIMER_STATE_RUNNING) {
        return;
    }

    if ((timer->countdown_ms == 0) && (time_step_ms != 0)) {
        return;
    }

    timer->countdown_ms -= time_step_ms;

    if (timer->countdown_ms != 0) {
        return;
    }

    if (timer->callback != NULL) {
        timer->callback(timer);
    }

    if (timer->repeat) {
        timer->countdown_ms = timer->reload_ms - 1;
    } else {
        timer_stop(timer);
    }
}

void timers_update(uint32_t time_since_last_update_ms) {
    uint32_t next_reload_ms = soft_timers_update(time_since_last_update_ms);

    hard_timer_update(next_reload_ms);
}

uint32_t soft_timers_update(uint32_t time_since_last_update_ms) {
    uint32_t time_until_next_timeout_ms = STOPPED_TIMER_COUNTDOWN_VALUE;

    for (uint8_t i = 0; i < SOFT_TIMER_MAX_TIMERS; i++) {
        timer_step(&m_timers[i], time_since_last_update_ms);

        time_until_next_timeout_ms = min(time_until_next_timeout_ms, m_timers[i].countdown_ms);
    }

    return (time_until_next_timeout_ms == STOPPED_TIMER_COUNTDOWN_VALUE) ?
           0 : time_until_next_timeout_ms;
}

void hard_timer_update(uint32_t timer_reload_ms) {
    hard_timer_reload_set(mp_htim, timer_reload_ms);

    if (timer_reload_ms != 0) {
        hard_timer_start(mp_htim);
    } else {
        hard_timer_stop(mp_htim);
    }
}

void hard_timer_init(TIM_HandleTypeDef* htim) {
    uint32_t hclk_frequency = HAL_RCC_GetHCLKFreq();
    uint32_t prescaler = HZ_TO_KHZ(hclk_frequency) - 1;

    __HAL_TIM_SET_PRESCALER(htim, prescaler);
    __HAL_TIM_SET_AUTORELOAD(htim, m_max_reload_ms);
    __HAL_TIM_CLEAR_FLAG(htim, TIM_FLAG_UPDATE);
}

void hard_timer_start(TIM_HandleTypeDef* htim) {
    __HAL_TIM_SET_COUNTER(htim, 0);
    HAL_TIM_Base_Start_IT(htim);
}

void hard_timer_stop(TIM_HandleTypeDef* htim) {
    HAL_TIM_Base_Stop_IT(htim);
    __HAL_TIM_SET_COUNTER(htim, 0);
}

uint32_t hard_timer_reload_get(TIM_HandleTypeDef* htim) {
    return __HAL_TIM_GET_AUTORELOAD(htim);
}

uint32_t hard_timer_counter_get(TIM_HandleTypeDef* htim) {
    return __HAL_TIM_GET_COUNTER(htim);
}

void hard_timer_reload_set(TIM_HandleTypeDef* htim, uint32_t reload_ms) {
    reload_ms = min(reload_ms, m_max_reload_ms);

    __HAL_TIM_SET_AUTORELOAD(htim, reload_ms);
}

TIM_TypeDef* hard_timer_get_instance(TIM_HandleTypeDef* htim) {
    return htim->Instance;
}
