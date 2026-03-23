cmake_minimum_required(VERSION 3.16)
project(scheduling_algorithm_simulator_gui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

add_executable(scheduler_gui
  src/main.cpp
  src/MainWindow.cpp
  src/GanttWidget.cpp
  ../cpp/src/scheduling_algorithms.cpp
  ../cpp/src/metrics.cpp
)

target_include_directories(scheduler_gui PRIVATE
  src
  ../cpp/src
)

target_link_libraries(scheduler_gui PRIVATE Qt6::Widgets)

