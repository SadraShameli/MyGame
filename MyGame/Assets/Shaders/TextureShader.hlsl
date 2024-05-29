// Pixel

struct PixelShaderInput
{
	float3 TexCoord : TEXCOORD;
};

TextureCube<float4> Texture : register(t0);
SamplerState LinearClampSampler : register(s0);

float4 PSMain(PixelShaderInput IN) : SV_Target
{
	return Texture.Sample(LinearClampSampler, IN.TexCoord);
}


// Vertex

struct ModelViewProjection
{
	matrix MVP;
};

struct VertexShaderInput
{
	float3 Position : POSITION;
};

struct VertexShaderOutput
{
	float3 TexCoord : TEXCOORD;
	float4 Position : SV_POSITION;
};

ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);

VertexShaderOutput VSMain(VertexShaderInput IN)
{
	VertexShaderOutput OUT;

	OUT.Position = mul(ModelViewProjectionCB.MVP, float4(IN.Position, 1.0f));
	OUT.TexCoord = IN.Position;

	return OUT;
}