project(main)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE runtime)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM runtime)