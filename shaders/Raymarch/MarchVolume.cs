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

struct MarchParams
{
  float initialStep;
  float marchZStep;
  uint iterations;
  bool mask;
};

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

float beerLambertAttenuation(float opticalDepth)
{
  return opticalInfo.flagApplyBeer ? exp(-opticalDepth) : 1.;
}

// Henyey-Greenstein scattering function
float hgScatter(float angularDistance, float gCoeff)
{
  static float intCoeff = 1. / (4. * PI);
  float numerator = 1. - gCoeff * gCoeff;
  float denominator = 1. + gCoeff * gCoeff - 2. * gCoeff * angularDistance;
  
  return opticalInfo.flagApplyHG ? numerator * intCoeff / pow(denominator, 1.5) : 1.;
}

float hgScatter(in float3 viewDir, in float3 lightDir, float gCoeff)
{
  float angular = dot(viewDir, -lightDir); // cos angle
  return hgScatter(angular, gCoeff);
}

float3 applyScatter(in float3 colour, float angularDistance)
{
  return colour * float3(
    hgScatter(angularDistance, opticalInfo.colourHGScatter.r),
    hgScatter(angularDistance, opticalInfo.colourHGScatter.g),
    hgScatter(angularDistance, opticalInfo.colourHGScatter.b));
}

float3 applyScatter(in float3 colour, in float3 viewDir, in float3 lightDir)
{
  float angular = dot(viewDir, -lightDir); // cos angle
  return applyScatter(colour, angular);
}

float4 alphaBlend(float4 foreground, float4 background)
{
  return foreground * foreground.a + (1. - foreground.a) * background;
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
  
  // Test sphere
  Sphere sphere;
  sphere.pos = float4(0., 0., 1., 1.);
  sphere.sqrRadius = 1.;
  
  // Compute optimal march params
  MarchParams params;
  params.iterations = dispatchInfo.iterations;
  determineSphereParams(params, ray, sphere);
  
  if(params.mask)
  {
    return;
  }
  
  // Jump forward
  march(ray, params.initialStep);
  
  // Directional lighting will have a constant phase along the ray
  float3 incomingIntensity = applyScatter(light.diffuse, ray.dir.xyz, light.direction.xyz);
  
  float absorption = 0.;
  float intensity = 0.;
  [loop] // Yeah need to consider this
  for (uint i = 0; i < params.iterations; ++i)
  {
    float density = opticalInfo.densityCoefficient * sphereDensitySample(ray, sphere);
    absorption += density * (1. - absorption);
    if (inSphere(ray, sphere))
    {
      intensity += density * beerLambertAttenuation(absorption * opticalInfo.attenuationFactor) * (1. - absorption);
    }
    
    march(ray, params.marchZStep);
  }
  
  screenOut[threadID.xy] *= beerLambertAttenuation(absorption * opticalInfo.attenuationFactor);
  screenOut[threadID.xy] += float4(intensity * incomingIntensity, 0.);
}