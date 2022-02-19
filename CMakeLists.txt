cmake_minimum_required(VERSION 3.11.3)
project(cpp_ogl)

# set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")

include_directories(include)
file(GLOB SOURCES "src/*.cpp" "src/*.c")

add_executable(cpp_ogl ${SOURCES})

target_link_libraries(cpp_ogl PUBLIC freetype glfw ${CMAKE_DL_LIBS})
# For Udacity Workspace:
# target_link_libraries(cpp_ogl PUBLIC stdc++fs freetype glfw ${CMAKE_DL_LIBS})


set_property(TARGET cpp_ogl PROPERTY CXX_STANDARD 17)
target_compile_options(cpp_ogl PRIVATE -lglfw3, -lGL, -lX11, -lpthread, -lXrandr, -lXi, -ldl)

