cmake_minimum_required(VERSION 3.10)
project(PerCoreEventBus VERSION 1.0 LANGUAGES CXX)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Threading support
find_package(Threads REQUIRED)

# HDR Histogram support
find_package(hdr_histogram REQUIRED)

# Common include directory (current dir)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# Release flags for high performance
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")

# 1. Generator Target
add_executable(generator generator.cpp)
target_link_libraries(generator PRIVATE Threads::Threads hdr_histogram::hdr_histogram)

# 2. Receiver Target
add_executable(receiver receiver.cpp)
target_link_libraries(receiver PRIVATE Threads::Threads)

# 3. Benchmark Target
add_executable(bench_ring_buffer bench_ring_buffer.cpp)
target_link_libraries(bench_ring_buffer PRIVATE Threads::Threads)

# Optional: Add testing target later
# if(BUILD_TESTING)
#   add_executable(test_ring_buffer test_ring_buffer.cpp)
#   target_link_libraries(test_ring_buffer PRIVATE Threads::Threads)
# endif()
