#include "RaymarchCommon.lib"
#include "../Lighting/LightStructs.lib"

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
  
  return volumeTexture.SampleLevel(volumeSampler, localPos, .5);
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
  return opticalInfo.flagApplyBeer ? exp(-opticalDepth) : 1.;
}
float blTransmission(float4x4 opticalDepths)
{
  return opticalInfo.flagApplyBeer ? exp(-opticalDepths) : 1.;
}

// Beer-Powder approximation of transmission
float bpTransmission(float opticalDepth)
{
  return blTransmission(opticalDepth) - blTransmission(opticalInfo.powderCoefficient * opticalDepth);
}
float bpTransmission(float4x4 opticalDepths)
{
  return blTransmission(opticalDepths) - blTransmission(opticalInfo.powderCoefficient * opticalDepths);
}

float4 hgScatter(float angularDistance, in float4 anisotropicTerms)
{
  static float intCoeff = 1. / (4. * PI);
  
  float4 numerator = 1. - anisotropicTerms * anisotropicTerms;
  
  float4 denominator = anisotropicTerms - float4(2., 2., 2., 2.) * angularDistance;
  denominator = anisotropicTerms * denominator;
  denominator += float4(1., 1., 1., 1.);
  denominator = pow(denominator, float4(1.5, 1.5, 1.5, 1.5));

  return opticalInfo.flagApplyHG ? numerator / denominator : float4(1., 1., 1., 1.);
}

#define Phase(angularDistance, anisotropicTerms) hgScatter(angularDistance, anisotropicTerms)
#define Transmission(opticalDepths) bpTransmission(opticalDepths)
#define Integrator SimpsonsIntegrator

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  // Actually a matrix is probably better here, maybe even rasterisation pass?
  float2 normScreen = float2(normToSigned((float(threadID.x) + .5f) * dispatchInfo.outputHorizontalStep),
                              -normToSigned((float(threadID.y) + .5f) * dispatchInfo.outputVerticalStep));
  
  // Form a ray from the screen
  Ray ray;
  ray.pos = float4(normScreen, 0, 1.);
  {
    float4 directionPointTarget = float4(normScreen, ZDirectionTest, 1.);
    
    // Convert both points to world space
    ray.pos = mul(ray.pos, camera.inverseViewProjectionMatrix);
    fromHomogeneous(ray.pos);
    directionPointTarget = mul(directionPointTarget, camera.inverseViewProjectionMatrix);
    fromHomogeneous(directionPointTarget);
    
    // Create a normalised direction vector
    ray.dir = normalize(directionPointTarget - ray.pos);
  }
  ray.colour = float4(1., 1., 1., 0.);
  
  // Bounding sphere
  Sphere sphere;
  sphere.pos = float4(0., 0., 0., 1.);
  
  // Sampling cube
  Cube cube;
  cube.size = float3(3., 3., 3.);
  cube.pos = float4(sphere.pos.xyz - cube.size * float3(.5,.5,.5), 1.); // Shift from centre origin to AABB
  
  sphere.sqrRadius = dot(cube.size * float3(.5, .5, .5), cube.size * float3(.5, .5, .5)); // Encapsulate cube
  
  // Set march params
  MarchParams params;
  params.iterations = dispatchInfo.iterations * 2; // *Must* be a multiple of 2
  params.marchZStep = dispatchInfo.marchZStep;
  params.initialStep = dispatchInfo.initialZStep;
  params.mask = 0;
  if (!dispatchInfo.flagManualMarch)
  {
    // Use the bounding sphere to approximate better params
    determineSphereParams(params, ray, sphere);
  }
  
  // Exit now if possible
  if(params.mask)
  {
    return;
  }
  
  // Jump ray forward
  march(ray, params.initialStep);
  
  float4x4 identity = float4x4(
  1., .0, .0, .0,
  .0, 1., .0, .0,
  .0, .0, 1., .0,
  .0, .0, .0, 1.
  );
  
  
  float4x4 wavelengths = opticalInfo.spectralWavelengths / 0.743f;
  
  // Directional lighting will have a constant phase along the ray
  float angularDistance = dot(ray.dir.xyz, -light.direction.xyz);
  float4 incomingForwardIrradiance = Phase(angularDistance, opticalInfo.anisotropicForwardTerms) * float4(light.diffuse, light.diffuse.r);
  float4 incomingBackwardIrradiance = Phase(angularDistance, opticalInfo.anisotropicBackwardTerms) * float4(light.diffuse, light.diffuse.r);
  float4 ambientIrradiance = float4(light.ambient, light.ambient.r);
  
  Integrator absorptionInte = { 0, 0, 0, 0, 0 }; // Keep distinct from transmission
  // Spectral CIE X1YZX2
  Integrator irradianceInteX = { 0, 0, 0, 0, 0 };
  Integrator irradianceInteY = { 0, 0, 0, 0, 0 };
  Integrator irradianceInteZ = { 0, 0, 0, 0, 0 };
  Integrator irradianceInteX2 = { 0, 0, 0, 0, 0 };
  
  [loop] // Yeah need to consider this impact
  for (uint i = 0; i < params.iterations; ++i)
  {
    // Accumulate absorption from density
    float densitySample = cubeDensitySample(ray, cube);
    append(absorptionInte, densitySample);
    
    float referenceOpticalDepth = opticalInfo.attenuationFactor * integrate(absorptionInte);
    
    // TODO: this is the non-constant second path and needs to be replaced!
    // Lets assume it is the current density along the ray for now
    float referenceScatterOpticalDepth = opticalInfo.attenuationFactor;
    
    // Accumulate intensity as a function of optical thickness
    float4 irradianceSample;
    
    if (opticalInfo.flagApplySpectral)
    {
      float4x4 baseRadiance = mul(identity, (float1x4)(ambientIrradiance + lerp(incomingForwardIrradiance, incomingBackwardIrradiance, opticalInfo.phaseBlendWeightTerms)));
      float4x4 transmissions = Transmission(referenceOpticalDepth * spectralScatter(wavelengths, opticalInfo.absorptionAngstromExponent)) * Transmission(referenceScatterOpticalDepth  * spectralScatter(wavelengths, opticalInfo.scatterAngstromExponent));
      float4x4 integratorRadiance = mul(transmissions, baseRadiance) * opticalInfo.spectralWeights;
      
      irradianceSample = mul(float1x4(1., 1., 1., 1.), integratorRadiance);
    }
    else
    {
      irradianceSample = Transmission(referenceOpticalDepth) * Transmission(referenceScatterOpticalDepth) * (ambientIrradiance + lerp(incomingForwardIrradiance, incomingBackwardIrradiance, opticalInfo.phaseBlendWeightTerms));
    }
      
    append(irradianceInteX, irradianceSample.x);
    append(irradianceInteY, irradianceSample.y);
    append(irradianceInteZ, irradianceSample.z);
    append(irradianceInteX2, irradianceSample.w);
    
    // March again!
    march(ray, params.marchZStep);
  }
  
  // Apply scattering to incoming background irradiance
  screenOut[threadID.xy] *= blTransmission(opticalInfo.attenuationFactor * integrate(absorptionInte)); //TODO: BP is not very good here
  // Add irradiance from the volume itself
  screenOut[threadID.xy] += float4(integrate(irradianceInteX) + integrate(irradianceInteX2), integrate(irradianceInteY), integrate(irradianceInteZ), .0);
}