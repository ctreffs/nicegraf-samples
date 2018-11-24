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
#define GET_GLFW_NATIVE_HANDLE(w) glfwGetWin32Window(w)
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#include "get_glfw_contentview.h"
#define GET_GLFW_NATIVE_HANDLE(w) get_glfw_contentview(w)
#else
#define GLFW_EXPOSE_NATIVE_X11
#define GET_GLFW_NATIVE_HANDLE(w) glfwGetX11Window(w)
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
int main(int, char **) {
  // Initialize GLFW.
  glfwInit();
 
  // Initialize nicegraf.
  ngf_error err = ngf_initialize(NGF_DEVICE_PREFERENCE_DONTCARE);
  assert(err == NGF_ERROR_OK);

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

  // Initialize ImGUI.
  //ImGui::SetCurrentContext(ImGui::CreateContext());

  // Style the controls.
  /*

  ImGui::StyleColorsLight();
  ImGuiStyle &gui_style = ImGui::GetStyle();
  gui_style.WindowRounding = 0.0f;
  gui_style.ScrollbarRounding = 0.0f;
  gui_style.FrameBorderSize = 1.0f;
  gui_style.ScrollbarSize = 20.0f;
  gui_style.WindowTitleAlign.x = 0.5f;

  ImGui_ImplGlfw_InitForOpenGL(win, true);
*/
  //ngf_imgui ui; // State of the ngf ImGUI backend.

  // Create a command buffer the UI rendering commands.
  ngf::cmd_buffer uibuf;
  ngf_cmd_buffer_info uibuf_info {0u};
  uibuf.initialize(uibuf_info);

  // Obtain the default render target.
  ngf_render_target *defaultrt = nullptr;
  ngf_default_render_target(NGF_LOAD_OP_KEEP,
                            NGF_LOAD_OP_DONTCARE,
                            NULL,
                            NULL,
                            &defaultrt);

  while (!glfwWindowShouldClose(win)) { // Main loop.
    glfwPollEvents(); // Get input events.

    // Update renderable area size.
    int win_size_x = 0, win_size_y = 0;
    glfwGetFramebufferSize(win, &win_size_x, &win_size_y);

    ngf_begin_frame(init_data.context);

    // Notify application.
    on_frame((uint32_t)win_size_x, (uint32_t)win_size_y, init_data.userdata);

    // Give application a chance to submit its UI drawing commands.
    // TODO: make toggleable.
    //ImGui::GetIO().DisplaySize.x = (float)win_size_x;
    //ImGui::GetIO().DisplaySize.y = (float)win_size_y;
    //ImGui::NewFrame();
    //ImGui_ImplGlfw_NewFrame();
    on_ui(init_data.userdata);
    // TODO: draw debug console window.

    // Draw the UI.
    ngf_start_cmd_buffer(uibuf);
    ngf_cmd_begin_pass(uibuf, defaultrt);
    //ui.record_rendering_commands(uibuf);
    ngf_cmd_end_pass(uibuf);
    ngf_end_cmd_buffer(uibuf);
    ngf_cmd_buffer *b = uibuf.get();
    ngf_submit_cmd_buffer(1u, &b);

    // End frame.
    ngf_end_frame(init_data.context);
  }
  ngf_destroy_render_target(defaultrt);
  on_shutdown(init_data.userdata);
  glfwTerminate();
  return 0;
}

#if defined(NGF_BACKEND_OPENGL)
#define SHADER_EXTENSION ".glsl"
#elif defined(NGF_BACKEND_VULKAN)
#define SHADER_EXTENSION ".spv"
#else
#define SHADER_EXTENSION ".msl"
#endif

ngf::shader_stage load_shader_stage(const char *root_name,
                                    ngf_stage_type type) {
  static const char *stage_names[] = {
    "vert", "tesc", "tese", "geom", "frag"
  };
  std::string file_name =
      "shaders/" + std::string(root_name) + "." + stage_names[type] +
       SHADER_EXTENSION;
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

ngf::context create_default_context(uintptr_t handle, uint32_t w, uint32_t h) {
  // Create a nicegraf context.
  ngf_swapchain_info swapchain_info = {
    NGF_IMAGE_FORMAT_BGRA8, // color format
    NGF_IMAGE_FORMAT_UNDEFINED, // depth format (none)
    8u, // MSAA 8x
    2u, // swapchain capacity hint
    w, // swapchain image width
    h, // swapchain image height
    handle,
    NGF_PRESENTATION_MODE_FIFO, // turn off vsync
  };
  ngf_context_info ctx_info = {
    &swapchain_info, // swapchain_info
    nullptr, // shared_context (nullptr, no shared context)
#if !defined(NDEBUG)
    true     // debug
#else
    false
#endif
  };
  ngf::context nicegraf_context;
  ngf_error err = nicegraf_context.initialize(ctx_info);
  assert(err == NGF_ERROR_OK);

  // Set the newly created context as current.
  err = ngf_set_context(nicegraf_context);
  assert(err == NGF_ERROR_OK);

#if !defined(NDEBUG)
  // Install debug message callback in debug mode only.
  ngf_debug_message_callback(nullptr, debugmsg_cb);
#endif

  return std::move(nicegraf_context);
}
