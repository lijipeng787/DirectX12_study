cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float  padding;
};

struct VertexInputType
{
    float3 position : POSITION;
    float2 tex      : TEXCOORD0;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
};

struct PixelInputType
{
    float4 position      : SV_POSITION;
    float2 tex           : TEXCOORD0;
    float3 normal        : NORMAL;
    float3 tangent       : TANGENT;
    float3 binormal      : BINORMAL;
    float3 viewDirection : TEXCOORD1;
};

PixelInputType SpecMapVertexShader(VertexInputType input)
{
    PixelInputType output;

    float4 position = float4(input.position, 1.0f);

    float4 worldPosition = mul(position, worldMatrix);
    output.position = mul(worldPosition, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;

    float3x3 world3x3 = (float3x3)worldMatrix;
    output.normal   = normalize(mul(input.normal,   world3x3));
    output.tangent  = normalize(mul(input.tangent,  world3x3));
    output.binormal = normalize(mul(input.binormal, world3x3));

    output.viewDirection = normalize(cameraPosition - worldPosition.xyz);

    return output;
}

Texture2D shaderTextures[3] : register(t0);
SamplerState SampleType : register(s0);

cbuffer LightBuffer : register(b0)
{
    float4 diffuseColor;
    float4 specularColor;
    float3 lightDirection;
    float  specularPower;
};

float4 SpecMapPixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor = shaderTextures[0].Sample(SampleType, input.tex);

    float4 bumpMap = shaderTextures[1].Sample(SampleType, input.tex);
    bumpMap = bumpMap * 2.0f - 1.0f;

    float3 bumpNormal =
        normalize(input.normal + bumpMap.x * input.tangent + bumpMap.y * input.binormal);

    float3 lightDir = normalize(-lightDirection);
    float lightIntensity = saturate(dot(bumpNormal, lightDir));

    float4 color = saturate(diffuseColor * lightIntensity) * textureColor;

    if (lightIntensity > 0.0f)
    {
        float4 specularIntensity = shaderTextures[2].Sample(SampleType, input.tex);
        float3 reflection = normalize(2.0f * lightIntensity * bumpNormal - lightDir);
        float specularFactor = pow(saturate(dot(reflection, input.viewDirection)), specularPower);
        float4 specular = specularFactor * specularIntensity * specularColor;
        color = saturate(color + specular);
    }

    return color;
}

