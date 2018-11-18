/**
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
*/
#include <GLFW/glfw3.h>
#if defined(_WIN32) || defined(_WIN64)
#define GLFW_EXPOSE_NATIVE_WIN32
#define GET_GLFW_NATIVE_HANDLE glfwGetWin32Window
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#define GET_GLFW_NATIVE_HANDLE glfwGetCocoaWindow
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GET_GLFW_NATIVE_HANDLE glfwGetX11Window
#endif
#include <GLFW/glfw3native.h>
#include <assert.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <fstream>

#include "common.h"

void on_initialized(uintptr_t handle, uint32_t w, uint32_t h);

int main(int argc, char **argv) {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow *win =
      glfwCreateWindow(800, 600, "nicegraf sample", nullptr, nullptr);
  assert(win != nullptr);
  on_initialized(
      reinterpret_cast<uintptr_t>(GET_GLFW_NATIVE_HANDLE(win)), 800, 600);
  while (!glfwWindowShouldClose(win)) {
    glfwPollEvents();
  }
  return 0;
}

ngf::shader_stage load_shader_stage(const char *root_name,
                                    ngf_stage_type type) {
  static const char *stage_names[] = {
    "vert", "tesc", "tese", "geom", "frag"
  };
  std::string file_name =
      "shaders/" + std::string(root_name) + "." + stage_names[type] + ".glsl";
  std::ifstream fs(file_name);
  std::vector<char> content((std::istreambuf_iterator<char>(fs)),
                       std::istreambuf_iterator<char>());
  ngf_shader_stage_info stage_info;
  stage_info.type = type;
  stage_info.content = content.data();
  stage_info.content_length = (uint32_t)content.size();
  stage_info.debug_name = "";
  ngf::shader_stage stage;
  ngf_error err = stage.initialize(stage_info);
  assert(err == NGF_ERROR_OK);
  return stage;
}