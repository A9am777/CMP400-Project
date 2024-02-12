#include "../Lighting/LightStructs.lib"

RWTexture2D<float4> screenOut : register(u0);

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float4 white = float4(1., 1., 1., 1.);
  float4 blue = float4(.0, .0, 1., 1.);
  screenOut[threadID.xy] = (threadID.x * threadID.y) % 5 == 0 ? white : blue;
}