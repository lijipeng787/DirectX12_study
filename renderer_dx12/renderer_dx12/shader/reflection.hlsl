cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer ReflectionBuffer : register(b1)
{
    matrix reflectionMatrix;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float4 reflectionPosition : TEXCOORD1;
};

PixelInputType ReflectionVertexShader(VertexInputType input)
{
    PixelInputType output;

    float4 worldPosition = mul(input.position, worldMatrix);
    float4 viewPosition = mul(worldPosition, viewMatrix);
    output.position = mul(viewPosition, projectionMatrix);

    output.tex = input.tex;

    float4 reflectionPosition = mul(input.position, worldMatrix);
    reflectionPosition = mul(reflectionPosition, reflectionMatrix);
    reflectionPosition = mul(reflectionPosition, projectionMatrix);
    output.reflectionPosition = reflectionPosition;

    return output;
}

Texture2D baseTexture : register(t0);
Texture2D reflectionTexture : register(t1);
SamplerState SampleType : register(s0);

float4 ReflectionPixelShader(PixelInputType input) : SV_TARGET
{
    float4 baseColor = baseTexture.Sample(SampleType, input.tex);

    float2 reflectTexCoord;
    reflectTexCoord.x = input.reflectionPosition.x / input.reflectionPosition.w / 2.0f + 0.5f;
    reflectTexCoord.y = -input.reflectionPosition.y / input.reflectionPosition.w / 2.0f + 0.5f;

    float4 reflectionColor = reflectionTexture.Sample(SampleType, reflectTexCoord);

    return lerp(baseColor, reflectionColor, 0.15f);
}

