cmake_minimum_required(VERSION 3.4.3)
project(analyzer VERSION 0.1.0 LANGUAGES CXX)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs BitWriter Core IPO IrReader InstCombine Instrumentation Target Linker Analysis ScalarOpts Support)
add_executable(analyzer analyzer.cpp)
target_link_libraries(analyzer ${llvm_libs})
