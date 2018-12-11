#version 430

precision mediump float;

layout(location = 0) flat in vec4 i_Color;

out vec4 o_FragColor;

void main() {
  o_FragColor = i_Color;
}