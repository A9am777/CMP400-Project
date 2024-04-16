#include "../Procedural/fBM.lib"

struct HaboobRadial
{
  float roofGradient;
  float exponentialRate;
  float exponentialScale;
  float rOffset;
  float noseHeight;
  float blendHeight;
  float blendRate;
  float padding;
};

struct HaboobDistribution
{
  float falloffScale;
  float heightScale;
  float heightExponent;
  float angleRange;
  float anglePower;
  float3 padding;
};

struct VolumeParams
{
  int3 size;
  uint padding;
  
  // Proc gen params
  uint4 seed;
  
  float worldSize;
  float octaves;
  float fractionalGap;
  float fractionalIncrement;
  
  float fbmOffset;
  float fbmScale;
  float wackyPower;
  float wackyScale;
  
  HaboobRadial radial;
  HaboobDistribution distribution;
};

struct VolumeElement
{
  float density; // R11
  float maxDensity; // G11
  float angstromExponent; // B10
};

cbuffer VolumeParamsSlot : register(b0)
{
  VolumeParams info;
}

// Returns the radial distance for the logarithmic component of the haboob
float haboobUndersideRadius(float height)
{
  return info.radial.rOffset + info.radial.exponentialScale * log(1. + height * info.radial.exponentialRate);
}

// Returns the radial distance for the flat roof component of the haboob
float haboobRoofRadius(float height)
{
  float nose = info.radial.noseHeight;
  return info.radial.roofGradient * (height - nose) + haboobUndersideRadius(nose);
}

// Returns the radial distance representing the leading edge of the haboob
float haboobRadius(float height)
{
  float undersideValue = haboobUndersideRadius(height);
  float flatRoofValue = haboobRoofRadius(height);
  
  // Exponential easing function
  float easing = max((height - info.radial.noseHeight) / info.radial.blendHeight, .0);
  easing = exp(-info.radial.blendRate * easing);
  
  return lerp(flatRoofValue, undersideValue, easing);
}

float haboobVerticalFlux(float height)
{
  return pow(info.distribution.heightScale * (1. + height), -info.distribution.heightExponent);
}

float haboobAngularFlux(float angle)
{
  float halfRange = info.distribution.angleRange * .5;
  float linearAngle = abs(halfRange - angle) / halfRange;
  return 1. - smoothstep(.0, 1., pow(linearAngle, info.distribution.anglePower));
}

float simpleBoltzmannFalloff(float x, float peakX, float falloff)
{
  static float constAxisOffset = sqrt(2.) * .5;
  float xSubstitute = peakX - falloff * constAxisOffset;
  float innerValue = (x - xSubstitute) / falloff;
  
  return saturate(innerValue * exp(-innerValue * innerValue));
}

RWTexture3D<VolumeElement> textureOut : register(u0);

[numthreads(8, 8, 8)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  // Fetch the location within the texture relative to the centre
  float3 normalisedLocation = float3(threadID.xyz) / float3(info.size);
  normalisedLocation = normalisedLocation - float3(.5, .5, .5);
  float3 cylinderCoord = float3(threadID.xyz) / float3(info.size);
  cylinderCoord.xz = 2. * cylinderCoord.xz - float2(1., 1.);
  
  // Start the parallel random number sequencer
  TEA rng;
  rng.delta = 0x9e3779b9; // Salt
  rng.seed(info.seed);
  rng.associate(uint2(0, 0));
  
  // Compute FBM noise
  float fbm;
  {
    // Create the procedural environment - prioritising high precision for fixed point numbers
    WorldSpace world;
    world.worldMax = float3(info.worldSize, info.worldSize, info.worldSize);
    world.fractionBits = 13;
    world.epsilon = 0.000001;
    
    // Dispatch a pass of fBM noise
    fBM noise;
    noise.octaves = info.octaves;
    noise.fracGap = info.fractionalGap;
    noise.fracIncr = info.fractionalIncrement;
    fbm = noise.fBMNoise(world, rng, normalisedLocation.xyz, float3(info.fbmOffset, info.fbmOffset, info.fbmOffset), float3(info.fbmScale, info.fbmScale, info.fbmScale));
  }
  
  // Determine where the leading edge lies
  float leadingRadius = haboobRadius(cylinderCoord.y);
  float radius = length(float2(cylinderCoord.x, cylinderCoord.z)) + info.wackyScale * fbm; // (plus an fBM 'interesting' alteration for exotic outputs)
  float arcAngle = atan2(cylinderCoord.z, -cylinderCoord.x) + 1.57079632679;
  float arcDensity = haboobAngularFlux(arcAngle);
  float radialDensity = simpleBoltzmannFalloff(radius, leadingRadius, info.distribution.falloffScale);
  float heightDensity = haboobVerticalFlux(cylinderCoord.y);
  
  float density = arcDensity * radialDensity * heightDensity;
  
  VolumeElement element;
  element.density = density;
  element.maxDensity = element.density;
  element.angstromExponent = pow(.5 - normalisedLocation.y, info.wackyPower) + info.wackyScale * fbm;
  
  textureOut[threadID.xyz] = element;
}