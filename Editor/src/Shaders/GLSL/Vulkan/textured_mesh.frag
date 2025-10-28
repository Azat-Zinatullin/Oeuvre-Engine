#version 460
#extension GL_EXT_scalar_block_layout : require

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;

//output write
layout (location = 0) out vec4 outFragColor;

layout(std430, set = 0, binding = 1) uniform MaterialData
{   
    vec3 color;
    int sponza;
    vec2 metalRough;
    int notTextured;   
    int useNormalMap;
} materialData;

//texture to access
layout(set = 1, binding = 0) uniform sampler2D albedoMap;
layout(set = 1, binding = 1) uniform sampler2D normalMap;
layout(set = 1, binding = 2) uniform sampler2D roughnessMap;
layout(set = 1, binding = 3) uniform sampler2D metallicMap;

layout(std430, set = 2, binding = 0) uniform LightData
{   
	vec4 lightPositionsAndIntensities[16];
    vec4 lightColorsAndRanges[16];    
    vec3 camPos;
    int lightCount;  
    mat4 view;
    vec3 lightDir;          
    float sunlightPower;
    float skyboxIntensity;
    float farPlane;
    vec2 padding;
} lightData;

const float PI = 3.14159265359;

vec3 getNormalFromMap()
{
    vec3 tangentNormal = texture(normalMap, inUV).xyz * 2.0 - 1.0;

    vec3 Q1  = dFdx(inWorldPos);
    vec3 Q2  = dFdy(inWorldPos);
    vec2 st1 = dFdx(inUV);
    vec2 st2 = dFdy(inUV);

    vec3 N   = normalize(inNormal);
    vec3 T  = normalize(Q1*st2.t - Q2*st1.t);
    vec3 B  = -normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

    return normalize(TBN * tangentNormal);
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}   
// ----------------------------------------------------------------------------

vec3 SRGBToLinear(vec3 color)
{
    vec3 x = color / 12.92;
    vec3 y = pow(max((color + 0.055) / 1.055, 0.0), vec3(2.4));

    vec3 clr = color;
    clr.r = color.r <= 0.04045 ? x.r : y.r;
    clr.g = color.g <= 0.04045 ? x.g : y.g;
    clr.b = color.b <= 0.04045 ? x.b : y.b;

    return clr;
}


void main() 
{
    float metal = materialData.metalRough.r;
    float rough = materialData.metalRough.g;

    vec3 albedo = pow(texture(albedoMap, inUV).rgb * SRGBToLinear(materialData.color), vec3(2.2));
    float metallic = texture(metallicMap, inUV).r;
    float roughness = texture(roughnessMap, inUV).r;

    if (materialData.sponza == 1)
    {
        metallic = texture(roughnessMap, inUV).b;
        roughness = texture(roughnessMap, inUV).g;     
    }

    metallic *= metal;
    roughness *= rough;

    // input lighting data
    vec3 N = inNormal;
    if (materialData.useNormalMap == 1)
        N = getNormalFromMap();

    vec3 V = normalize(lightData.camPos - inWorldPos);

    vec3 R = reflect(-V, N); 


    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0 
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)    
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < lightData.lightCount; ++i) 
    {
        // calculate per-light radiance
        vec3 L = normalize(lightData.lightPositionsAndIntensities[i].xyz - inWorldPos);
        vec3 H = normalize(V + L);
        float distance = length(lightData.lightPositionsAndIntensities[i].xyz - inWorldPos);
        //float attenuation = 1.0 / (distance * distance);
        float lightRange = 1.0 / lightData.lightColorsAndRanges[i].w;
        float attenuation = clamp(1.0 - (distance * distance) / (lightRange * lightRange), 0.0, 1.0);
        //attenuation *= mix(attenuation, 1.0, light.Falloff);
        vec3 radiance = SRGBToLinear(lightData.lightColorsAndRanges[i].rgb) * attenuation * lightData.lightPositionsAndIntensities[i].w;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);   
        float G   = GeometrySmith(N, V, L, roughness);    
        vec3 F    = fresnelSchlick(max(dot(H, V), 0.0), F0);        
        
        vec3 numerator    = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001; // + 0.0001 to prevent divide by zero
        vec3 specular = numerator / denominator;
        
         // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals 
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;	                
            
        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);        

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL; // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }  

    vec3 ambient = albedo * 0.04;
    
    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2)); 

    outFragColor = vec4(color , 1.0);
}