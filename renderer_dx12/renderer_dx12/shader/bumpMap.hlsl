cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
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
    float4 position : SV_POSITION;
    float2 tex      : TEXCOORD0;
    float3 normal   : NORMAL;
    float3 tangent  : TANGENT;
    float3 binormal : BINORMAL;
};

PixelInputType BumpMapVertexShader(VertexInputType input)
{
    PixelInputType output;

    float4 position = float4(input.position, 1.0f);

    output.position = mul(position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;

    float3x3 world3x3 = (float3x3)worldMatrix;
    output.normal   = normalize(mul(input.normal,   world3x3));
    output.tangent  = normalize(mul(input.tangent,  world3x3));
    output.binormal = normalize(mul(input.binormal, world3x3));

    return output;
}

Texture2D shaderTextures[2] : register(t0);
SamplerState SampleType : register(s0);

cbuffer LightBuffer : register(b0)
{
    float4 diffuseColor;
    float3 lightDirection;
    float  padding;
};

float4 BumpMapPixelShader(PixelInputType input) : SV_TARGET
{
    float4 textureColor = shaderTextures[0].Sample(SampleType, input.tex);

    float4 bumpMap = shaderTextures[1].Sample(SampleType, input.tex);
    bumpMap = bumpMap * 2.0f - 1.0f;

    // Correct bump normal calculation using TBN matrix
    // bumpMap contains the normal in tangent space (from normal map texture)
    // We need to transform it to world space using the TBN basis vectors
    float3 bumpNormal = normalize(
        bumpMap.x * input.tangent + 
        bumpMap.y * input.binormal + 
        bumpMap.z * input.normal
    );

    float3 lightDir = normalize(-lightDirection);
    float  lightIntensity = saturate(dot(bumpNormal, lightDir));

    float4 color = saturate(diffuseColor * lightIntensity);
    return color * textureColor;
}

