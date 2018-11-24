#version 430

#ifndef SPIRV_CROSS_CONSTANT_ID_0
#define SPIRV_CROSS_CONSTANT_ID_0 1.0
#endif

#ifndef SPIRV_CROSS_CONSTANT_ID_1
#define SPIRV_CROSS_CONSTANT_ID_1 1.0
#endif

out vec4 o_FragColor;

void main() {
  o_FragColor = vec4(SPIRV_CROSS_CONSTANT_ID_0, SPIRV_CROSS_CONSTANT_ID_1, 1.0, 1.0);
}
