#pragma once

struct VertexType
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};
class Polygon
{
private:
	ComPtr<ID3D12Resource1> createBuffer(unsigned int BufferSize, const void* InitialData);

	ComPtr<ID3D12Resource1> vertexbuffer_;
	ComPtr<ID3D12Resource1> indexbuffer_;
	ComPtr<ID3DBlob> pixelshader_;
	ComPtr<ID3DBlob> vertexshader_;
	ComPtr<ID3D12RootSignature> rootsignature_;
	ComPtr<ID3D12PipelineState> pipeline_;
	D3D12_VERTEX_BUFFER_VIEW vertexbufferview_;
	D3D12_INDEX_BUFFER_VIEW indexbufferview_;
	unsigned int indexcount_;



public:
	void init();
	void destroy();

};

