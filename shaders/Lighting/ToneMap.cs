cbuffer ToneValues : register(b0)
{
  float gamma;
  float exposure;
  float2 padding;
}

RWTexture2D<float4> colourOut : register(u0);

[numthreads(8, 8, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  float4 baseColour = colourOut[threadID.xy];
  
  // Map the colour for exposure
  float3 applied = float3(1.0f, 1.0f, 1.0f) - exp(baseColour.rgb * -exposure);
  // Map by gamma, bringing the value into LDR range
  applied = pow(applied, float3(1.0f, 1.0f, 1.0f) / gamma);
  
  // Return the modified colour
  colourOut[threadID.xy] = float4(applied, baseColour.a);
}