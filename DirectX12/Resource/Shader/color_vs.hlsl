struct VertexInputType
{
    float4 position : POSITION;
    float4 color : COLOR;
};

struct PixelInputType
{
    float4 positon : SV_POSITION;
    float4 color : COLOR;
};

PixelInputType main(VertexInputType Input)
{
    PixelInputType output;
    
    //���͂����̂܂܏o�͂ɃR�s�[
    output.positon = Input.position;
    output.color = Input.color;
    
    return output;
}