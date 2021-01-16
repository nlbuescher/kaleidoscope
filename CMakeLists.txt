cmake_minimum_required(VERSION 3.17)
project(kaleidoscope CXX)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
	add_compile_options(/permissive- /W4 /w14640)
else ()
	add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif ()

set(SOURCES
	src/main.cpp
	src/kaleidoscope/lexer.cpp)

add_executable(kaleidoscope ${SOURCES})
