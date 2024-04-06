#include "../Utility/MeshCommon.lib"
#include "../Utility/Globals.lib"
#include "LightStructs.lib"
#include "LightCommon.lib"

#ifndef MACRO_MANAGED
  #define APPLY_UPSCALE 1
  #define APPLY_BEER 1
  #define APPLY_IMPROVE_BSM 1
  #define APPLY_BSM 1
  #define SHADOW_EXPONENT 100.
  #define SHADOW_BIAS .05
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
Texture2D<float4> beerMapTex : register(t4);

// Beer-Lambert law
float blTransmission(float opticalDepth)
{
  #if APPLY_BEER
  return exp(-opticalDepth);
  #else
  return 1. / (1. + opticalDepth);
  #endif
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
  
  #if APPLY_UPSCALE
  float4 lightMap = mul(float4(lightSpace.xy, .0, 1.), lightCamera.inverseViewProjectionMatrix);
  #else
  float4 lightMap = mul(float4(lightSpace.xy, .0, 1.), lightCamera.inverseViewProjectionMatrix);
  #endif
  //lightMap /= lightMap.w;
  
  // Convert from NDC to texture coords
  lightSpace.xy *= float2(.5, .5); // Scale from [-1, 1] -> [-.5, .5]
  lightSpace.xy += float2(.5, .5); //Translate from [-.5, .5] -> [0, 1]
  lightSpace.y = 1.0f - lightSpace.y; //Flip y axis
  
  // Sample and apply exponential shadow map
  float shadowSample = depthMapTex.SampleLevel(shadowSampler, lightSpace.xy, .0).r;
  float shadowValue = saturate(exp(SHADOW_EXPONENT * (shadowSample - lightSpace.z + SHADOW_BIAS)));
  shadowValue = max(outsideBox3D(float3(lightSpace.xyz), float3(0, 0, 0), float3(1, 1, 1)), shadowValue);
  
  
  float3 lightPlanePosition = -float3(lightCamera.viewMatrix._m30, lightCamera.viewMatrix._m31, lightCamera.viewMatrix._m32);
  #if APPLY_BSM
  float mapDepth = dot(world.xyz - lightPlanePosition, light.direction.xyz);
  
  float3 BSMspace = lightSpace.xyz;
  #if APPLY_UPSCALE
  BSMspace.xy *= .5;
  #endif
  
  // min-Z, Z-range, integrated density, integrated angstrom
  float4 beerSample = beerMapTex.SampleLevel(shadowSampler, BSMspace.xy, .0);
  
  float opticalValue = (mapDepth - beerSample.r) / beerSample.g;
  opticalValue = saturate(opticalValue);
  #if APPLY_IMPROVE_BSM
  opticalValue = smoothstep(.0, 1., opticalValue);
  #endif
  float opticalDepth = opticalValue * beerSample.b;
  
  shadowValue *= max(outsideBox3D(lightSpace.xyz, float3(0.01, 0.01, 0), float3(.98, .98, 1)), blTransmission(opticalDepth));
  #endif
  
  // Compute the shadow + half lambert irradiance
  float irradiance = shadowValue * saturate(halfLambert(dot(normal, -light.direction.xyz)));
  
  // Return the lit colour
  litColourOut[threadID.xy] = float4(diffuseColour * light.diffuse * irradiance, baseAlpha);
}