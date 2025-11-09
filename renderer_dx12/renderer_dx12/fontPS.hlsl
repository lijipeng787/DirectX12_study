struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

Texture2D shaderTexture;
SamplerState SampleType;

cbuffer PixelBuffer
{
    float4 pixelColor;
};

float4 FontPixelShader(PixelInputType input) : SV_TARGET
{
	float4 color;
	
	color = shaderTexture.Sample(SampleType, input.tex);
	
	// If the color is black on the texture then treat this pixel as transparent.
	// Use threshold comparison instead of exact equality for floating point values
	const float threshold = 0.01f;
	if(color.r < threshold)
	{
		color.a = 0.0f;
	}
	
	// If the color is other than black on the texture then this is a pixel in the font so draw it using the font pixel color.
	else
	{
		color.rgb = pixelColor.rgb;
		color.a = 1.0f;
	}

    return color;
}
