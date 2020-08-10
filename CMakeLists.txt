cmake_minimum_required(VERSION 2.4)

project("oe-ray-tracer")
SET(CMAKE_CXX_STANDARD 17)

# game engine dependency
set(OE_BUILD_MODE 1)
add_subdirectory("src/vendor/engine")

# project
add_executable(${PROJECT_NAME} "src/main.cpp")
target_link_libraries(${PROJECT_NAME} PRIVATE "engine")
target_link_libraries(${PROJECT_NAME} PRIVATE "${engine_LIBS}")
target_include_directories(${PROJECT_NAME} PRIVATE "${engine_INCLUDE_DIRS}")