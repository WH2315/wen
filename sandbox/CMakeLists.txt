project(sandbox)

ParseHeaders(
    sandbox           # TARGET_NAME
    ./                # INPUT_DIR
    ./auto_generated  # OUTPUT_DIR
)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS ./*.cpp)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE runtime)
target_precompile_headers(${PROJECT_NAME} REUSE_FROM runtime)

SolveDependency(sandbox)