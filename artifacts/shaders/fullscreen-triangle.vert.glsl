#version 430

precision mediump float;

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

layout(location=0) out vec4 color;
layout(location=1) out vec2 uv;

void main() {
  const vec4 pos[] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4( 3.0, -1.0, 0.0, 1.0),
    vec4(-1.0,  3.0, 0.0, 1.0));
  const vec2 texcoords[] = vec2[](
    vec2(0.0), vec2(2.0, 0.0), vec2(0.0, 2.0)
  );
  int i = gl_VertexID % 3;
  gl_Position = pos[i];
  uv = texcoords[i];
  color = 0.5 * (gl_Position + 1.0);
}