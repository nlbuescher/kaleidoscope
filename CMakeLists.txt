cmake_minimum_required(VERSION 3.17)
project(kaleidoscope CXX)

set(CMAKE_CXX_STANDARD 17)

if (MSVC)
	add_compile_options(/permissive- /W4 /w14640)
else ()
	add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif ()

set(CMAKE_PREFIX_PATH /usr/local/opt/llvm)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

set(SOURCES
	src/main.cpp
	src/kaleidoscope/lexer.cpp
	src/kaleidoscope/parser.cpp
	src/kaleidoscope/compiler.cpp)

add_executable(kaleidoscope ${SOURCES})
llvm_config(kaleidoscope core native orcjit)
