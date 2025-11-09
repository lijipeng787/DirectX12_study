// Constant Buffers - using global unique register numbering across VS/PS
// VS uses b0, b1
// PS uses b2 (see pbrPS.hlsl)
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix normalMatrix;  // Inverse-transpose of world matrix for correct normal transformation
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float padding;
};

struct VertexInputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 binormal : BINORMAL;
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

PixelInputType PbrVertexShader(VertexInputType input)
{
    PixelInputType output;
    float4 worldPosition;

    input.position.w = 1.0f;

    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    output.tex = input.tex;

    // Transform normals to world space using the normal matrix
    // The normal matrix is the inverse-transpose of the world matrix,
    // which correctly handles:
    // - Rotation (orthogonal matrices)
    // - Uniform scaling
    // - NON-UNIFORM scaling (e.g., Scale(2, 1, 1))
    // - Shear transformations
    // - Mirror transformations (e.g., Scale(-1, 1, 1))
    //
    // This ensures normals remain perpendicular to the surface even when
    // the object is scaled non-uniformly or transformed in complex ways.
    float3x3 normal3x3 = (float3x3)normalMatrix;
    output.normal = normalize(mul(input.normal, normal3x3));
    output.tangent = normalize(mul(input.tangent, normal3x3));
    output.binormal = normalize(mul(input.binormal, normal3x3));

    worldPosition = mul(input.position, worldMatrix);
    // Pre-normalize in VS to reduce interpolation error and maintain data range
    // PS will re-normalize because linear interpolation destroys unit length
    output.viewDirection = normalize(cameraPosition - worldPosition.xyz);

    return output;
}

