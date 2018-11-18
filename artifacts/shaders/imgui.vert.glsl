#version 430

layout(std140, binding = 0) uniform ProjMtx {
  mat4 u_Projection;
};

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout (location = 0) in vec2 Position;
layout (location = 1) in vec2 UV;
layout (location = 2) in vec4 Color;

out vec2 Frag_UV;
out vec4 Frag_Color;

void main() {
  Frag_UV = UV;
  Frag_Color = Color;
  gl_Position = u_Projection * vec4(Position.xy,0,1);
}