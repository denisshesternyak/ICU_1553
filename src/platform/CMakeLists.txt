file(GLOB SOURCES
    "*.c"
)

add_executable(platform ${SOURCES})
target_link_libraries(platform ${LIBS})