RWTexture3D<float> textureOut : register(u0);
#pragma pack_matrix( row_major )
struct VolumeParams
{
  int3 size;
  uint padding;
};

cbuffer VolumeParamsSlot : register(b0)
{
  VolumeParams info;
}

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float3 normalisedLocation = float3(threadID.xyz) / float3(info.size);
  normalisedLocation = normalisedLocation - float3(.5, .5, .5);
  
  float sqrRadius = .5 * .5;
  
  float sphereDensity = saturate(sqrRadius - dot(normalisedLocation, normalisedLocation)) / sqrRadius;
  
  textureOut[threadID.xyz] = sphereDensity;
}