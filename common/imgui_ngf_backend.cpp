#include "imgui_ngf_backend.h"
#include "common.h"
#include <nicegraf_util.h>
#include <assert.h>
#include <vector>

ngf_imgui::ngf_imgui() {
  vertex_stage_ = load_shader_stage("imgui", "VSMain", NGF_STAGE_VERTEX);
  fragment_stage_ = load_shader_stage("imgui", "PSMain", NGF_STAGE_FRAGMENT);

  // Obtain default rendertarget.
  ngf_render_target *rt;
  ngf_error err =
      ngf_default_render_target(NGF_LOAD_OP_DONTCARE, NGF_LOAD_OP_DONTCARE,
                                NULL, NULL, &rt);
  assert(err == NGF_ERROR_OK);
  default_rt_ = ngf::render_target(rt);
  
  // Initial pipeline configuration with OpenGL-style defaults.
  ngf_util_graphics_pipeline_data pipeline_data;
  ngf_util_create_default_graphics_pipeline_data(nullptr,
                                                 &pipeline_data);
  
  plmd *pipeline_metadata = load_pipeline_metadata("imgui");

  // Simple pipeline layout with just one descriptor set that has
  // a uniform buffer and a texture.
  err = ngf_util_create_pipeline_layout_from_metadata(
      ngf_plmd_get_layout(pipeline_metadata), &pipeline_data.layout_info);
  assert(err == NGF_ERROR_OK);

  // Set up blend state.
  pipeline_data.blend_info.enable = true;
  pipeline_data.blend_info.sfactor = NGF_BLEND_FACTOR_SRC_ALPHA;
  pipeline_data.blend_info.dfactor = NGF_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

  // Set up depth & stencil state.
  pipeline_data.depth_stencil_info.depth_test = false;
  pipeline_data.depth_stencil_info.stencil_test = false;

  // Make viewport and scissor dynamic.
  pipeline_data.pipeline_info.dynamic_state_mask =
    NGF_DYNAMIC_STATE_SCISSOR | NGF_DYNAMIC_STATE_VIEWPORT;
  
  // Assign programmable stages.
  ngf_graphics_pipeline_info &pipeline_info = pipeline_data.pipeline_info;
  pipeline_info.nshader_stages = 2u;
  pipeline_info.shader_stages[0] = vertex_stage_.get();
  pipeline_info.shader_stages[1] = fragment_stage_.get();
  pipeline_info.compatible_render_target = default_rt_.get();

  // Assign separate-to-combined maps
  pipeline_info.image_to_combined_map =
      ngf_plmd_get_image_to_cis_map(pipeline_metadata);
  pipeline_info.sampler_to_combined_map =
      ngf_plmd_get_sampler_to_cis_map(pipeline_metadata);

  // Configure vertex input.
  ngf_vertex_attrib_desc vertex_attribs[] = {
    {0u, 0u, offsetof(ImDrawVert, pos), NGF_TYPE_FLOAT, 2u, false},
    {1u, 0u, offsetof(ImDrawVert,  uv), NGF_TYPE_FLOAT, 2u, false},
    {2u, 0u, offsetof(ImDrawVert, col), NGF_TYPE_UINT8, 4u, true},
  };
  pipeline_data.vertex_input_info.attribs = vertex_attribs;
  pipeline_data.vertex_input_info.nattribs = 3u;
  ngf_vertex_buf_binding_desc binding_desc = {
    0u, // binding
    sizeof(ImDrawVert), // stride
    NGF_INPUT_RATE_VERTEX // input rate
  };
  pipeline_data.vertex_input_info.nvert_buf_bindings = 1u;
  pipeline_data.vertex_input_info.vert_buf_bindings = &binding_desc;

  err = pipeline_.initialize(pipeline_data.pipeline_info);
  assert(err == NGF_ERROR_OK);

  // Generate data for the font texture.
  ImGuiIO& io = ImGui::GetIO();
  unsigned char* font_pixels;
  int width, height;
  io.Fonts->GetTexDataAsRGBA32(&font_pixels, &width, &height);

  // Create and populate font texture.
  ngf_image_info font_texture_info = {
    NGF_IMAGE_TYPE_IMAGE_2D, // type
    {(uint32_t)width, (uint32_t)height, 1u}, // extent
    1u, // nmips
    NGF_IMAGE_FORMAT_RGBA8, // image_format
    0u, // nsamples
    NGF_IMAGE_USAGE_SAMPLE_FROM // usage_hint
  };
  err = font_texture_.initialize(font_texture_info);
  assert(err == NGF_ERROR_OK);
  err = ngf_populate_image(font_texture_.get(), 0u, {0u, 0u, 0u},
                           {(uint32_t)width, (uint32_t)height, 1u},
                           font_pixels);
  assert(err == NGF_ERROR_OK);
  ImGui::GetIO().Fonts->TexID = (ImTextureID)(uintptr_t)font_texture_.get();

  // Allocate UBO for projection matrix.
  ngf_uniform_buffer_info projmtx_ubo_info = {
    sizeof(float) * 16u
  };
  err = projmtx_ubo_.initialize(projmtx_ubo_info);
  assert(err == NGF_ERROR_OK);

  // Create a sampler for the font texture.
  ngf_sampler_info sampler_info {
    NGF_FILTER_NEAREST,
    NGF_FILTER_NEAREST,
    NGF_FILTER_NEAREST,
    NGF_WRAP_MODE_CLAMP_TO_EDGE,
    NGF_WRAP_MODE_CLAMP_TO_EDGE,
    NGF_WRAP_MODE_CLAMP_TO_EDGE,
    0.0f,
    0.0f,
    0.0f,
    {0.0f, 0.0f, 0.0f, 0.0f},
  };
  tex_sampler_.initialize(sampler_info);

  ngf_plmd_destroy(pipeline_metadata, NULL);
}

void ngf_imgui::record_rendering_commands(ngf_cmd_buffer *cmdbuf) {
  ImGui::Render();
  ImDrawData *data = ImGui::GetDrawData();
  if (data->TotalIdxCount <= 0) return;
  //Compute effective viewport width and height, apply scaling for
  // retina/high-dpi displays.
  ImGuiIO& io = ImGui::GetIO();
  int fb_width = (int)(data->DisplaySize.x * io.DisplayFramebufferScale.x);
  int fb_height = (int)(data->DisplaySize.y * io.DisplayFramebufferScale.y);
  data->ScaleClipRects(io.DisplayFramebufferScale);

  // Avoid rendering when minimized.
  if (fb_width <= 0 || fb_height <= 0) { return; }

  // Bind the ImGui rendering pipeline.
  ngf_cmd_bind_pipeline(cmdbuf, pipeline_);
  
  // Bind resources.
  ngf_image_ref font_ref {
    font_texture_.get(),
    0u,
    0u,
    false
  };
  ngf_resource_bind_op resource_binds[3];
  resource_binds[0].target_binding = 0u;
  resource_binds[0].target_set = 0u;
  resource_binds[0].type = NGF_DESCRIPTOR_UNIFORM_BUFFER;
  resource_binds[0].info.uniform_buffer = ngf_uniform_buffer_bind_info {
    projmtx_ubo_.get(), 0u, 16u * sizeof(float)
  };
  resource_binds[1].target_binding = 1u;
  resource_binds[1].target_set = 0u;
  resource_binds[1].type = NGF_DESCRIPTOR_TEXTURE;
  resource_binds[1].info.image_sampler = ngf_image_sampler_bind_info {
    font_ref,
    NULL,
  };
  resource_binds[2].target_binding = 2u;
  resource_binds[2].target_set = 0u;
  resource_binds[2].type = NGF_DESCRIPTOR_SAMPLER;
  resource_binds[2].info.image_sampler = ngf_image_sampler_bind_info {
    font_ref,
    tex_sampler_.get(),
  };
  ngf_cmd_bind_resources(cmdbuf, resource_binds, 3u);

  // Set viewport.
  ngf_irect2d viewport_rect = {
    0u, 0u,
    (uint32_t)fb_width, (uint32_t)fb_height
  };
  ngf_cmd_viewport(cmdbuf, &viewport_rect);

  // Build projection matrix.
  const ImVec2 &pos = data->DisplayPos;
  const float L = pos.x;
  const float R = pos.x + data->DisplaySize.x;
  const float T = pos.y;
  const float B = pos.y + data->DisplaySize.y;
  const float ortho_projection[4][4] =
  {
      { 2.0f/(R-L),   0.0f,         0.0f,   0.0f },
      { 0.0f,         2.0f/(T-B),   0.0f,   0.0f },
      { 0.0f,         0.0f,        -1.0f,   0.0f },
      { (R+L)/(L-R),  (T+B)/(B-T),  0.0f,   1.0f },
  };
  ngf_error err =
    ngf_write_uniform_buffer(projmtx_ubo_.get(),
                             ortho_projection, sizeof(ortho_projection));
  assert(err == NGF_ERROR_OK);

  // These vectors will store vertex and index data for the draw calls.
  // Later this data will be transferred to GPU buffers.
  std::vector<ImDrawVert> vertex_data((size_t)data->TotalVtxCount,
                                      ImDrawVert());
  std::vector<ImDrawIdx> index_data((size_t)data->TotalIdxCount,
                                    0u);
  struct draw_data {
    ngf_irect2d scissor;
    uint32_t first_elem;
    uint32_t nelem;
  };
  std::vector<draw_data> draw_data;

  uint32_t last_vertex = 0u;
  uint32_t last_index = 0u;

  // Process each ImGui command list and translate it into the nicegraf
  // command buffer.
  for (int i = 0u; i < data->CmdListsCount; ++i) {
    // Append vertex data.
    const ImDrawList *imgui_cmd_list = data->CmdLists[i];
    memcpy(&vertex_data[last_vertex],
           imgui_cmd_list->VtxBuffer.Data,
           sizeof(ImDrawVert) * (size_t)imgui_cmd_list->VtxBuffer.Size);

    // Append index data.
    for (int a = 0u; a < imgui_cmd_list->IdxBuffer.Size; ++a) {
      // ImGui uses separate index buffers, but we'll use just one. We will
      // update the index values accordingly.
      index_data[last_index + (size_t)a] =
          (ImDrawIdx)(last_vertex + imgui_cmd_list->IdxBuffer[a]);
    }
    last_vertex += (uint32_t)imgui_cmd_list->VtxBuffer.Size;
    
    // Process each ImGui command in the draw list.
    uint32_t idx_buffer_sub_offset = 0u;
    for (int j = 0u; j < imgui_cmd_list->CmdBuffer.Size; ++j) {
      const ImDrawCmd &cmd = imgui_cmd_list->CmdBuffer[j];
      if (cmd.UserCallback != nullptr) {
        cmd.UserCallback(imgui_cmd_list, &cmd);
      } else {
        ImVec4 clip_rect = ImVec4(cmd.ClipRect.x - pos.x,
                                  cmd.ClipRect.y - pos.y,
                                  cmd.ClipRect.z - pos.x,
                                  cmd.ClipRect.w - pos.y);
        if (clip_rect.x < fb_width &&
            clip_rect.y < fb_height &&
            clip_rect.z >= 0.0f && clip_rect.w >= 0.0f) {
          const ngf_irect2d scissor_rect {
            (int32_t)clip_rect.x,
            fb_height - (int32_t)clip_rect.w,
            (uint32_t)(clip_rect.z - clip_rect.x),
            (uint32_t)(clip_rect.w - clip_rect.y)
          };
          draw_data.push_back(
            {scissor_rect, last_index + idx_buffer_sub_offset,
             (uint32_t)cmd.ElemCount});
          idx_buffer_sub_offset += (uint32_t)cmd.ElemCount;
        }
      }
    }
    last_index += (uint32_t)imgui_cmd_list->IdxBuffer.Size;
  }

  // Create new vertex and index buffers.
  ngf_attrib_buffer_info attrib_buffer_info{
    sizeof(ImDrawVert) * vertex_data.size(), // data size
    vertex_data.data(),
    NULL,
    NULL,
    NGF_VERTEX_DATA_USAGE_TRANSIENT
  };
  attrib_buffer_.initialize(attrib_buffer_info);

  ngf_index_buffer_info index_buffer_info{
    sizeof(ImDrawIdx) * index_data.size(),
    index_data.data(),
    NULL,
    NULL,
    NGF_VERTEX_DATA_USAGE_TRANSIENT
  };
  index_buffer_.initialize(index_buffer_info);

  // Record commands into the cmd buffer.
  ngf_cmd_bind_index_buffer(cmdbuf, index_buffer_,
                            sizeof(ImDrawIdx) < 4
                                ? NGF_TYPE_UINT16 : NGF_TYPE_UINT32);
  ngf_cmd_bind_attrib_buffer(cmdbuf, attrib_buffer_, 0u, 0u);
  for (const auto &draw : draw_data) {
    ngf_cmd_scissor(cmdbuf, &draw.scissor);
    ngf_cmd_draw(cmdbuf, true, draw.first_elem, draw.nelem, 1u);
  }
}
