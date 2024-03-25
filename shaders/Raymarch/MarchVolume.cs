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

RWTexture2D<float4> screenOut : register(u0);
Texture3D<float4> volumeTexture : register(t0);
SamplerState volumeSampler : register(s0);

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

float3 solveQuadratic(float a, float b, float c)
{
  float determinant = b * b - 4 * a * c;
  float rtDeterminant = sqrt(determinant);
  return float3(determinant, float2(-b - rtDeterminant, -b + rtDeterminant) / (2 * a));
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

// Angstrom's coefficient
float4x4 spectralScatter(float4x4 relativeWavelengths, float angstromExponent)
{
  return pow(relativeWavelengths, -angstromExponent);
}

// Beer-Lambert law
float blTransmission(float opticalDepth)
{
  #if APPLY_BEER
  return exp(-opticalDepth);
  #else
  return 1. / (1. + opticalDepth);
  #endif
}
float4x4 blTransmission(float4x4 opticalDepths)
{
  #if APPLY_BEER
  return exp(-opticalDepths);
  #else
  return ONE_MAT / (ONE_MAT + opticalDepths);
  #endif
}

// Beer-Powder approximation of transmission
float bpTransmission(float opticalDepth)
{
  return blTransmission(opticalDepth) - blTransmission(opticalInfo.powderCoefficient * opticalDepth);
}
float4x4 bpTransmission(float4x4 opticalDepths)
{
  return blTransmission(opticalDepths) - blTransmission(opticalInfo.powderCoefficient * opticalDepths);
}

float4 hgScatter(float angularDistance, in float4 anisotropicTerms)
{
  #if APPLY_HG
  static float intCoeff = 1. / (4. * PI);
  
  float4 numerator = 1. - anisotropicTerms * anisotropicTerms;
  
  float4 denominator = anisotropicTerms - FILL_VEC(2.) * angularDistance;
  denominator = anisotropicTerms * denominator;
  denominator += ONE_VEC;
  denominator = pow(denominator, float4(1.5, 1.5, 1.5, 1.5));

  return numerator / denominator;
  #else
  return ONE_VEC;
  #endif
}

#define Phase(angularDistance, anisotropicTerms) hgScatter(angularDistance, anisotropicTerms)
#define Transmission(opticalDepths) bpTransmission(opticalDepths)
#define Integrator SimpsonsIntegrator

[numthreads(16, 16, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
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
  float2 normScreen = float2(rayParams.z, normToSigned(rayParams.w - 1.));
  
  // Form a ray from the screen
  Ray ray;
  float rayMaxDepth = .0;
  
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
  
  float4x4 wavelengths = opticalInfo.spectralWavelengths / 0.743f;
  
  // Directional lighting will have a constant phase along the ray
  float angularDistance = dot(ray.dir.xyz, -light.direction.xyz);
  float4 incomingForwardIrradiance = Phase(angularDistance, opticalInfo.anisotropicForwardTerms) * float4(light.diffuse, light.diffuse.r);
  float4 incomingBackwardIrradiance = Phase(angularDistance, opticalInfo.anisotropicBackwardTerms) * float4(light.diffuse, light.diffuse.r);
  float4 ambientIrradiance = opticalInfo.ambientFraction * float4(light.ambient, light.ambient.r) * Phase(1., opticalInfo.anisotropicForwardTerms);
  
  Integrator absorptionInte = { 0, 0, 0, 0, 0 }; // Keep distinct from transmission
  Integrator angstromInte = { 0, 0, 0, 0, 0 };
  // Spectral CIE X1YZX2
  Integrator irradianceInteX = { 0, 0, 0, 0, 0 };
  Integrator irradianceInteY = { 0, 0, 0, 0, 0 };
  Integrator irradianceInteZ = { 0, 0, 0, 0, 0 };
  Integrator irradianceInteX2 = { 0, 0, 0, 0, 0 };
  
  [loop] // Yeah need to consider this impact
  for (uint i = 0; i < params.iterations; ++i)
  {
    // Accumulate absorption from density
    float3 haboobSample = haboobCubeDensitySample(ray);
    float densitySample = haboobSample.x;
    float densityMaxSample = haboobSample.y;
    float angstromSample = haboobSample.z;
    
    append(absorptionInte, densitySample);
    append(angstromInte, angstromSample);
    
    float referenceOpticalDepth = opticalInfo.attenuationFactor * integrate(absorptionInte, ray.travelDistance);
    float referenceAngstromAbsorption = opticalInfo.absorptionAngstromExponent * integrate(angstromInte, ray.travelDistance);
    
    // TODO: this is the non-constant second path and needs to be replaced!
    // Lets assume it is the current density along the ray for now
    float referenceScatterOpticalDepth = opticalInfo.attenuationFactor * 4.;
    
    // Accumulate intensity as a function of optical thickness
    float4 irradianceSample;
    
    #if APPLY_SPECTRAL
      float4x4 baseRadiance = mul(IDENTITY_MAT, (float1x4)(ambientIrradiance + lerp(incomingForwardIrradiance, incomingBackwardIrradiance, opticalInfo.phaseBlendWeightTerms)));
      float4x4 transmissions = Transmission(referenceOpticalDepth * spectralScatter(wavelengths, referenceAngstromAbsorption)) * Transmission(referenceScatterOpticalDepth  * spectralScatter(wavelengths, opticalInfo.scatterAngstromExponent));
      float4x4 integratorRadiance = mul(transmissions, baseRadiance) * opticalInfo.spectralWeights;
      
      irradianceSample = mul(ONE_VEC, integratorRadiance);
    #else
      irradianceSample = Transmission(referenceOpticalDepth) * Transmission(referenceScatterOpticalDepth) * (ambientIrradiance + lerp(incomingForwardIrradiance, incomingBackwardIrradiance, opticalInfo.phaseBlendWeightTerms));
    #endif
      
    append(irradianceInteX, irradianceSample.x);
    append(irradianceInteY, irradianceSample.y);
    append(irradianceInteZ, irradianceSample.z);
    append(irradianceInteX2, irradianceSample.w);
    
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

  // Background transmission
  float finalTransmission = max(blTransmission(opticalInfo.attenuationFactor * integrate(absorptionInte, ray.travelDistance)), .1);
  
  // Set as irradiance from the volume with alpha = background transmission
  screenOut[threadID.xy] = float4(integrate(irradianceInteX, ray.travelDistance) + integrate(irradianceInteX2, ray.travelDistance), integrate(irradianceInteY, ray.travelDistance), integrate(irradianceInteZ, ray.travelDistance), finalTransmission);
}