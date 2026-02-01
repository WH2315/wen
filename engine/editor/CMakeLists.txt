set(TARGET_NAME editor)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    organize_files("src")
    organize_files("include")
endif()

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE headers CONFIGURE_DEPENDS "include/*.hpp")

add_library(${TARGET_NAME} STATIC ${sources} ${headers})

target_include_directories(${TARGET_NAME} PUBLIC ./include)