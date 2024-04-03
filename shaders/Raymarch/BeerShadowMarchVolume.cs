#include "RaymarchCommon.lib"
#include "../Lighting/LightStructs.lib"

#ifndef MACRO_MANAGED
#define APPLY_BEER 1
#define APPLY_HG 1
#define APPLY_SPECTRAL 1
#define APPLY_CONE_TRACE 1
#define APPLY_UPSCALE 1
#define MARCH_MANUAL 0
#define MARCH_STEP_COUNT 24
#define SHOW_DENSITY 0
#define SHOW_ANGSTROM 0
#define SHOW_SAMPLE_LEVEL 0
#define SHOW_MASK 0
#define SHOW_RAY_TRAVEL 0
#endif

RWTexture2D<float4> rays : register(u0);
RWTexture2D<float4> bsmOut : register(u1);
Texture3D<float4> volumeTexture : register(t0);
SamplerState volumeSampler : register(s0);

// Of the light source perspective
cbuffer CameraSlot : register(b0)
{
  CameraBuffer camera;
}

cbuffer MarchSlot : register(b1)
{
  MarchVolumeDispatchInfo dispatchInfo;
  BasicOptics opticalInfo;
}

cbuffer LightSlot : register(b2)
{
  DirectionalLight light;
}

// Converts [0, 1] to [-1, 1]
float normToSigned(float norm)
{
  return (norm - .5) * 2.;
}

void fromHomogeneous(inout float4 vec)
{
  vec = vec / vec.w;
}

void march(inout Ray ray, float stepSize)
{
  ray.pos = ray.pos + ray.dir * stepSize;
  ray.travelDistance += stepSize;
}

// Returns the sample level for cone tracing
float getConeSampleLevel(in Ray ray, float worldToTexels)
{
  float rayRadius = dispatchInfo.pixelRadius + dispatchInfo.pixelRadiusDelta * ray.travelDistance;
#if APPLY_UPSCALE
  rayRadius *= 2.;
#endif
  return log2(rayRadius * worldToTexels);
}

float3 haboobCubeDensitySample(in Ray ray)
{
  // Compute the local coordinates within the cube
  float4 localPos = mul(float4(ray.pos.xyz, 1.), dispatchInfo.localVolumeTransform);
  localPos = localPos / localPos.w;
  localPos.xyz += float3(.5, .5, .5);
  
#if APPLY_CONE_TRACE
  return volumeTexture.SampleLevel(volumeSampler, localPos.xyz, getConeSampleLevel(ray, dispatchInfo.texelDensity / dispatchInfo.volumeSize.x)).rgb;
#else
    return volumeTexture.SampleLevel(volumeSampler, localPos.xyz, .0).rgb;
#endif
}

#define Integrator SimpsonsIntegrator

[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  #if APPLY_UPSCALE
  // Using 1/4 rays over 1/4 of the screen
  int2 screenPosition = threadID.xy * 2;
  
  // Fetch parameters that have been fed in (over the 2x2 pixel area)
  float4 rayParams = rays[screenPosition];
  rayParams += rays[screenPosition + int2(1, 0)];
  rayParams += rays[screenPosition + int2(0, 1)];
  rayParams += rays[screenPosition + int2(1, 1)];
  rayParams *= .25;
  #else
  // Using all rays over the screen
  int2 screenPosition = threadID.xy;
  
  // Fetch parameters that have been fed in
  float4 rayParams = rays[screenPosition];
  #endif
  
  // Mask fragments
  if (rayParams.x < 0 || rayParams.y < 0)
  {
    #if SHOW_MASK
    bsmOut[threadID.xy] = float4(0., 0., 100., 1.);
    #else
    bsmOut[threadID.xy] = float4(.0, 1., .0, .0);
    #endif
    return;
  }
  
  // Utilise the rasterization pass to fetch screen coordinates
  float2 normScreen = float2(rayParams.z, rayParams.w);
  
  // Form a ray from the screen
  Ray ray;
  float rayZMapDepth = .0;
  float rayMaxDepth = .0;
  
  ray.pos = float4(normScreen, rayParams.x, 1.);
  {
    float4 directionPointTarget = float4(normScreen, rayParams.y, 1.);
    
    // Convert both points to world space
    ray.pos = mul(ray.pos, camera.inverseViewProjectionMatrix);
    fromHomogeneous(ray.pos);
    directionPointTarget = mul(directionPointTarget, camera.inverseViewProjectionMatrix);
    fromHomogeneous(directionPointTarget);
    
    float4 testZPlane = float4(normScreen, 0, 1.);
    testZPlane = mul(testZPlane, camera.inverseViewProjectionMatrix);
    fromHomogeneous(testZPlane);
    
    // Create a normalised direction vector
    ray.dir = directionPointTarget - ray.pos;
    rayMaxDepth = length(ray.dir);
    ray.dir /= rayMaxDepth;
    
    rayZMapDepth = length(ray.pos - testZPlane);
  }
  ray.colour = float4(1., 1., 1., 0.);
  ray.travelDistance = .0;
  
  // Set march params
  MarchParams params;
  params.iterations = dispatchInfo.iterations * 2; // *Must* be a multiple of 2
  params.marchZStep = dispatchInfo.marchZStep;
  params.initialStep = dispatchInfo.initialZStep;
  params.mask = 0;
  #if !MARCH_MANUAL
    params.initialStep = .0;
    params.marchZStep = rayMaxDepth / float(params.iterations);
  #endif
  
  // Jump ray forward
  march(ray, params.initialStep);
  
  Integrator absorptionInte = { 0, 0, 0, 0, 0 };
  Integrator angstromInte = { 0, 0, 0, 0, 0 };
  
  // Must not unroll due to iterative sampling
  [loop]
  for (uint i = 0; i < params.iterations; ++i)
  {
    // Accumulate absorption from density
    float3 haboobSample = haboobCubeDensitySample(ray);
    float densitySample = haboobSample.x;
    float densityMaxSample = haboobSample.y;
    float angstromSample = haboobSample.z;
    
    append(absorptionInte, densitySample);
    append(angstromInte, angstromSample);
    
    // March again!
    march(ray, params.marchZStep);
  }
  
  // min-Z, Z-range, integrated density, integrated angstrom
  bsmOut[threadID.xy] = float4(rayZMapDepth, rayMaxDepth, 10 * integrate(absorptionInte, ray.travelDistance), 10 * integrate(angstromInte, ray.travelDistance));
}