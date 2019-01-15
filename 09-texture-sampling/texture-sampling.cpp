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
  ngf::sampler bilinear_sampler;
  ngf::sampler nearest_sampler;
  ngf::uniform_buffer ubo;
};

struct uniform_data {
  float w, h, x, y;
  uint8_t pad[240];
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
  clear.clear_color[0] =
  clear.clear_color[1] =
  clear.clear_color[2] =
  clear.clear_color[3] = 0.0f;
  
  // Obtain the default render target.
  ngf_render_target *rt;
  ngf_error err =
      ngf_default_render_target(NGF_LOAD_OP_CLEAR, NGF_LOAD_OP_DONTCARE,
                                &clear, NULL, &rt);
  assert(err == NGF_ERROR_OK);
  state->default_rt = ngf::render_target(rt);

  // Load shader stages.
  state->vert_stage =
      load_shader_stage("textured-quad", "VSMain", NGF_STAGE_VERTEX);
  state->frag_stage =
      load_shader_stage("textured-quad", "PSMain", NGF_STAGE_FRAGMENT);

  // Initial pipeline configuration with OpenGL-style defaults.
  ngf_util_graphics_pipeline_data pipeline_data;
  ngf_util_create_default_graphics_pipeline_data(nullptr,
                                                 &pipeline_data);
  ngf_graphics_pipeline_info &pipe_info = pipeline_data.pipeline_info;
  pipe_info.nshader_stages = 2u;
  pipe_info.shader_stages[0] = state->vert_stage.get();
  pipe_info.shader_stages[1] = state->frag_stage.get();
  pipe_info.compatible_render_target = state->default_rt.get();

  // Create pipeline layout from metadata.
  plmd *pipeline_metadata = load_pipeline_metadata("textured-quad");
  assert(pipeline_metadata);
  err = ngf_util_create_pipeline_layout_from_metadata(
      ngf_plmd_get_layout(pipeline_metadata),
      &pipeline_data.layout_info);
  assert(err == NGF_ERROR_OK);
  assert(pipeline_data.layout_info.ndescriptor_set_layouts == 2);
  pipe_info.image_to_combined_map =
      ngf_plmd_get_image_to_cis_map(pipeline_metadata);
  pipe_info.sampler_to_combined_map =
      ngf_plmd_get_sampler_to_cis_map(pipeline_metadata);
  err = state->pipeline.initialize(pipe_info);
  assert(err == NGF_ERROR_OK);

  // Create the image.
  const ngf_extent3d img_size { 512u, 512u, 1u };
  const ngf_image_info img_info {
    NGF_IMAGE_TYPE_IMAGE_2D,
    img_size,
    10u,
    NGF_IMAGE_FORMAT_RGBA8,
    0u,
    NGF_IMAGE_USAGE_SAMPLE_FROM
  };
  err = state->image.initialize(img_info);
  assert(err == NGF_ERROR_OK);

  // Populate image with data.
  char file_name[] = "textures/LENA0.DATA";
  uint32_t w = 512u, h = 512u;
  for (uint32_t mip_level = 0u;  mip_level < 9u; ++mip_level) {
    snprintf(file_name, sizeof(file_name), "textures/LENA%d.DATA", mip_level);
    std::vector<char> data = load_raw_data(file_name);
    err = ngf_populate_image(state->image.get(), mip_level, {0u, 0u, 0u},
                             {w, h, 1u}, data.data());
    assert(err == NGF_ERROR_OK);
    w = w >> 1; h = h >> 1;
  }
  
  // Create samplers.
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
  err = state->bilinear_sampler.initialize(samp_info);
  assert(err == NGF_ERROR_OK);
  samp_info.min_filter = samp_info.mag_filter = NGF_FILTER_NEAREST;
  err = state->nearest_sampler.initialize(samp_info);
  assert(err == NGF_ERROR_OK);

  // Create the uniform buffer.
  ngf_uniform_buffer_info ubo_info = {
    sizeof(uniform_data) * 3u
  };
  err = state->ubo.initialize(ubo_info);
  assert(err == NGF_ERROR_OK);

  return { std::move(ctx), state};
}

void draw_textured_quad(const ngf_uniform_buffer *ubo,
                        size_t buffer_offset,
                        const ngf_sampler *sampler,
                        ngf_cmd_buffer *cmd_buf) {
  ngf_resource_bind_op bind_ops[2];
  bind_ops[0].type = NGF_DESCRIPTOR_SAMPLER;
  bind_ops[0].target_set = 1u;
  bind_ops[0].target_binding = 1u;
  bind_ops[0].info.image_sampler.sampler = sampler;
  bind_ops[1].type = NGF_DESCRIPTOR_UNIFORM_BUFFER;
  bind_ops[1].target_set = 1u;
  bind_ops[1].target_binding = 0u;
  bind_ops[1].info.uniform_buffer.buffer = ubo;
  bind_ops[1].info.uniform_buffer.offset = buffer_offset * sizeof(uniform_data);
  bind_ops[1].info.uniform_buffer.range= sizeof(uniform_data);
  ngf_cmd_bind_resources(cmd_buf, bind_ops, 2u);
  ngf_cmd_draw(cmd_buf, false, 0u, 6u, 1u);
}

// Called every frame.
void on_frame(uint32_t w, uint32_t h, float, void *userdata) {
  static uint32_t old_w = 0u, old_h = 0u;
  app_state *state = (app_state*)userdata;
  if (old_w != w || old_h != h) {
    const uniform_data uniform_data[3] = {
      {512.0f / w, 512.0f / h, (512.0f - w)/float(w), (h - 512.0f) / float(h)},
      {340.0f / w, 340.0f / h, (1364.0f - w)/float(w),(h - 340.0f) / float(h)},
      {340.0f / w, 340.0f / h, (1364.0f - w)/float(w),(h - 1020.0f) / float(h)},
    };
    ngf_write_uniform_buffer(state->ubo, uniform_data, sizeof(uniform_data));
    old_w = w; old_h = h;
  }
  ngf_irect2d viewport { 0, 0, w, h };
  ngf_cmd_buffer *cmd_buf = nullptr;
  ngf_cmd_buffer_info cmd_info;
  ngf_create_cmd_buffer(&cmd_info, &cmd_buf);
  ngf_start_cmd_buffer(cmd_buf);
  ngf_cmd_begin_pass(cmd_buf, state->default_rt);
  ngf_cmd_bind_pipeline(cmd_buf, state->pipeline);
  ngf_cmd_viewport(cmd_buf, &viewport);
  ngf_cmd_scissor(cmd_buf, &viewport);
  ngf_resource_bind_op texture_bind_op;
  texture_bind_op.type = NGF_DESCRIPTOR_TEXTURE;
  texture_bind_op.target_set = 0u;
  texture_bind_op.target_binding = 0u;
  texture_bind_op.info.image_sampler.image_subresource.image = state->image;
  ngf_cmd_bind_resources(cmd_buf, &texture_bind_op, 1u);
  draw_textured_quad(state->ubo, 0, state->nearest_sampler, cmd_buf);
  draw_textured_quad(state->ubo, 1, state->bilinear_sampler, cmd_buf);
  draw_textured_quad(state->ubo, 2, state->nearest_sampler, cmd_buf);
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
