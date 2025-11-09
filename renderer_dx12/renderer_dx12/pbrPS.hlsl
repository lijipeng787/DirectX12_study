Texture2D diffuseTexture : register(t0);
Texture2D normalMap : register(t1);
Texture2D rmTexture : register(t2);
SamplerState SampleType : register(s0);

// Use b2 to avoid confusion with VS b0 (MatrixBuffer)
// Even though ShaderVisibility keeps them separate, global unique numbering improves readability
cbuffer LightBuffer : register(b2)
{
    float3 lightDirection;
    float padding;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
    float3 viewDirection : TEXCOORD1;
};

float DistributionGGX(float NdotH, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    return a2 / max(denom * denom, 0.0001f);
}

float GeometrySmith(float NdotV, float NdotL, float roughness)
{
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    float smithV = NdotV / (NdotV * (1.0f - k) + k);
    float smithL = NdotL / (NdotL * (1.0f - k) + k);
    return smithV * smithL;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0f - F0) * pow(1.0f - cosTheta, 5.0f);
}

// Gamma correction: Linear to sRGB
// All lighting calculations are done in linear space
// Convert to sRGB before output for proper display
float3 LinearToSRGB(float3 linearColor)
{
    // Approximation of sRGB gamma curve (close to pow(x, 1/2.2))
    // Uses exact sRGB specification:
    // if (linear <= 0.0031308) return linear * 12.92
    // else return 1.055 * pow(linear, 1/2.4) - 0.055
    float3 sRGB;
    sRGB.r = linearColor.r <= 0.0031308f ? linearColor.r * 12.92f : 1.055f * pow(linearColor.r, 1.0f / 2.4f) - 0.055f;
    sRGB.g = linearColor.g <= 0.0031308f ? linearColor.g * 12.92f : 1.055f * pow(linearColor.g, 1.0f / 2.4f) - 0.055f;
    sRGB.b = linearColor.b <= 0.0031308f ? linearColor.b * 12.92f : 1.055f * pow(linearColor.b, 1.0f / 2.4f) - 0.055f;
    return sRGB;
}

float4 PbrPixelShader(PixelInputType input) : SV_TARGET
{
    float3 lightDir = normalize(-lightDirection);
    float3 albedo = diffuseTexture.Sample(SampleType, input.tex).rgb;
    float3 rmColor = rmTexture.Sample(SampleType, input.tex).rgb;
    float roughness = saturate(rmColor.r);
    float metallic = saturate(rmColor.b);

    float3 bumpMap = normalMap.Sample(SampleType, input.tex).rgb;
    bumpMap = bumpMap * 2.0f - 1.0f;
    // Reconstruct TBN normal - the result needs normalization due to weighted sum
    float3 bumpNormal =
        normalize(bumpMap.x * input.tangent + bumpMap.y * input.binormal + bumpMap.z * input.normal);

    // IMPORTANT: Re-normalize after rasterizer interpolation
    // Even though viewDirection was normalized in VS, linear interpolation destroys unit length
    // Example: normalize(A) + normalize(B) != normalize(A + B)
    float3 viewDir = normalize(input.viewDirection);
    float3 halfDir = normalize(viewDir + lightDir);

    float NdotV = max(dot(bumpNormal, viewDir), 0.0f);
    float NdotL = max(dot(bumpNormal, lightDir), 0.0f);
    float NdotH = max(dot(bumpNormal, halfDir), 0.0f);
    float HdotV = max(dot(halfDir, viewDir), 0.0f);

    float3 F0 = lerp(float3(0.04f, 0.04f, 0.04f), albedo, metallic);

    float normalDistribution = DistributionGGX(NdotH, roughness);
    float geometricShadow = GeometrySmith(NdotV, NdotL, roughness);
    float3 fresnel = FresnelSchlick(HdotV, F0);

    float3 numerator = normalDistribution * geometricShadow * fresnel;
    float denominator = max(4.0f * NdotV * NdotL, 0.0001f);
    float3 specular = numerator / denominator;

    float3 kS = fresnel;
    float3 kD = (1.0f - kS) * (1.0f - metallic);

    float3 diffuse = kD * albedo / 3.14159265f;

    // Ambient lighting using simple approximation
    // NOTE: This is a simplified approach. For proper PBR, use:
    // - Image-Based Lighting (IBL) with environment maps
    // - Spherical Harmonics for diffuse
    // - Pre-filtered environment maps for specular
    // 
    // Current implementation: Simple hemisphere ambient with energy conservation
    float3 ambient = float3(0.03f, 0.03f, 0.03f) * albedo * kD;  // Only diffuse gets ambient
    
    // Combine: ambient (constant) + directional light contribution
    float3 Lo = (diffuse + specular) * NdotL;  // Direct lighting
    float3 color = ambient + Lo;               // Total lighting
    
    // Apply gamma correction for proper display on sRGB monitors
    // Since render target is DXGI_FORMAT_R8G8B8A8_UNORM (linear),
    // we need to convert from linear space to sRGB
    color = LinearToSRGB(color);
    
    return float4(color, 1.0f);
}

