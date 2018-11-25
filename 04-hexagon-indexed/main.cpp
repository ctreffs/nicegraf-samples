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

#include "common.h"
#include <nicegraf.h>
#include <nicegraf_util.h>
#include <nicegraf_wrappers.h>
#include <imgui.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

/** 
    These samples do not use PI on principle.
    https://tauday.com/tau-manifesto
*/
constexpr double TAU = 6.28318530718;

struct app_state {
  ngf::render_target default_rt;
  ngf::shader_stage vert_stage;
  ngf::shader_stage frag_stage;
  ngf::graphics_pipeline pipeline;
  ngf::buffer vert_buffer;
  ngf::buffer index_buffer;
};

struct vertex_data {
  float position[2];
  float color[3];
};

// Called upon application initialization.
init_result on_initialized(uintptr_t native_handle,
                           uint32_t initial_width,
                           uint32_t initial_height) {
  app_state *state = new app_state;

  ngf::context ctx = create_default_context(native_handle,
                                            initial_width, initial_height);

  // Obtain the default render target.
  ngf_clear clear;
  clear.clear_color[0] = 0.0f;
  clear.clear_color[1] = 0.0f;
  clear.clear_color[2] = 0.0f;
  clear.clear_color[3] = 0.0f;
  ngf_render_target *rt;
  ngf_error err = ngf_default_render_target(NGF_LOAD_OP_CLEAR,
                                            NGF_LOAD_OP_DONTCARE,
                                            &clear, NULL, &rt);
  assert(err == NGF_ERROR_OK);
  state->default_rt = ngf::render_target(rt);

  // Load shader stages.
  state->vert_stage =
      load_shader_stage("hexagon", NGF_STAGE_VERTEX);
  state->frag_stage =
      load_shader_stage("hexagon", NGF_STAGE_FRAGMENT);

  // Initial pipeline configuration with OpenGL-style defaults.
  ngf_util_graphics_pipeline_data pipeline_data;
  ngf_util_create_default_graphics_pipeline_data(nullptr,
                                                 &pipeline_data);
  ngf_graphics_pipeline_info &pipe_info = pipeline_data.pipeline_info;

  // Pipeline configuration.
  // Shader stages.
  pipe_info.nshader_stages = 2u; 
  pipe_info.shader_stages[0] = state->vert_stage.get();
  pipe_info.shader_stages[1] = state->frag_stage.get();
  // Vertex input.
  // First, attribute descriptions.
  // There will be two vertex attributes - for position and color.
  ngf_vertex_input_info &vert_info = pipeline_data.vertex_input_info;
  vert_info.nattribs = 2u;
  ngf_vertex_attrib_desc attribs[2] = {
    {0u, 0u, 0u, NGF_TYPE_FLOAT, 2u, false},
    {1u, 0u, offsetof(vertex_data, color), NGF_TYPE_FLOAT, 3u, false},
  };
  vert_info.attribs = attribs;
  // Next, configure bindings.
  vert_info.nvert_buf_bindings = 1u;
  ngf_vertex_buf_binding_desc binding;
  binding.binding = 0u;
  binding.input_rate = NGF_INPUT_RATE_VERTEX;
  binding.stride = sizeof(vertex_data);
  vert_info.vert_buf_bindings = &binding;
  // Enable multisampling for anti-aliasing.
  pipeline_data.multisample_info.multisample = true;
  // Done configuring, initialize the pipeline.
  err = state->pipeline.initialize(pipe_info);
  assert(err == NGF_ERROR_OK);

  // Create the vertex data buffer.
  ngf_buffer_info buf_info {
    7u * sizeof(vertex_data),
    NGF_BUFFER_TYPE_VERTEX,
    NGF_BUFFER_USAGE_STATIC,
    NGF_BUFFER_ACCESS_DRAW
  };
  err = state->vert_buffer.initialize(buf_info);
  assert(err == NGF_ERROR_OK);

  // Create index data buffer.
  buf_info.size = 3u * 6u * sizeof(uint16_t);
  buf_info.type = NGF_BUFFER_TYPE_INDEX;
  err = state->index_buffer.initialize(buf_info);
  assert(err == NGF_ERROR_OK);

  // Populate vertex buffer with data.
  vertex_data vertices[7u] = {
      {  // First vertex is the center of the hexagon.
          {0.0f, 0.0f},
          {1.0f, 1.0f, 1.0f}
      }
  };
  for (uint32_t v = 1u; v <= 6u; ++v) {
    vertex_data &vertex = vertices[v];
    vertex.position[0] = 0.5f * (float)cos((v - 1u) * TAU / 6.0f);
    vertex.position[1] = 0.5f * (float)sin((v - 1u) * TAU / 6.0f);
    vertex.color[0] = 0.5f*(vertex.position[0] + 1.0f);
    vertex.color[1] = 0.5f*(vertex.position[1] + 1.0f);
    vertex.color[2] = 1.0f - vertex.position[0];
  }
  ngf_populate_buffer(state->vert_buffer.get(),
                      0u, sizeof(vertices), vertices);

  // Populate index buffer with data.
  uint16_t indices[3u * 6u];
  for (uint16_t t = 0u; t < 6u; ++t) {
    indices[3u*t + 0u] =  0;
    indices[3u*t + 1u] = (t + 1u) % 7u;
    indices[3u*t + 2u] = (t + 2u >= 7u) ? 1u : (t + 2u);
  }
  ngf_populate_buffer(state->index_buffer.get(), 0u, sizeof(indices), indices);

  return { std::move(ctx), state};
}

// Called every frame.
void on_frame(uint32_t w, uint32_t h, void *userdata) {
  app_state *state = (app_state*)userdata;
  ngf_irect2d viewport { 0, 0, w, h };
  ngf_cmd_buffer *cmd_buf = nullptr;
  ngf_cmd_buffer_info cmd_info;
  ngf_create_cmd_buffer(&cmd_info, &cmd_buf);
  ngf_start_cmd_buffer(cmd_buf);
  ngf_cmd_begin_pass(cmd_buf, state->default_rt);
  ngf_cmd_bind_pipeline(cmd_buf, state->pipeline);
  ngf_cmd_bind_vertex_buffer(cmd_buf, state->vert_buffer, 0u, 0u);
  ngf_cmd_bind_index_buffer(cmd_buf, state->index_buffer, NGF_TYPE_UINT16);
  ngf_cmd_viewport(cmd_buf, &viewport);
  ngf_cmd_scissor(cmd_buf, &viewport);
  ngf_cmd_draw(cmd_buf, true, 0u, 3u * 6u, 1u); 
  ngf_cmd_end_pass(cmd_buf);
  ngf_end_cmd_buffer(cmd_buf);
  ngf_submit_cmd_buffer(1u, &cmd_buf);
  ngf_destroy_cmd_buffer(cmd_buf);
}

// Called every time the application has to dra an ImGUI overlay.
void on_ui(void*) {}

// Called when the app is about to close.
void on_shutdown(void *userdata) {
  delete (app_state*)userdata;
}
