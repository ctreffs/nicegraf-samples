#pragma once

#include <nicegraf.h>
#include <nicegraf_wrappers.h>
#include <imgui.h>

// A nicegraf-based rendering backend for ImGui.
class ngf_imgui {
public:
  // Initializes the internal state of the ImGui rendering backend.
  ngf_imgui(const ngf_shader_stage *vertex_stage,
            const ngf_shader_stage *fragment_stage);

  // Records commands for rendering the contents of ImGui draw data into the
  // given command buffer.
  void record_rendering_commands(ImDrawData *data, ngf_cmd_buffer *cmdbuf);

private:
  ngf::graphics_pipeline pipeline_;
  ngf::buffer projmtx_ubo_;
  ngf::image font_texture_;
  ngf::descriptor_set desc_set_;
  ngf::sampler tex_sampler_;
  ngf::buffer vertex_buffer_;
  ngf::buffer index_buffer_;
};