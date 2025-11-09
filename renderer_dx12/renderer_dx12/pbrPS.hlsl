Texture2D diffuseTexture : register(t0);
Texture2D normalMap : register(t1);
Texture2D rmTexture : register(t2);
SamplerState SampleType : register(s0);

cbuffer LightBuffer : register(b0)
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

float4 PbrPixelShader(PixelInputType input) : SV_TARGET
{
    float3 lightDir = normalize(-lightDirection);
    float3 albedo = diffuseTexture.Sample(SampleType, input.tex).rgb;
    float3 rmColor = rmTexture.Sample(SampleType, input.tex).rgb;
    float roughness = saturate(rmColor.r);
    float metallic = saturate(rmColor.b);

    float3 bumpMap = normalMap.Sample(SampleType, input.tex).rgb;
    bumpMap = bumpMap * 2.0f - 1.0f;
    float3 bumpNormal =
        normalize(bumpMap.x * input.tangent + bumpMap.y * input.binormal + bumpMap.z * input.normal);

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

    // Add ambient lighting to prevent the model from being too dark
    float3 ambient = float3(0.15f, 0.15f, 0.15f) * albedo;
    
    float3 color = ambient + (diffuse + specular) * NdotL;
    return float4(color, 1.0f);
}

