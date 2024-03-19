struct VS_INPUT
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoords : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 texCoords : TEXCOORD0;
	float3 worldPos : TEXCOORD1;
};

cbuffer CBufferPerFrame
{
	float totalTime;
	float3 eyePos;
	row_major float4x4 world;
	row_major float4x4 perspective;
	float4 ambient;
	float4 diffuse;
	float4 specular;
	float4 fogColour;
	float2 specularPower;
	float fogStart;
	float fogRange;

};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output;

	//float4x4 scaling = float4x4(0.3, 0.0, 0.0, 0.0, 0.0, 0.3, 0.0, 0.0, 0.0, 0.0, 0.3, 0.0, 0.0, 0.0, 0.0, 1.0);

	float4x4 w = mul(world, perspective);

	output.position = mul(float4(input.position.xyz, 1.0f), w);
	output.worldPos = mul(float4(input.position.xyz, 1.0f), world);
    output.normal = normalize(mul(float4(input.normal, 1.0f), world));
	output.texCoords = input.texCoords;
	//output.eyeDir = normalize(eyePos - output.position.xyz);

	

	return output;
}