cmake_minimum_required(VERSION 3.16)
project(YSEdit VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(ysedit src/main.cpp)

if(MSVC)
    target_compile_options(ysedit PRIVATE /W4)
else()
    target_compile_options(ysedit PRIVATE -Wall -Wextra -Wpedantic)
endif()

install(TARGETS ysedit RUNTIME DESTINATION bin)
