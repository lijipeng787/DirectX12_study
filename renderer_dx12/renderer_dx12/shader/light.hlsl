cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix normalMatrix;
};

cbuffer FogBuffer : register(b1)
{
    float fogStart;
    float fogEnd;
    float padding1;
    float padding2;
};

struct VertexInputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float fogFactor : FOG;
};

PixelInputType LightVertexShader(VertexInputType input)
{
    PixelInputType output;
    float4 input_pos = float4(input.position, 1.0f);

    output.position = mul(input_pos, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;

    // Transform normal to world space using the normal matrix (inverse-transpose of world)
    // This correctly handles non-uniform scaling.
    output.normal = mul(input.normal, (float3x3)normalMatrix);
    output.normal = normalize(output.normal);

    float4 cameraPosition = mul(input_pos, worldMatrix);
    cameraPosition = mul(cameraPosition, viewMatrix);

    // Calculate linear fog.
    output.fogFactor = saturate((fogEnd - cameraPosition.z) / (fogEnd - fogStart));

    return output;
}

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightDirection;
    float padding;
};

float4 LightPixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor = shaderTexture.Sample(SampleType, input.tex);

    float4 color = ambientColor;

    float3 lightDir = normalize(-lightDirection);
    float lightIntensity = saturate(dot(input.normal, lightDir));

    if (lightIntensity > 0.0f)
    {
        color += diffuseColor * lightIntensity;
    }

    color = saturate(color) * textureColor;

    // Fog color - currently hardcoded
    // TODO: Pass fog color via constant buffer for runtime configuration
    const float4 fogColor = float4(0.5f, 0.5f, 0.5f, 1.0f);  // Gray fog instead of yellow-green

    // Calculate the final color using the fog effect equation.
    color = input.fogFactor * color + (1.0f - input.fogFactor) * fogColor;

    return color;
}


