#include "../Utility/Globals.lib"
#include "RaymarchCommon.lib"
#include "../Utility/MeshCommon.lib"

#ifndef MACRO_MANAGED
  #define APPLY_UPSCALE 1
#endif

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer MarchSlot : register(b1)
{
  MarchVolumeDispatchInfo dispatchInfo;
  BasicOptics opticalInfo;
}

RWTexture2D<float4> litColourOut : register(u0);

[numthreads(8, 8, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float2 uv = float2(threadID.xy) * float2(dispatchInfo.outputHorizontalStep, dispatchInfo.outputVerticalStep);
  #if APPLY_UPSCALE
    uv *= .5;
  #endif
  
  // (more complex behaviour may be performed here, such as spectral blending)
  
  // Simply sample the texture
  float4 appliedOverlay = texture0.SampleLevel(sampler0, uv, .0);
  
  litColourOut[threadID.xy] = float4(litColourOut[threadID.xy].rgb * appliedOverlay.a + appliedOverlay.rgb, 1.);
}