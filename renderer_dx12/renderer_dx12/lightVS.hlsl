cbuffer MatrixBuffer
{
	matrix worldMatrix;
	matrix viewMatrix;
	matrix projectionMatrix;
};

cbuffer FogBuffer
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
    float4 input_pos = float4(input.position.x, input.position.y, input.position.z, 1.0f);
    	
    output.position = mul(input_pos, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
	output.tex = input.tex;
	
	// Transform normal to world space
	// NOTE: Direct multiplication only works for orthogonal/uniform-scale matrices
	// For non-uniform scaling, use inverse-transpose of world matrix
	output.normal = mul(input.normal, (float3x3)worldMatrix);
	
	output.normal = normalize(output.normal);
    
    float4 cameraPosition;
    // Calculate the camera position.
    cameraPosition = mul(input_pos, worldMatrix);
    cameraPosition = mul(cameraPosition, viewMatrix);

    // Calculate linear fog.    
    output.fogFactor = saturate((fogEnd - cameraPosition.z) / (fogEnd - fogStart));
	
    return output;
}
