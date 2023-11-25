#include "RaymarchCommon.lib"
#include "../Lighting/LightStructs.lib"

RWTexture2D<float4> screenOut : register(u0);
Texture3D<float4> volumeTexture : register(t0);
SamplerState volumeSampler : register(s0);

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

struct Cube
{
  float4 pos;
  float3 size;
};

struct Integrator // Simpsons rule
{
  float firstTerm;
  float lastTerm;
  float odds;
  float evens;
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

void append(inout Integrator inte, float value, uint index)
{
  inte.lastTerm = value;
  if (index % 2 == 0)
  {
    inte.evens += value;
  }
  else
  {
    inte.odds += value;
  }
}

float integrate(inout Integrator inte, uint sampleCount)
{
  // Note: sample count must always be even
  float stepSize = 1. / float(sampleCount);
  float result = inte.firstTerm + inte.lastTerm;
  result += 4. * inte.odds;
  result += 2 * (inte.evens - inte.lastTerm);
  return result * stepSize / 3.;
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

float cubeDensitySample(in Ray ray, in Cube cube)
{
  // Compute the local coordinates within the cube (TODO: use matrix)
  float3 localPos = ray.pos.xyz - cube.pos.xyz;
  localPos.xyz = localPos.xyz / cube.size;
  
  return volumeTexture.SampleLevel(volumeSampler, localPos, .5);
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
  float2 normScreen = float2(normToSigned((float(threadID.x) + .5f) * dispatchInfo.outputHorizontalStep),
                              -normToSigned((float(threadID.y) + .5f) * dispatchInfo.outputVerticalStep));
  
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
  
  // Test bounding sphere
  Sphere sphere;
  sphere.pos = float4(0., 0., 0., 1.);
  
  // Test sampling cube
  Cube cube;
  cube.size = float3(3., 3., 3.);
  cube.pos = float4(sphere.pos.xyz - cube.size * float3(.5,.5,.5), 1.); // Shift from centre origin to AABB
  
  sphere.sqrRadius = dot(cube.size * float3(.5, .5, .5), cube.size * float3(.5, .5, .5)); // Encapsulate cube
  
  // Set march params
  MarchParams params;
  params.iterations = dispatchInfo.iterations;
  params.marchZStep = dispatchInfo.marchZStep;
  params.initialStep = dispatchInfo.initialZStep;
  if (!dispatchInfo.flagManualMarch)
  {
    // Use the bounding sphere to approximate better params
    determineSphereParams(params, ray, sphere);
  }
  
  if(params.mask)
  {
    return;
  }
  
  // Jump forward
  march(ray, params.initialStep);
  
  // Directional lighting will have a constant phase along the ray
  float3 incomingIntensity = applyScatter(light.diffuse, ray.dir.xyz, light.direction.xyz);
  
  Integrator absorptionInte;
  absorptionInte.firstTerm = absorptionInte.odds = absorptionInte.evens = absorptionInte.lastTerm = .0;
  Integrator intensityInte;
  intensityInte.firstTerm = intensityInte.odds = intensityInte.evens = intensityInte.lastTerm = .0;

  float absorption = 0.;
  float intensity = 0.;
  [loop] // Yeah need to consider this
  for (uint i = 0; i < params.iterations; ++i)
  {
    float densitySample = opticalInfo.densityCoefficient * cubeDensitySample(ray, cube);
    absorption += densitySample;
    append(absorptionInte, densitySample, i + 1);
    //if (inSphere(ray, sphere))
    //{
    float intensitySample = densitySample * beerLambertAttenuation(integrate(absorptionInte, i + 1) * opticalInfo.attenuationFactor);
    intensity += intensitySample;
    append(intensityInte, intensitySample, i + 1);
    //}
    
    march(ray, params.marchZStep);
  }
  
  screenOut[threadID.xy] *= beerLambertAttenuation(integrate(absorptionInte, params.iterations) * opticalInfo.attenuationFactor);
  screenOut[threadID.xy] += float4(integrate(intensityInte, params.iterations) * incomingIntensity, 0.);
}