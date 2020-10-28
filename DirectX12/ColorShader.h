#pragma once
class ColorShader
{
private:
	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT4 color;
	};
	ComPtr<ID3D12Resource1>vertexbuffer_;
	ComPtr<ID3D12Resource1>indexbuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexbufferview_;
	D3D12_INDEX_BUFFER_VIEW indexbufferview_;
	ComPtr<ID3DBlob> vertexshader_;
	ComPtr<ID3DBlob> pixelshader_;
	ComPtr<ID3D12RootSignature> rootsignature_;
	ComPtr<ID3D12PipelineState> pipeline_;
	unsigned int indexcount_;

	ComPtr<ID3D12Resource1>createBuffer(unsigned int BufferSize, const void* InitialData);
public:
	ColorShader();
	~ColorShader();
	void init();
	void destroy();
	void makeCommand();
};
