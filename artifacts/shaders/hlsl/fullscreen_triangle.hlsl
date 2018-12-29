//T: fullscreen-triangle ps:PSMain vs:VSMain

#include "triangle.hlsl"

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
  return float4(ps_in.position * 0.5 + 0.5, 1.0);
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0, 0.0, 0.0);
}
