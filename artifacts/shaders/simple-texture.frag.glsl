#version 430

precision mediump float;

layout(location = 1) in vec2 uv;

layout(binding = 0) uniform sampler2D tex;

out vec4 o_FragColor;

void main() {
  o_FragColor = texture(tex, uv);
}