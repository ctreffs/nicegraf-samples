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
#pragma once

#include <vector>
#include <nicegraf.h>
#include <nicegraf_wrappers.h>

ngf::shader_stage load_shader_stage(const char *root_name,
                                    const char *entry_point_name,
                                    ngf_stage_type type,
                                    const char *prefix = "shaders/generated/");
ngf_plmd* load_pipeline_metadata(const char *name,
                             const char *prefix = "shaders/generated/");

ngf::context create_default_context(uintptr_t handle, uint32_t w, uint32_t h);

std::vector<char> load_raw_data(const char *file_path);

struct init_result {
  ngf::context context;
  void *userdata;
};
