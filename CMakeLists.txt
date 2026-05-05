cmake_minimum_required(VERSION 4.2)
project(backend)

set(CMAKE_CXX_STANDARD 20)

add_executable(backend
        main.cpp
        src/core/Core.cpp
        src/core/Core.h
        src/core/Log.cpp
        src/core/Log.h
        src/routing/Router.cpp
        src/routing/Router.h
        src/utils/FileSystem.cpp
        src/utils/FileSystem.h
        src/lua/LuaBinding.cpp
        src/lua/LuaBinding.h
        src/lua/LuaObject.h
        src/core/HTTPObject.cpp
        src/core/HTTPObject.h
        src/template/Template.cpp
        src/template/Template.h
)

find_package(Lua REQUIRED)
target_link_libraries(backend PRIVATE ${LUA_LIBRARIES})

target_include_directories(backend PRIVATE ${LUA_INCLUDE_DIRS})