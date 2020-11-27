struct VertexInputType
{
	float4 Position : POSITION;
	float3 Normal : NORMAL;
	float4 OffsetPos : WORLD_POS;
	float4 Color : BASE_COLOR;
};

struct PixelInputType
{
	float4 Position : SV_POSITION;
	float4 Color : COLOR;
};

cbuffer ShaderParameter: register(b0)
{
	float4x4 world;
	float4x4 view;
	float4x4 projection;
}

PixelInputType main( VertexInputType Input )
{
	PixelInputType output;
	
	//���W���擾
	float4 pos = Input.Position;
	
	//���W�ɃI�t�Z�b�g��K��
	pos.xyz += Input.OffsetPos.xyz;
	
	float4x4 WVP = mul(world, mul(view, projection));
	output.Position = mul(pos, WVP);
	output.Color = saturate(dot(Input.Normal, float3(0, 1, 0))) * 0.5 + 0.5;
	output.Color.a = 1.0F;
	output.Color *= Input.Color;
	
	return output;
}