cmake_minimum_required(VERSION 3.10)
project(My1553Projects C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -g")

include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/src)
link_directories(${CMAKE_SOURCE_DIR}/lib)

set(LIBS
    Exc1553Px
    Exc4000
    exchost2dpr
    dl
    rt
    pthread
    ftd2
    ftd3
    stdc++
)

function(copy_config target)
    add_custom_command(
        TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_CURRENT_SOURCE_DIR}/config.ini
                $<TARGET_FILE_DIR:${target}>
    )
endfunction()

add_subdirectory(src/platform)
add_subdirectory(src/icu)
add_subdirectory(src/irst)