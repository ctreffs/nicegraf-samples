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
  set(EXECUTABLE_SUFFIX ".exe")
else()
  set(COMMON_COMPILE_OPTS "--std=c++14" "-Wall" "-Wconversion" "-Werror"
        "-Wno-unknown-pragmas")
  set(EXECUTABLE_SUFFIX "")
endif()

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nicegraf)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/nicegraf-shaderc)
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

file(GLOB shader_files ${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/hlsl/*.hlsl)
foreach(source_path ${shader_files})
  file(STRINGS ${source_path} tech_lines REGEX "// *T *: *([a-zA-Z0-9_]+)")
  if (tech_lines)
    set(tech_names "")
    foreach(tech_line ${tech_lines})
      string(REPLACE ":" ";" tmp ${tech_line})
      list(GET tmp 1 tmp)
      string(STRIP "${tmp}" tmp)
      string(REGEX REPLACE " +" ";" tmp ${tmp})
      list(GET tmp 0 tech_name)
      list(APPEND tech_names "${tech_name}")
    endforeach(tech_line)
    set(output_files_list "")
    foreach(tech ${tech_names})
      list(APPEND output_files_list "${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/generated/${tech}.vs.430.glsl")
      list(APPEND output_files_list "${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/generated/${tech}.vs.12.msl")
      list(APPEND output_files_list "${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/generated/${tech}.ps.430.glsl")
      list(APPEND output_files_list "${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/generated/${tech}.ps.12.msl")
      list(APPEND output_files_list "${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/generated/${tech}.pipeline")
    endforeach(tech)
    add_custom_command(OUTPUT ${output_files_list}
                       MAIN_DEPENDENCY ${source_path}
                       DEPENDS nicegraf_shaderc
                       COMMAND ${CMAKE_CURRENT_LIST_DIR}/nicegraf-shaderc/nicegraf_shaderc${EXECUTABLE_SUFFIX} ARGS ${source_path} "-t" "gl430" "-t" "msl12" "-O" "../generated/"
                       WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/artifacts/shaders/hlsl)
    set(generated_shaders_list "${output_files_list};${generated_shaders_list}")
  endif()
endforeach(source_path)
add_custom_target(generated_shaders DEPENDS ${generated_shaders_list})

function (add_sample number name)
  set(SAMPLE ${number}-${name})
  add_executable(${SAMPLE}
    "${CMAKE_CURRENT_LIST_DIR}/${number}-${name}/${name}.cpp")
  target_link_libraries(${SAMPLE} common ${NGF_OPENGL_LIB})
  target_compile_options(${SAMPLE} PRIVATE ${COMMON_COMPILE_OPTS})
  add_dependencies(${SAMPLE} generated_shaders)
  set_output_dir(${SAMPLE} "${CMAKE_CURRENT_LIST_DIR}/artifacts")
  set_target_properties(${SAMPLE} PROPERTIES PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/build")
endfunction(add_sample)

add_sample("00" "imgui")
add_sample("01" "full-screen-triangle")
add_sample("02" "spec-consts")
add_sample("03" "hexagon")
add_sample("04" "hexagon-indexed")
add_sample("05" "uniform-buffers")
add_sample("06" "depth")
add_sample("07" "simple-texture")
add_sample("08" "render-to-texture")
add_sample("09" "texture-sampling")
