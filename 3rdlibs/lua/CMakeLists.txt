# Official Lua 5.4.7 (vendored, https://www.lua.org/ftp/lua-5.4.7.tar.gz).
# Pure C, self-contained. We build the embeddable library only (exclude the
# standalone interpreter/compiler executables lua.c / luac.c).
file(GLOB lua_sources CONFIGURE_DEPENDS "src/*.c")
list(REMOVE_ITEM lua_sources
     "${CMAKE_CURRENT_SOURCE_DIR}/src/lua.c"
     "${CMAKE_CURRENT_SOURCE_DIR}/src/luac.c")

add_library(lua STATIC ${lua_sources})
target_include_directories(lua PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
