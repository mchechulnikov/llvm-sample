cmake_minimum_required(VERSION 3.8)
project(llvm_sample)

set(CMAKE_CXX_STANDARD 17)

set(SOURCE_FILES main.cpp lexer.cpp lexer.h ast.h parser.cpp parser.h)
add_executable(llvm_sample ${SOURCE_FILES})