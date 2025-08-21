file(GLOB SOURCES
    "*.c"
)

add_executable(irst ${SOURCES})
target_link_libraries(irst ${LIBS})

copy_config(irst)