#include <imgui.h>
#include <nicegraf.h>
#include <nicegraf_util.h>
#include <nicegraf_wrappers.h>

class ngf_imgui {
public:
  ngf_imgui(const ngf_shader_stage *vertex_stage,
            const ngf_shader_stage *fragment_stage) {
    // Initial pipeline configuration with OpenGL-style defaults.
    ngf_util_graphics_pipeline_data pipeline_data;
    ngf_util_create_default_graphics_pipeline_data(nullptr,
                                                   &pipeline_data);

    // Simple pipeline layout with just one descriptor set that has
    // a uniform buffer and a texture.
    ngf_descriptor_info descs[] = {
      { NGF_DESCRIPTOR_UNIFORM_BUFFER, 0u },
      { NGF_DESCRIPTOR_TEXTURE_AND_SAMPLER, 1u },
    };
    ngf_error err =
        ngf_util_create_simple_layout(descs, 2u, &pipeline_data.layout_info);
    assert(err == NGF_ERROR_OK);

    // Set up blend state.
    pipeline_data.blend_info.enable = true;
    pipeline_data.blend_info.sfactor = NGF_BLEND_FACTOR_SRC_ALPHA;
    pipeline_data.blend_info.dfactor = NGF_BLEND_FACTOR_DST_ALPHA;

    // Set up depth & stencil state.
    pipeline_data.depth_stencil_info.depth_test = false;
    pipeline_data.depth_stencil_info.stencil_test = false;

    // Make viewport and scissor dynamic.
    pipeline_data.pipeline_info.dynamic_state_mask =
      NGF_DYNAMIC_STATE_SCISSOR | NGF_DYNAMIC_STATE_VIEWPORT;
    
    // Assign programmable stages.
    ngf_graphics_pipeline_info &pipeline_info = pipeline_data.pipeline_info;
    pipeline_info.nshader_stages = 2u;
    pipeline_info.shader_stages[0] = vertex_stage;
    pipeline_info.shader_stages[1] = fragment_stage;

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
      {width, height, 1u}, // extent
      1u, // nmips
      NGF_IMAGE_FORMAT_RGBA8, // image_format
      NGF_IMAGE_USAGE_SAMPLE_FROM // usage_hint
    };
    err = font_texture_.initialize(font_texture_info);
    assert(err == NGF_ERROR_OK);
    err = ngf_populate_image(font_texture_.get(), 0u, {0u, 0u, 0u}, {width, height, 1u},
                             font_pixels);
    assert(err == NGF_ERROR_OK);

    // Allocate UBO for projection matrix.
    ngf_buffer_info projmtx_ubo_info = {
      sizeof(float) * 16u,
      NGF_BUFFER_TYPE_UNIFORM,
      NGF_BUFFER_USAGE_STATIC,
      NGF_BUFFER_ACCESS_DRAW
    };
    err = projmtx_ubo_.initialize(projmtx_ubo_info);
    assert(err == NGF_ERROR_OK);
  }

  void record_rendering_commands(ImDrawData *data, ngf_cmd_buffer *cmdbuf) {
    int fb_width = (int)data->DisplaySize.x * io.DisplayFramebufferScale.x);
    int fb_height = (int)(data->DisplaySize.y * io.DisplayFramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) { return; }
    ngf_irect2d viewport_rect = {0u, 0u, fb_width, fb_height};
    data->ScaleClipRects(io.DisplayFramebufferScale);
    ngf_cmd_bind_pipeline(cmdbuf, pipeline_);
    ngf_cmd_viewport(cmdbuf, &viewport_rect);
    ngf_cmd_bind_vertex_buffer(cmdbuf, vertex_buffer, 0u, 0u);
    
    
  }
private:
  ngf::graphics_pipeline pipeline_;
  ngf::buffer projmtx_ubo_;
  ngf::image font_texture_;
};