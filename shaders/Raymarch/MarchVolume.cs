#include "RaymarchCommon.lib"
#include "../Lighting/LightStructs.lib"

RWTexture2D<float4> screenOut : register(u0);

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

static float PI = radians(180.);

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

float sphereDensitySample(in Ray ray, in Sphere sphere)
{
  float3 displacement = ray.pos.xyz - sphere.pos.xyz;
  return max(sphere.sqrRadius - dot(displacement, displacement), 1.) / sphere.sqrRadius; // dot(x, x) = |x|^2
}

float beerLambertAttenuation(float opticalDepth)
{
  return exp(-opticalDepth);
}

// Henyey-Greenstein scattering function
float hgScatter(in float3 viewDir, in float3 lightDir, float gCoeff)
{
  float phase = dot(viewDir, lightDir); // cos angle
  static float intCoeff = 1. / (4. * PI);
  float numerator = 1. - gCoeff * gCoeff;
  float denominator = 1. + gCoeff * gCoeff - 2. * gCoeff * phase;
  
  return numerator * intCoeff / pow(denominator, 1.5);
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
  
  //
  float3 incomingIntensity = light.diffuse;
  if (opticalInfo.flagApplyHG)
  {
    // Directional lighting will have a constant phase along the ray
    incomingIntensity = incomingIntensity * float3(
    hgScatter(ray.dir.xyz, light.direction.xyz, opticalInfo.colourHGScatter.r),
    hgScatter(ray.dir.xyz, light.direction.xyz, opticalInfo.colourHGScatter.g),
    hgScatter(ray.dir.xyz, light.direction.xyz, opticalInfo.colourHGScatter.b));
  }
  
  float opticalDepthAccum = 0.;
  float intensity = 0.;
  [loop] // Yeah need to consider this
  for (uint i = 0; i < dispatchInfo.iterations; ++i)
  {
    opticalDepthAccum += sphereDensitySample(ray, sphere);
    if (inSphere(ray, sphere))
    {
      intensity += opticalInfo.flagApplyBeer ? beerLambertAttenuation(opticalDepthAccum * opticalInfo.attenuationFactor) : 1. / opticalDepthAccum;
    }
    
    march(ray, dispatchInfo.marchZStep);
  }
  
  screenOut[threadID.xy] = screenOut[threadID.xy] + float4(intensity * incomingIntensity, 0.);
}