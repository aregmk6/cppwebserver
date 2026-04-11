cmake_minimum_required(VERSION 4.3)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(graphics-programming C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED	True)

project(MyWebServer C CXX)

add_executable(server ${CMAKE_SOURCE_DIR}/src/main.cpp)

target_include_directories(server
		PRIVATE ${CMAKE_SOURCE_DIR}/include
)


add_subdirectory(${CMAKE_SOURCE_DIR}/src)
