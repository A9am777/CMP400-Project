#include "Globals.lib"
#include "MeshCommon.lib"

cbuffer CameraSlot : register(b0)
{
  CameraBuffer camera;
};

struct OutputType
{
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
};

OutputType main(VertexInputType input)
{
  OutputType output;

  // Standard projection
  output.position = mul(input.position, camera.worldMatrix);
  output.position = mul(output.position, camera.viewMatrix);
  output.position = mul(output.position, camera.projectionMatrix);

	// Mirror the texel coord
  output.uv = input.uv;

  return output;
}