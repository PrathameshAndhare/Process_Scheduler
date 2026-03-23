cmake_minimum_required(VERSION 3.16)
project(scheduling_algorithm_simulator_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(scheduler_cli
  src/main.cpp
  src/scheduling_algorithms.cpp
  src/metrics.cpp
)

target_include_directories(scheduler_cli PRIVATE src)

