#include "../Procedural/fBM.lib"

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

RWTexture3D<VolumeElement> textureOut : register(u0);

[numthreads(8, 8, 8)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  // Fetch the location within the texture relative to the centre
  float3 normalisedLocation = float3(threadID.xyz) / float3(info.size);
  normalisedLocation = normalisedLocation - float3(.5, .5, .5);
  float sqrRadius = .5 * .5;
  
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
  
  // Compute the falloff over the square radius (plus an fBM 'interesting' alteration for exotic outputs)
  float sphereDensity = saturate(sqrRadius - dot(normalisedLocation, normalisedLocation) - info.wackyScale * pow(abs(fbm), info.wackyPower)) / sqrRadius;
  
  VolumeElement element;
  element.density = sphereDensity * abs(fbm); // Apply spherical falloff to fBM
  element.maxDensity = element.density;
  element.angstromExponent = pow(1. - normalisedLocation.y - .5, 0.8);
  
  textureOut[threadID.xyz] = element;
}