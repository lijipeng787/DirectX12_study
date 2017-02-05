Texture2D shaderTextures[3];
SamplerState SampleType;

cbuffer LightBuffer{
	float4 ambientColor;
	float4 diffuseColor;
    float3 lightDirection;
	float padding;
};

struct PixelInputType{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float fogFactor : FOG;
};

float4 LightPixelShader(PixelInputType input) : SV_TARGET{

	float4 texture_color1 = shaderTextures[0].Sample(SampleType, input.tex);

	float4 texture_color2 = shaderTextures[1].Sample(SampleType, input.tex);

	float4 texture_alpha = shaderTextures[2].Sample(SampleType, input.tex);

	float4 alpha_color = texture_color1*texture_alpha + texture_color2*(1.0f - texture_alpha);

	alpha_color = saturate(alpha_color);

	// Set the color of the fog to grey.
    float4 fogColor = float4(0.5f, 0.5f, 0.0f, 1.0f);

    // Calculate the final color using the fog effect equation.
    alpha_color = input.fogFactor * alpha_color + (1.0 - input.fogFactor) * fogColor;

	return alpha_color;
}
