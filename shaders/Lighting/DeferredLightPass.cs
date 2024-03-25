#include "../Utility/MeshCommon.lib"
#include "LightStructs.lib"
#include "LightCommon.lib"

#ifndef MACRO_MANAGED
  #define SHADOW_EXPONENT 100.
#endif

cbuffer LightSlot : register(b0)
{
  DirectionalLight light;
}

cbuffer LightCamSlot : register(b1)
{
  CameraBuffer lightCamera;
}

SamplerState shadowSampler : register(s0);

RWTexture2D<float4> litColourOut : register(u0);
Texture2D<float4> diffuseTex : register(t0);
Texture2D<float4> normalTex : register(t1);
Texture2D<float4> worldTex : register(t2);
Texture2D<float4> depthMapTex : register(t3);

// Test
float insideBox3D(float3 v, float3 bottomLeft, float3 topRight)
{
  float3 s = step(bottomLeft, v) - step(topRight, v);
  return s.x * s.y * s.z;
}

[numthreads(8, 8, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float3 diffuseColour = diffuseTex[threadID.xy].rgb;
  float baseAlpha = diffuseTex[threadID.xy].a;
  float3 normal = normalTex[threadID.xy].rgb;
  float4 world = float4(worldTex[threadID.xy].xyz, 1.);
  
  // Fetch the position within the light's view
  // it is orthographic so z can be used
  float4 lightSpace = mul(world, lightCamera.viewMatrix);
  lightSpace = mul(lightSpace, lightCamera.projectionMatrix);
  lightSpace = lightSpace / lightSpace.w;
  
  // Convert from NDC to texture coords
  lightSpace.xy += float2(1.0f, 1.0f); //Translate from [-1, 1] -> [0, 2]
  lightSpace.xy *= float2(0.5f, 0.5f); //Scale from [0, 2] -> [0, 1]
  lightSpace.y = 1.0f - lightSpace.y; //Flip y axis
  
  // Sample and apply exponential shadow map
  float shadowSample = depthMapTex.SampleLevel(shadowSampler, lightSpace.xy, .5).r;
  float shadowValue = saturate(exp(SHADOW_EXPONENT * (shadowSample - lightSpace.z + 0.01f)));
  shadowValue = max(1. - insideBox3D(float3(lightSpace.xyz), float3(0, 0, 0), float3(1, 1, 1)), shadowValue);
  
  // Compute the shadow + half lambert irradiance
  float irradiance = shadowValue * saturate(halfLambert(dot(normal, -light.direction.xyz)));
  
  // Return the lit colour
  litColourOut[threadID.xy] = float4(diffuseColour * light.diffuse * irradiance, baseAlpha);
}