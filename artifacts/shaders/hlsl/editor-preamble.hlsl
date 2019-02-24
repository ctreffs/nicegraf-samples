#include "shaders/hlsl/triangle.hlsl"

cbuffer ShaderEdUniformBuffer : register(b0) {
  float  u_Time;
  float  u_TimeDelta;
  float2 u_ScreenSize;
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0, 0.0, 0.0);
}