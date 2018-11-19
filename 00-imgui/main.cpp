/**
Copyright © 2018 nicegraf contributors
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

#include "common.h"
#include <nicegraf.h>
#include <nicegraf_wrappers.h>
#include <imgui.h>
#include <assert.h>
#include <stdint.h>

ngf::context nicegraf_context;
ngf::pass clear_pass;
ngf_render_target *default_rt;

// Called upon application initialization.
init_result on_initialized(uintptr_t native_handle,
                           uint32_t initial_width,
                           uint32_t initial_height) {
  ngf_error err = ngf_initialize(NGF_DEVICE_PREFERENCE_DONTCARE);
  assert(err == NGF_ERROR_OK);

  ngf_swapchain_info swapchain_info = {
    NGF_IMAGE_FORMAT_BGRA8, // color format
    NGF_IMAGE_FORMAT_UNDEFINED, // depth format (none)
    0, // number of MSAA samples (0, non-multisampled)
    2u, // swapchain capacity hint
    initial_width, // swapchain image width
    initial_height, // swapchain image height
    native_handle,
    NGF_PRESENTATION_MODE_IMMEDIATE, // turn off vsync
  };

  ngf_context_info ctx_info = {
    &swapchain_info, // swapchain_info
    nullptr, // shared_context (nullptr, no shared context)
    true     // debug
  };

  // Create a nicegraf context with the above parameters.
  err = nicegraf_context.initialize(ctx_info);
  assert(err == NGF_ERROR_OK);

  // Set the newly created context as current.
  err = ngf_set_context(nicegraf_context);
  assert(err == NGF_ERROR_OK);

  // Obtain the default render target.
  ngf_default_render_target(&default_rt);

  // Set up a render pass.
  ngf_attachment_load_op pass_load_op = NGF_LOAD_OP_CLEAR;
  ngf_clear_info clear_info;
  clear_info.clear_color[0] = 0.6f;
  clear_info.clear_color[1] = 0.7f;
  clear_info.clear_color[2] = 0.8f;
  clear_info.clear_color[3] = 1.0f;
  ngf_pass_info pass_info {
    &pass_load_op,
    1u,
    &clear_info
  };
  err = clear_pass.initialize(pass_info);
  assert(err == NGF_ERROR_OK);

  return { std::move(nicegraf_context), nullptr};
}

// Called every frame.
void on_frame(uint32_t w, uint32_t h, void*) {
  ngf_begin_frame(nicegraf_context.get());
  ngf_cmd_buffer *cmd_buf = nullptr;
  ngf_cmd_buffer_create(&cmd_buf);
  ngf_cmd_buffer_start(cmd_buf);
  ngf_cmd_begin_pass(cmd_buf, clear_pass, default_rt);
  ngf_cmd_end_pass(cmd_buf);
  ngf_cmd_buffer_end(cmd_buf);
  ngf_cmd_buffer_submit(1u, &cmd_buf);
  ngf_cmd_buffer_destroy(cmd_buf);
}

// Called every time the application has to dra an ImGUI overlay.
void on_ui(void*) {
  ImGui::ShowDemoWindow();
}

