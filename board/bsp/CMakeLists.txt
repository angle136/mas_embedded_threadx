# bsp library
add_library(bsp OBJECT
    bsp_init.c
)

target_include_directories(bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bsp PUBLIC stm32cubemx azrtos::threadx utils)
