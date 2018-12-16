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
#define _CRT_SECURE_NO_WARNINGS
#include "common.h"
#include <nicegraf.h>
#include <nicegraf_util.h>
#include <nicegraf_wrappers.h>
#include <imgui.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct app_state {
  ngf::render_target default_rt;
  ngf::shader_stage vert_stage;
  ngf::shader_stage frag_stage;
  ngf::graphics_pipeline pipeline;
  ngf::image image;
  ngf::sampler sampler;
  ngf::descriptor_set_layout set_layout;
  ngf::descriptor_set desc_set;
};

// Called upon application initialization.
init_result on_initialized(uintptr_t native_handle,
                           uint32_t initial_width,
                           uint32_t initial_height) {
  app_state *state = new app_state;
   
  ngf::context ctx = create_default_context(native_handle,
                                            initial_width, initial_height);

  // Set up a render pass.
  ngf_clear clear;
  clear.clear_color[0] = 0.6f;
  clear.clear_color[1] = 0.7f;
  clear.clear_color[2] = 0.8f;
  clear.clear_color[3] = 1.0f;
  
  // Obtain the default render target.
  ngf_render_target *rt;
  ngf_error err =
      ngf_default_render_target(NGF_LOAD_OP_CLEAR, NGF_LOAD_OP_DONTCARE,
                                &clear, NULL, &rt);
  assert(err == NGF_ERROR_OK);
  state->default_rt = ngf::render_target(rt);

  // Load shader stages.
  state->vert_stage =
      load_shader_stage("fullscreen-triangle", NGF_STAGE_VERTEX);
  state->frag_stage =
      load_shader_stage("simple-texture", NGF_STAGE_FRAGMENT);

  // Initial pipeline configuration with OpenGL-style defaults.
  ngf_util_graphics_pipeline_data pipeline_data;
  ngf_util_create_default_graphics_pipeline_data(nullptr,
                                                 &pipeline_data);
  ngf_graphics_pipeline_info &pipe_info = pipeline_data.pipeline_info;
  pipe_info.nshader_stages = 2u;
  pipe_info.shader_stages[0] = state->vert_stage.get();
  pipe_info.shader_stages[1] = state->frag_stage.get();
  pipe_info.compatible_render_target = state->default_rt.get();
  
  // Create a simple pipeline layout.
  ngf_descriptor_info desc_info {
    NGF_DESCRIPTOR_TEXTURE_AND_SAMPLER,
    0u,
    NGF_DESCRIPTOR_FRAGMENT_STAGE_BIT
  };
  err = ngf_util_create_simple_layout(&desc_info, 1u,
                                      &pipeline_data.layout_info);
  assert(err == NGF_ERROR_OK);
  state->set_layout.reset(pipeline_data.layout_info.descriptors_layouts[0]);
  err = state->pipeline.initialize(pipe_info);
  assert(err == NGF_ERROR_OK);

  // Create the image.
  const ngf_extent3d img_size { 512u, 512u, 1u };
  const ngf_image_info img_info {
    NGF_IMAGE_TYPE_IMAGE_2D,
    img_size,
    1u,
    NGF_IMAGE_FORMAT_RGBA8,
    0u,
    NGF_IMAGE_USAGE_SAMPLE_FROM
  };
  err = state->image.initialize(img_info);
  assert(err == NGF_ERROR_OK);

  // Populate image with data.
  FILE *image = fopen("textures/LENA.DATA", "rb");
  assert(image != NULL);
  fseek(image, 0, SEEK_END);
  const uint32_t image_data_size = (uint32_t)ftell(image);
  fseek(image, 0, SEEK_SET);
  uint8_t *image_data = new uint8_t[image_data_size];
  size_t read_bytes = fread(image_data, 1, image_data_size, image);
  assert(read_bytes == 512u * 512u * 4u);
  fclose(image);
  err = ngf_populate_image(state->image.get(), 0u, {0u, 0u, 0u},
                           {512u, 512u, 1u}, image_data);
  delete[] image_data;
  assert(err == NGF_ERROR_OK);

  // Create sampler.
  ngf_sampler_info samp_info {
    NGF_FILTER_LINEAR,
    NGF_FILTER_LINEAR,
    NGF_FILTER_NEAREST,
    NGF_WRAP_MODE_CLAMP_TO_EDGE,
    NGF_WRAP_MODE_CLAMP_TO_EDGE,
    NGF_WRAP_MODE_CLAMP_TO_EDGE,
    0.0f,
    0.0f,
    0.0f,
    {0.0f}
  };
  err = state->sampler.initialize(samp_info);
  assert(err == NGF_ERROR_OK);

  // Create and write to the descriptor set.
  state->desc_set.initialize(*state->set_layout.get());
  ngf_descriptor_write write_op;
  write_op.type = NGF_DESCRIPTOR_TEXTURE_AND_SAMPLER;
  write_op.binding = 0u;
  write_op.op.image_sampler_bind.image_subresource.image = state->image.get();
  write_op.op.image_sampler_bind.image_subresource.layered = false;
  write_op.op.image_sampler_bind.image_subresource.layer = 0u;
  write_op.op.image_sampler_bind.image_subresource.mip_level = 0u;
  write_op.op.image_sampler_bind.sampler = state->sampler.get();
  err = ngf_apply_descriptor_writes(&write_op, 1u, state->desc_set.get());
  assert(err == NGF_ERROR_OK);

  return { std::move(ctx), state};
}

// Called every frame.
void on_frame(uint32_t w, uint32_t h, float, void *userdata) {
  app_state *state = (app_state*)userdata;
  ngf_irect2d viewport { 0, 0, w, h };
  ngf_cmd_buffer *cmd_buf = nullptr;
  ngf_cmd_buffer_info cmd_info;
  ngf_create_cmd_buffer(&cmd_info, &cmd_buf);
  ngf_start_cmd_buffer(cmd_buf);
  ngf_cmd_begin_pass(cmd_buf, state->default_rt);
  ngf_cmd_bind_pipeline(cmd_buf, state->pipeline);
  ngf_cmd_viewport(cmd_buf, &viewport);
  ngf_cmd_scissor(cmd_buf, &viewport);
  ngf_cmd_bind_descriptor_set(cmd_buf, state->desc_set, 0u);
  ngf_cmd_draw(cmd_buf, false, 0u, 3u, 1u); 
  ngf_cmd_end_pass(cmd_buf);
  ngf_end_cmd_buffer(cmd_buf);
  ngf_submit_cmd_buffer(1u, &cmd_buf);
  ngf_destroy_cmd_buffer(cmd_buf);
}

// Called every time the application has to dra an ImGUI overlay.
void on_ui(void*) { }

// Called when the app is about to close.
void on_shutdown(void *userdata) {
  delete (app_state*)userdata;
}
