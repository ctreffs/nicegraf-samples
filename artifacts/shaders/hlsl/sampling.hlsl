//T: textured-quad ps:PSMain vs:VSMain

struct PixelShaderInput {
  float4 clip_pos : SV_POSITION;
  float2 uv : ATTR0;
};

[vk::binding(0, 1)] cbuffer UniformData {
  float2 u_Scale;
  float2 u_Offset;
};

PixelShaderInput VSMain(uint vid : SV_VertexID) {
  const float2 vertices[] = {
    float2(1.0, 1.0), float2(-1.0, 1.0), float2(1.0, -1.0),
    float2(1.0, -1.0), float2(-1.0, 1.0), float2(-1.0, -1.0)
  };
  float2 position = vertices[vid % 6];
  PixelShaderInput result = {
    float4(position * u_Scale + u_Offset, 0.0, 1.0),
    position * 0.5 + 0.5
  };
  return result;
}

[vk::binding(0, 0)] uniform Texture2D tex;
[vk::binding(1, 1)] uniform sampler   smp;

float4 PSMain(PixelShaderInput input) : SV_TARGET {
  return tex.Sample(smp, input.uv);
}
