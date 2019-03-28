#pragma once

#include <nicegraf.h>
#include <nicegraf_wrappers.h>
#include <imgui.h>

// A nicegraf-based rendering backend for ImGui.
class ngf_imgui {
public:
  // Initializes the internal state of the ImGui rendering backend.
  ngf_imgui();

  // Records commands for rendering the contents of ImGui draw data into the
  // given command buffer.
  void record_rendering_commands(ngf_cmd_buffer *cmdbuf);

private:
  struct uniform_data {
    float ortho_projection[4][4];
  };

#if !defined(NGF_NO_IMGUI)
  ngf::graphics_pipeline pipeline_;
  ngf::streamed_uniform<uniform_data> uniform_data_;
  ngf::image font_texture_;
  ngf::sampler tex_sampler_;
  ngf::attrib_buffer attrib_buffer_;
  ngf::index_buffer index_buffer_;
  ngf::pixel_buffer texture_data_;
  ngf::shader_stage vertex_stage_;
  ngf::shader_stage fragment_stage_;
  ngf::render_target default_rt_;
#endif
};
