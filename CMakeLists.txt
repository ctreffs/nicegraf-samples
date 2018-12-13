#[[
Copyright (c) 2018 nicegraf contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
]]

cmake_minimum_required(VERSION 3.0.2)

project(nicegraf-samples)

if (WIN32)
  set(COMMON_COMPILE_OPTS "/W4" "/WX")
else()
  set(COMMON_COMPILE_OPTS "--std=c++14" "-Wall" "-Wconversion" "-Werror"
        "-Wno-unknown-pragmas")
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nicegraf)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/third_party/glfw)

add_library(imgui
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui/imgui.cpp
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui/imgui_draw.cpp
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui/imgui_widgets.cpp
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui/imgui_demo.cpp
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui/examples/imgui_impl_glfw.cpp)

target_include_directories(imgui PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/third_party/imgui)

target_compile_options(imgui PRIVATE ${COMMON_COMPILE_OPTS})

target_link_libraries(imgui glfw)
  
set(NGF_SAMPLES_COMMON_SOURCES
  ${CMAKE_CURRENT_LIST_DIR}/common/common.cpp
  ${CMAKE_CURRENT_LIST_DIR}/common/common.h
  ${CMAKE_CURRENT_LIST_DIR}/common/imgui_ngf_backend.h
  ${CMAKE_CURRENT_LIST_DIR}/common/imgui_ngf_backend.cpp)

if(APPLE)
  add_definitions(-DNGF_BACKEND_METAL)
  set(NGF_SAMPLES_COMMON_SOURCES ${NGF_SAMPLES_COMMON_SOURCES}
      ${CMAKE_CURRENT_LIST_DIR}/common/get_glfw_contentview.mm)
else()
  add_definitions(-DNGF_BACKEND_OPENGL)
endif()

add_library(common
  ${NGF_SAMPLES_COMMON_SOURCES})
 
target_include_directories(common PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/common)
  
target_compile_options(common PRIVATE ${COMMON_COMPILE_OPTS})


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
  target_compile_options(${SAMPLE} PRIVATE ${COMMON_COMPILE_OPTS})
  set_output_dir(${SAMPLE} "${CMAKE_CURRENT_LIST_DIR}/artifacts")
endfunction(add_sample)

add_sample("00-imgui")
add_sample("01-full-screen-triangle")
add_sample("02-spec-consts")
add_sample("03-hexagon")
add_sample("04-hexagon-indexed")
add_sample("05-uniform-buffers")
add_sample("06-depth")
add_sample("07-simple-texture")
