#version 430

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec3 i_Color;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout(location = 0) out vec4 o_Color;

void main() {
  gl_Position = vec4(i_Position, 0.0, 1.0);
  o_Color = vec4(i_Color, 1.0);
}