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
llvm_map_components_to_libnames(LLVM_LIBRARIES core)

set(SOURCES
	src/main.cpp
	src/kaleidoscope/lexer.cpp
	src/kaleidoscope/parser.cpp
	src/kaleidoscope/compiler.cpp)

add_executable(kaleidoscope ${SOURCES})

target_link_libraries(kaleidoscope ${LLVM_LIBRARIES})
