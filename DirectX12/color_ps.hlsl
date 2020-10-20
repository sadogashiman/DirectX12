struct PixelInputType
{
	float4 positon : SV_POSITION;
	float4 color : COLOR;
};

float4 main(PixelInputType Input) : SV_TARGET
{
	return Input.color;
}