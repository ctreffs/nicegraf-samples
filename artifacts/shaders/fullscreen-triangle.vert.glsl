#version 430

out gl_PerVertex
{
  vec4 gl_Position;
  float gl_PointSize;
  float gl_ClipDistance[];
};

out vec4 color;

void main() {
  const vec4 pos[] = vec4[](
    vec4(-1.0, -1.0, 0.0, 1.0),
    vec4( 3.0, -1.0, 0.0, 1.0),
    vec4(-1.0,  3.0, 0.0, 1.0));
  gl_Position = pos[gl_VertexID % 3];
  color = 0.5 * (gl_Position + 1.0);
}