/**
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
#include "imgui_ngf_backend.h"
#include <examples/imgui_impl_glfw.h>

void debugmsg_cb(const char *msg, const void*) {
  // TODO: surface these logs in a GUI console.
  printf("%s\n", msg);
}

init_result on_initialized(uintptr_t handle, uint32_t w, uint32_t h);
void on_frame(uint32_t w, uint32_t h, void *userdata);
void on_ui(void *userdata);
void on_shutdown(void *userdata);

// This is the "common main" for desktop apps.
int main(int argc, char **argv) {
  // Initialize GLFW.
  glfwInit();
  
  // Tell GLFW not to attempt to create an API context (nicegraf does it for
  // us).
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

  // Create the window.
  GLFWwindow *win =
      glfwCreateWindow(1024, 768, "nicegraf sample", nullptr, nullptr);
  assert(win != nullptr);

  // Notify the app.
  init_result init_data = on_initialized(
      reinterpret_cast<uintptr_t>(GET_GLFW_NATIVE_HANDLE(win)), 1024, 768);

#if !defined(NDEBUG)
  // Install debug message callback in debug mode only.
  ngf_debug_message_callback(nullptr, debugmsg_cb);
#endif

  // Initialize ImGUI.
  ImGui::SetCurrentContext(ImGui::CreateContext());

  // Style the controls.
  ImGui::StyleColorsLight();
  ImGuiStyle &gui_style = ImGui::GetStyle();
  gui_style.WindowRounding = 0.0f;
  gui_style.ScrollbarRounding = 0.0f;
  gui_style.FrameBorderSize = 1.0f;
  gui_style.ScrollbarSize = 20.0f;
  gui_style.WindowTitleAlign.x = 0.5f;

  ImGui_ImplGlfw_InitForOpenGL(win, true);

  ngf_imgui ui; // State of the ngf ImGUI backend.

  // Create a render pass for rendering the UI.
  ngf_attachment_load_op ui_layer_load_op = NGF_LOAD_OP_KEEP;
  ngf_pass_info ui_pass_info {
    &ui_layer_load_op,
    0u,
    nullptr
  };
  ngf::pass ui_pass;
  ui_pass.initialize(ui_pass_info);
 
  // Create a command buffer the UI rendering commands.
  ngf_cmd_buffer *uibuf = nullptr;
  ngf_cmd_buffer_create(&uibuf);

  // Obtain the default render target.
  ngf_render_target *defaultrt = nullptr;
  ngf_default_render_target(&defaultrt);

  while (!glfwWindowShouldClose(win)) { // Main loop.
    glfwPollEvents(); // Get input events.

    // Update renderable area size.
    int win_size_x = 0, win_size_y = 0;
    glfwGetFramebufferSize(win, &win_size_x, &win_size_y);

    // Notify application.
    on_frame(win_size_x, win_size_y, init_data.userdata);

    // Give application a chance to submit its UI drawing commands.
    // TODO: make toggleable.
    ImGui::GetIO().DisplaySize.x = (float)win_size_x;
    ImGui::GetIO().DisplaySize.y = (float)win_size_y;
    ImGui::NewFrame();
    ImGui_ImplGlfw_NewFrame();
    on_ui(init_data.userdata);
    // TODO: draw debug console window.

    // Draw the UI.
    ngf_cmd_buffer_start(uibuf);
    ngf_cmd_begin_pass(uibuf, ui_pass, defaultrt);
    ui.record_rendering_commands(uibuf);
    ngf_cmd_end_pass(uibuf);
    ngf_cmd_buffer_end(uibuf);
    ngf_cmd_buffer_submit(1u, &uibuf);

    // End frame.
    ngf_end_frame(init_data.context);
  }
  on_shutdown(init_data.userdata);
  glfwTerminate();
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
  stage_info.is_binary = false;
  ngf::shader_stage stage;
  ngf_error err = stage.initialize(stage_info);
  assert(err == NGF_ERROR_OK);
  return stage;
}