# Utils library
add_library(utils OBJECT
    utils_init.c
    RTT/SEGGER_RTT.c
    RTT/SEGGER_RTT_printf.c
    ulog/ulog.c
    lwrb/lwrb.c
)

target_include_directories(utils PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/RTT
    ${CMAKE_CURRENT_SOURCE_DIR}/ulog
    ${CMAKE_CURRENT_SOURCE_DIR}/lwrb
)

target_link_libraries(utils PUBLIC stm32cubemx azrtos::threadx)
