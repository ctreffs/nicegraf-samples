// T: cubemap vs:VSMain ps:PSMain

#include "triangle.hlsl"

[vk::binding(0, 0)] cbuffer UniformData {
  float4x4 u_Transform;
  float    u_Aspect;
};
[vk::binding(1, 0)] uniform TextureCube tex;
[vk::binding(2, 0)] uniform sampler samp;

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
  float3 dir = (u_Transform * float4(-ps_in.position.x, ps_in.position.y / (2.0*u_Aspect), -1.0, 0.0)).xyz;
  return tex.Sample(samp, dir);
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0, 0.0, 0.0);
}
