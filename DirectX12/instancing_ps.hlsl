struct PixelInputType
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

float4 main(PixelInputType Input) : SV_TARGET
{
    return Input.Color;
}