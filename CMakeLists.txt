#[[
Copyright © 2018 nicegraf contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]

cmake_minimum_required(VERSION 3.0.2)

project(nicegraf-samples)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nicegraf)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/glfw)

add_library(imgui
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui/imgui.cpp)
  
target_include_directories(imgui PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui)

set(NGF_SAMPLES_COMMON_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/common/common.cpp
  ${CMAKE_CURRENT_LIST_DIR}/common/common.h
  ${CMAKE_CURRENT_LIST_DIR}/common/imgui_ngf_backend.cpp)

add_library(common
  ${NGF_SAMPLES_COMMON_SOURCES})
 
target_include_directories(common PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/common)
  
target_link_libraries(common glfw imgui nicegraf nicegraf_util)

if(WIN32)
  set(NGF_OPENGL_LIB "opengl32.lib")
elseif(UNIX AND NOT APPLE)
  set(NGF_OPENGL_LIB "gl")
endif()

function(set_output_dir target dir)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${dir}")
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${dir}")
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${dir}")
endfunction(set_output_dir)

function (add_sample name)
  set(SAMPLE ${name})
  add_executable(${SAMPLE}
    ${CMAKE_CURRENT_LIST_DIR}/${name}/main.cpp)
  target_link_libraries(${SAMPLE} common ${NGF_OPENGL_LIB})
  set_output_dir(${SAMPLE} "${CMAKE_CURRENT_LIST_DIR}/artifacts")
endfunction(add_sample)

add_sample("00-imgui")