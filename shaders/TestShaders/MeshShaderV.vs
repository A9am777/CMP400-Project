#include "../Utility/MeshCommon.lib"

cbuffer CameraSlot : register(b0)
{
  CameraBuffer camera;
};

PixelInputType main(VertexInputType input)
{
  PixelInputType output;

	// Convert to world position
  output.position = mul(input.position, camera.worldMatrix);
  // Bunnyhop the world position excluding projection
  output.worldPosition = output.position.xyz;
  // Convert to screen coordinates
  output.position = mul(output.position, camera.viewMatrix);
  output.position = mul(output.position, camera.projectionMatrix);
  output.screenPosition = output.position / output.position.w; // Make sure to pass screen coordinates
    
	// Bunnyhop the texel
  output.uv = input.uv;

	// Calculate the normal vector within the world
  output.normal = mul(input.normal, (float3x3)camera.worldMatrix);
  output.normal = normalize(output.normal);
    
  return output;
}