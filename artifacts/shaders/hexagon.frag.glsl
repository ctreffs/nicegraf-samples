#version 430

layout(location = 0) in vec4 o_Color;

out vec4 o_FragColor;

void main() {
  o_FragColor = o_Color;
}

