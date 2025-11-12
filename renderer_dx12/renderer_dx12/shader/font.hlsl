cbuffer PerFrameBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

PixelInputType FontVertexShader(VertexInputType input)
{
    PixelInputType output;

    input.position.w = 1.0f;

    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;

    return output;
}

Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer PixelBuffer : register(b0)
{
    float4 pixelColor;
};

float4 FontPixelShader(PixelInputType input) : SV_TARGET
{
    float4 color = shaderTexture.Sample(SampleType, input.tex);

    // If the color is black on the texture then treat this pixel as transparent.
    // Use threshold comparison instead of exact equality for floating point values
    const float threshold = 0.01f;
    if (color.r < threshold)
    {
        color.a = 0.0f;
    }
    else
    {
        // If the color is other than black on the texture then this is a pixel in the font so draw it using the font pixel color.
        color.rgb = pixelColor.rgb;
        color.a = 1.0f;
    }

    return color;
}


