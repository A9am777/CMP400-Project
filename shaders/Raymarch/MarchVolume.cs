#include "RaymarchCommon.lib"

RWTexture2D<float4> screenOut : register(u0);

cbuffer CameraSlot : register(b0)
{
  CameraBuffer camera;
}

cbuffer MarchInfo : register(b1)
{
  MarchVolumeDispatchInfo dispatchInfo;
}

struct Ray
{
  float4 pos;
  float4 dir;
  float4 colour;
};

struct Sphere
{
  float4 pos;
  float sqrRadius;
};

// The depth to find the ray 'end' at
#define ZDirectionTest .01

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

bool inSphere(in Ray ray, in Sphere sphere)
{
  float3 displacement = ray.pos.xyz - sphere.pos.xyz;
  return dot(displacement, displacement) <= sphere.sqrRadius; // dot(x, x) = |x|^2
}

[numthreads(1, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 threadID : SV_DispatchThreadID)
{
  // Actually a matrix is probably better here, maybe even rasterisation pass?
  float2 normScreen = float2(normToSigned(float(threadID.x) * dispatchInfo.outputHorizontalStep),
                              -normToSigned(float(threadID.y) * dispatchInfo.outputVerticalStep));
  
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
  
  // Jump forward
  march(ray, dispatchInfo.initialZStep);
  
  // Test sphere
  Sphere sphere;
  sphere.pos = float4(0., 0., 1., 1.);
  sphere.sqrRadius = 1.;
  
  [loop] // Yeah need to consider this
  for (uint i = 0; i < dispatchInfo.iterations; ++i)
  {
    ray.colour.a = ray.colour.a + (inSphere(ray, sphere) ? 1. / float(dispatchInfo.iterations) : 0.);
    
    march(ray, dispatchInfo.marchZStep);
  }
  
  screenOut[threadID.xy] =  screenOut[threadID.xy] + ray.colour * ray.colour.a;
  //screenOut[threadID.xy] = float4(normScreen, 1., 1.);

}