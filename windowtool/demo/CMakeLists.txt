cmake_minimum_required(VERSION 3.0)

set(PROJECT_NAME "windowloader")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall")

#Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

project(${PROJECT_NAME})

include_directories(SYSTEM ${Qt5Core_PRIVATE_INCLUDE_DIRS})

FILE(GLOB SC_FILES "*.cpp" "*.h")

add_executable(${PROJECT_NAME} ${SC_FILES})

target_link_libraries(${PROJECT_NAME}  dde-windowtool png)
