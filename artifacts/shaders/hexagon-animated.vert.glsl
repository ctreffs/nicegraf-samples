#version 430

layout(location = 0) in vec2 i_Position;
layout(location = 1) in vec3 i_Color;

layout(std140, binding = 0) uniform AspectRatioBuf {
  float u_AspectRatio;
};

layout(std140, binding = 1) uniform TimeBuf {
  float u_Time;
};

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout(location = 0) out vec4 o_Color;

void main() {
  float theta = u_Time / 2.0;
  mat2 rot_mtx = mat2(cos(theta), -sin(theta), sin(theta), cos(theta));
  gl_Position = vec4(rot_mtx * i_Position * vec2(1.0, u_AspectRatio), 0.0, 1.0);
  o_Color = vec4(i_Color, 1.0);
}