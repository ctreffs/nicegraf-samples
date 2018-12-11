#version 430

precision mediump float;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout(std140, binding = 0) uniform TriangleUBO {
  float u_Scale;
  float u_OffsetX;
  float u_OffsetY;
  float u_Depth;
  vec4  u_Color;
};

layout(location = 0) flat out vec4 o_Color;

void main() {
 o_Color = u_Color;
 const vec2 pos[] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 0.0,  1.0));
  gl_Position = vec4(pos[gl_VertexID % 3] * u_Scale + vec2(u_OffsetX, u_OffsetY), u_Depth, 1.0);
}