//T:depth vs:VSMain ps:PSMain

#include "triangle.hlsl"

cbuffer TriangleData : register(b0) {
  float u_Scale;
  float u_OffsetX;
  float u_OffsetY;
  float u_Depth;
  float4  u_Color;
};

struct PSInput {
  Triangle_PSInput tri;
  nointerpolation float4 color : COLOR0;
};

float4 PSMain(PSInput ps_in) : SV_TARGET {
  return ps_in.color;
}

PSInput VSMain(uint vid : SV_VertexID) {
  PSInput output = {
    Triangle(vid, u_Scale, float2(u_OffsetX, u_OffsetY), u_Depth),
    u_Color
  };
  return output;
}
