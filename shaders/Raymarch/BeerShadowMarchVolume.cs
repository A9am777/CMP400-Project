#include "../Lighting/LightStructs.lib"
#include "MarchVolumeMacros.lib"

RWTexture2D<float4> rays : register(u0);
RWTexture2D<float4> bsmOut : register(u1);
Texture3D<float3> volumeTexture : register(t0);
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

[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  int2 screenPosition;
  float4 rayParams;
  fetchPixelRayInfo(screenPosition, rayParams, threadID.xy, rays);
  
  // Mask fragments
  if (isRayMasked(rayParams) > 0)
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
    
    float4 testZPlane = float4(normScreen, .0, 1.);
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
    float3 haboobSample = haboobCubeDensitySample(ray, dispatchInfo, volumeSampler, volumeTexture);
    float densitySample = haboobSample.x;
    float densityMaxSample = haboobSample.y;
    float angstromSample = haboobSample.z;
    
    append(absorptionInte, densitySample);
    append(angstromInte, angstromSample);
    
    // March again!
    march(ray, params.marchZStep);
  }
  
  // min-Z, Z-range, integrated density, integrated angstrom
  bsmOut[threadID.xy] = float4(rayZMapDepth, rayMaxDepth, opticalInfo.attenuationFactor * integrate(absorptionInte, ray.travelDistance), opticalInfo.scatterAngstromExponent * integrate(angstromInte, ray.travelDistance));
}