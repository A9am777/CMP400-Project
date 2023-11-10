#include "LightStructs.lib"
#include "LightCommon.lib"

cbuffer LightSlot : register(b0)
{
  DirectionalLight light;
}

RWTexture2D<float4> litColourOut : register(u0);
Texture2D<float4> diffuseTex : register(t0);
Texture2D<float4> normalTex : register(t1);

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float3 diffuseColour = diffuseTex[threadID.xy].rgb;
  float baseAlpha = diffuseTex[threadID.xy].a;
  float3 normal = normalTex[threadID.xy].rgb;
  
  // Return the lit colour
  litColourOut[threadID.xy] = float4(diffuseColour * light.diffuse * saturate(halfLambert(dot(normal, -light.direction.xyz))), baseAlpha);
}