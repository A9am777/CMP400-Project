#include "../Raymarch/RaymarchCommon.lib"
#include "../Utility/MeshCommon.lib"
#include "../Utility/Globals.lib"
#include "LightStructs.lib"
#include "LightCommon.lib"

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

[numthreads(8, 8, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float3 diffuseColour = diffuseTex[threadID.xy].rgb;
  float baseAlpha = diffuseTex[threadID.xy].a;
  float3 normal = normalTex[threadID.xy].rgb;
  float4 world = float4(worldTex[threadID.xy].xyz, 1.);
  
  float shadowValue = 1.;
  #if APPLY_SHADOW
    // Fetch the shadow geometric terms
    float4 shadowSpace = getShadowDependents(world, lightCamera, light);
  
    // The exponential shadow map value
    shadowValue *= getExponentialShadowCoefficient(shadowSpace, depthMapTex, shadowSampler);
  
    // The BSM value
    #if APPLY_BSM
      float2 bsmValues = getBSMOpticalCoefficients(shadowSpace, beerMapTex, shadowSampler);
      shadowValue *= blTransmission(bsmValues.x);
    #endif
  #endif
  
  // Compute the shadow + half lambert irradiance
  float irradiance = shadowValue * saturate(halfLambert(dot(normal, -light.direction.xyz)));
  
  // Return the lit colour
  litColourOut[threadID.xy] = float4(diffuseColour * light.diffuse * irradiance, baseAlpha);
}