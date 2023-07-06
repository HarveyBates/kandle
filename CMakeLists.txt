cmake_minimum_required(VERSION 3.19.0)

# Set C standard to use
set(CMAKE_CXX_STANDARD 17)

project("kandle")

# All files in source directory
file(GLOB_RECURSE SRC_DIR RELATIVE ${CMAKE_SOURCE_DIR} src/*.cpp)

# All files in include directory
file(GLOB_RECURSE INC_DIR RELATIVE ${CMAKE_SOURCE_DIR} include/*.hpp)

# Set output directories
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Specify executable
add_executable(${PROJECT_NAME} ${SRC_DIR} ${INC_DIR})

# Add flags to compilation
target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
target_compile_options(${PROJECT_NAME} PRIVATE -pedantic)
# target_compile_options(${PROJECT_NAME} PRIVATE -Werror)

# Colored output for logs
target_compile_options(${PROJECT_NAME} PRIVATE -DLOG_USE_COLOR)

# Set include directories and those required by find_package()
include_directories(${CMAKE_SOURCE_DIR}/include)

# Link required libraries if specified in find_package()
target_link_libraries(${PROJECT_NAME})

# Optional, install to /usr/local/bin/kandle (UNIX) or Program Files (Windows)
install(TARGETS ${PROJECT_NAME})
