cmake_minimum_required(VERSION 3.17)
project(space_invaders)

set(CMAKE_CXX_STANDARD 17)

find_package(SFML 2.5 COMPONENTS system window graphics REQUIRED)

add_executable(space_invaders src/main.cpp)

target_link_libraries(space_invaders sfml-system sfml-window sfml-graphics)