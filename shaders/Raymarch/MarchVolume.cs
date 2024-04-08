#include "RaymarchCommon.lib"
#include "../Lighting/LightStructs.lib"

#ifndef MACRO_MANAGED  
  #define MARCH_MANUAL 0
  #define MARCH_STEP_COUNT 24

  #define SHOW_DENSITY 0
  #define SHOW_ANGSTROM 0
  #define SHOW_SAMPLE_LEVEL 0
  #define SHOW_MASK 0
  #define SHOW_RAY_TRAVEL 0
#endif

RWTexture2D<float4> screenOut : register(u0);
Texture3D<float4> volumeTexture : register(t0);
Texture2D<float4> directShadowTexture : register(t1);
Texture2D<float4> beerMapTexture : register(t2);
SamplerState volumeSampler : register(s0);
SamplerState shadowSampler : register(s1);

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

cbuffer LightCameraSlot : register(b3)
{
  CameraBuffer lightCamera;
}

// Compute the optimal march params for this sphere
void determineSphereParams(inout MarchParams params, in Ray ray, in Sphere sphere)
{
  // Compute intersection(s)
  float3 result;
  {
    float3 translation = ray.pos.xyz - sphere.pos.xyz;
    float a = dot(ray.dir.xyz, ray.dir.xyz);
    float b = 2. * dot(translation, ray.dir.xyz);
    float c = dot(translation, translation) - sphere.sqrRadius;
  
    result = solveQuadratic(a, b, c);
  }
  
  params.mask = result.x < .0;
  params.initialStep = max(result.y, .0); // Do NOT step if currently inside volume
  params.marchZStep = (result.z - params.initialStep) / float(params.iterations);
}

bool inSphere(in Ray ray, in Sphere sphere)
{
  float3 displacement = ray.pos.xyz - sphere.pos.xyz;
  return dot(displacement, displacement) <= sphere.sqrRadius; // dot(x, x) = |x|^2
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

float sphereDensitySample(in Ray ray, in Sphere sphere)
{
  float3 displacement = ray.pos.xyz - sphere.pos.xyz;
  return max(sphere.sqrRadius - dot(displacement, displacement), 0.) / sphere.sqrRadius; // dot(x, x) = |x|^2
}

float cubeDensitySample(in Ray ray, in Cube cube)
{
  // Compute the local coordinates within the cube (TODO: use matrix)
  float3 localPos = ray.pos.xyz - cube.pos.xyz;
  localPos.xyz = localPos.xyz / cube.size;
  
  return volumeTexture.SampleLevel(volumeSampler, localPos, .5).r;
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

float4 alphaBlend(float4 foreground, float4 background)
{
  return foreground * foreground.a + (1. - foreground.a) * background;
}

#define Phase(angularDistance, anisotropicTerms) hgScatter(angularDistance, anisotropicTerms)
#define Transmission(opticalDepths) bpTransmission(opticalDepths, opticalInfo.powderCoefficient)
#define Integrator4 SimpsonsIntegrator4
#define Integrator SimpsonsIntegrator

[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  // Fetch the ray parameter per fragment
  #if APPLY_UPSCALE
  // Using 1/4 rays over 1/4 of the screen
  int2 screenPosition = threadID.xy * 2;
  
  // Fetch parameters that have been fed in (over the 2x2 pixel area)
  float4 rayParams = screenOut[screenPosition];
  rayParams += screenOut[screenPosition + int2(1, 0)];
  rayParams += screenOut[screenPosition + int2(0, 1)];
  rayParams += screenOut[screenPosition + int2(1, 1)];
  rayParams *= .25;
  #else
  // Using all rays over the screen
  int2 screenPosition = threadID.xy;
  
  // Fetch parameters that have been fed in
  float4 rayParams = screenOut[screenPosition];
  #endif
  
  // Mask fragments
  if(rayParams.x < 0 || rayParams.y < 0) 
  { 
    #if SHOW_MASK
    screenOut[threadID.xy] = float4(100., 100., 100., 1.);
    #else
    screenOut[threadID.xy] = float4(.0, .0, .0, 1.);
    #endif
    return; 
  }
  
  // Utilise the rasterization pass to fetch screen coordinates
  float2 normScreen = float2(rayParams.z, rayParams.w);
  
  // Form a ray from the screen
  Ray ray;
  float rayMaxDepth = .0;
  
  // Shadow terms of the ray at each extent (u, v, linear-Z)
  float4 shadowSpace;
  float4 deltaShadowSpace;
  
  ray.pos = float4(normScreen, rayParams.x, 1.);
  {
    float4 directionPointTarget = float4(normScreen, rayParams.y, 1.);
    
    // Convert both points to world space
    ray.pos = mul(ray.pos, camera.inverseViewProjectionMatrix);
    fromHomogeneous(ray.pos);
    directionPointTarget = mul(directionPointTarget, camera.inverseViewProjectionMatrix);
    fromHomogeneous(directionPointTarget);
    
    // Create a normalised direction vector
    ray.dir = directionPointTarget - ray.pos;
    rayMaxDepth = length(ray.dir);
    ray.dir /= rayMaxDepth;
    
    // Catch the shadow terms
    shadowSpace = getShadowDependents(ray.pos, lightCamera, light);
    deltaShadowSpace = getShadowDependents(directionPointTarget, lightCamera, light);
    deltaShadowSpace -= shadowSpace;
    deltaShadowSpace /= rayMaxDepth;
  }
  ray.colour = float4(1., 1., 1., 0.);
  ray.travelDistance = .0;
  
  // Set march params
  MarchParams params;
  params.iterations = dispatchInfo.iterations;
  params.mask = true;
  #if MARCH_MANUAL
    params.marchZStep = dispatchInfo.marchZStep;
    params.initialStep = dispatchInfo.initialZStep;
  #else
    params.initialStep = .0;
    params.marchZStep = rayMaxDepth / float(params.iterations);
  #endif
  
  // Jump ray forward
  march(ray, params.initialStep);
  
  float4x4 wavelengths = opticalInfo.spectralWavelengths / 0.743f;
  
  // Directional lighting will have a constant phase along the ray
  float angularDistance = dot(ray.dir.xyz, -light.direction.xyz);
  float4 incomingForwardIrradiance = Phase(angularDistance, opticalInfo.anisotropicForwardTerms) * float4(light.diffuse, light.diffuse.r);
  float4 incomingBackwardIrradiance = Phase(angularDistance, opticalInfo.anisotropicBackwardTerms) * float4(light.diffuse, light.diffuse.r);
  float4 ambientIrradiance = opticalInfo.ambientFraction * float4(light.ambient, light.ambient.r) * Phase(1., opticalInfo.anisotropicForwardTerms);
  
  incomingForwardIrradiance *= .25;
  incomingBackwardIrradiance *= .25;
  
  // Integrated optical depth and angstrom
  Integrator absorptionInte = { 0, 0, 0, 0, 0 };
  Integrator angstromInte = { 0, 0, 0, 0, 0 };
  
  // Integrated spectral CIE X1_Y_Z_X2
  Integrator4 irradianceInte;
  irradianceInte.count = 0;
  irradianceInte.termBuckets[0] = irradianceInte.termBuckets[1] = irradianceInte.firstTerms = irradianceInte.lastTerms = ZERO_VEC;
  
  // Must not unroll due to iterative sampling
  [loop]
  for (uint i = 0; i < params.iterations; ++i)
  {
    // Accumulate absorption from density
    {
      float3 haboobSample = haboobCubeDensitySample(ray);
      float densitySample = haboobSample.x;
      float densityMaxSample = haboobSample.y;
      float angstromSample = haboobSample.z;
    
      append(absorptionInte, densitySample);
      append(angstromInte, angstromSample);
    }
    
    // Linearly interpolate geometric shadow information
    float4 shadowTerms = shadowSpace + deltaShadowSpace * ray.travelDistance;
    
    // Integrate the direct optical depth + angstrom along the ray
    float referenceOpticalDepth = opticalInfo.attenuationFactor * integrate(absorptionInte, ray.travelDistance);
    float referenceAngstrom = opticalInfo.absorptionAngstromExponent * integrate(angstromInte, ray.travelDistance);
    
    // Determine the integrated optical depth + angstrom value along the incoming light ray according to the BSM
    #if APPLY_BSM
      float2 bsmValues = getBSMOpticalCoefficients(shadowTerms, beerMapTexture, shadowSampler);
    
      float referenceScatterOpticalDepth = bsmValues.x;
      float referenceScatterAngstrom = bsmValues.y;
    #else
      float referenceScatterOpticalDepth = opticalInfo.attenuationFactor * EULER;
      float referenceScatterAngstrom = opticalInfo.scatterAngstromExponent * EULER;
    #endif
    
    // Determine the direct exponential shadow map value
    float shadowValue = getExponentialShadowCoefficient(shadowTerms, directShadowTexture, shadowSampler);
    
    // Accumulate intensity as a function of optical thickness
    {
      float4 directIrradiance = ambientIrradiance + shadowValue * lerp(incomingForwardIrradiance, incomingBackwardIrradiance, opticalInfo.phaseBlendWeightTerms);
      #if APPLY_SPECTRAL
        // Compute falloff per wavelength
        float4x4 transmissions = Transmission(referenceOpticalDepth * spectralScatter(wavelengths, referenceAngstrom)) * Transmission(referenceScatterOpticalDepth  * spectralScatter(wavelengths, referenceScatterAngstrom));
      
        // Determine weighted transmission of irradiance across wavelengths
        float4x4 integratorRadiance = mul(diagonal(directIrradiance), transmissions) * opticalInfo.spectralWeights;
      
        // Sum up all wavelength contributions
        float4 irradianceSample = mul(integratorRadiance, ONE_VEC);
      #else
        float4 irradianceSample = Transmission(referenceOpticalDepth) * Transmission(referenceScatterOpticalDepth) * directIrradiance;
      #endif
      
      append4(irradianceInte, irradianceSample);
    }
    
    // March again!
    march(ray, params.marchZStep);
  }
  
  // Debug/testing outputs
  #if SHOW_DENSITY
    screenOut[threadID.xy] = float4(opticalInfo.attenuationFactor * integrate(absorptionInte, ray.travelDistance), .0, .0, 1.);
    return;
  #elif SHOW_ANGSTROM
    screenOut[threadID.xy] = float4(opticalInfo.absorptionAngstromExponent * integrate(angstromInte, ray.travelDistance), .0, .0, 1.);
    return;
  #elif SHOW_SAMPLE_LEVEL
    float4 sampleLevelInfo = float4(.0, getConeSampleLevel(ray, dispatchInfo.texelDensity / dispatchInfo.volumeSize.x), .0, 1.);
    ray.travelDistance = params.initialStep;
    sampleLevelInfo.x = getConeSampleLevel(ray, dispatchInfo.texelDensity / dispatchInfo.volumeSize.x);
    screenOut[threadID.xy] = sampleLevelInfo / 8.;
    return;
  #elif SHOW_RAY_TRAVEL
    screenOut[threadID.xy] = float4(ray.travelDistance, 1., 1., 1.);
    return;
  #endif
  
  // Background transmission, assume uniform behaviour across wavelengths (use Beer-Lambert over powder)
  float finalTransmission = blTransmission(opticalInfo.attenuationFactor * integrate(absorptionInte, ray.travelDistance));
  
  // Determine final colour
  float3 finalIrradiance = spectralToRGB(integrate4(irradianceInte, ray.travelDistance), opticalInfo.spectralToRGB);
  
  // Set as irradiance from the volume with alpha = background transmission
  screenOut[threadID.xy] = float4(finalIrradiance, finalTransmission);
}