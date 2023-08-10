set(SOFT_TIMER_PATH
    ./lib/STM32SoftTimer
)

list(APPEND LIB_SOURCES
    ${SOFT_TIMER_PATH}/src/soft_timer.c
)

list(APPEND LIB_INCLUDE_DIRECTORIES
    ${SOFT_TIMER_PATH}/inc
)
