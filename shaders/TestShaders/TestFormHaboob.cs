#include "../Procedural/fBM.lib"

RWTexture3D<float> textureOut : register(u0);
#pragma pack_matrix( row_major )
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

cbuffer VolumeParamsSlot : register(b0)
{
  VolumeParams info;
}

[numthreads(8, 8, 8)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float3 normalisedLocation = float3(threadID.xyz) / float3(info.size);
  normalisedLocation = normalisedLocation - float3(.5, .5, .5);
  
  TEA rng;
  rng.delta = 0x9e3779b9;
  rng.seed(info.seed);
  rng.associate(uint2(0, 0));
  
  // Compute FBM noise
  float fbm;
  {
    WorldSpace world;
    world.worldMax = float3(info.worldSize, info.worldSize, info.worldSize);
    world.fractionBits = 13;
    world.epsilon = 0.000001;
    
    fBM noise;
    noise.octaves = info.octaves;
    noise.fracGap = info.fractionalGap;
    noise.fracIncr = info.fractionalIncrement;
    
    fbm = noise.fBMNoise(world, rng, normalisedLocation.xyz, float3(info.fbmOffset, info.fbmOffset, info.fbmOffset), float3(info.fbmScale, info.fbmScale, info.fbmScale));
  }
  
  float sqrRadius = .5 * .5;
  float sphereDensity = saturate(sqrRadius - dot(normalisedLocation, normalisedLocation) - info.wackyScale * pow(abs(fbm), info.wackyPower)) / sqrRadius;
  
  textureOut[threadID.xyz] = sphereDensity * abs(fbm);
}