file(GLOB SOURCES
    "*.c"
    "logger.c"
)

add_executable(icu ${SOURCES})
target_link_libraries(icu ${LIBS})

copy_config(icu)