cmake_minimum_required(VERSION 3.15)
project(ecs)

find_package(raylib 3.0 REQUIRED) # Requires at least version 3.0

# set(CMAKE_C_STANDARD 11) # Requires C11 standard

# add_compile_options(-g -Wall -Wextra -Wuninitialized -Wmaybe-uninitialized)
add_compile_options(-fsanitize=address -g -O0)
add_link_options(-fsanitize=address)

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
    raylib
)
