cmake_minimum_required(VERSION 3.10)
project(MyNcursesApp C)

set(CMAKE_C_STANDARD 11)

# Find ncurses
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# Create executable
add_executable(my_app src/main.c)

# Link ncurses
target_link_libraries(my_app ${CURSES_LIBRARIES})
