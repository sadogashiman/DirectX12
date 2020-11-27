#pragma once
#include "SceneBase.h"
const UINT kInstanceDataMax = 200;

class Game :public SceneBase
{
public:
	struct SceneParamerter
	{
		XMFLOAT4X4 world;
		XMFLOAT4X4 view;
		XMFLOAT4X4 projection;
	};
private:
	struct ModelData
	{
		UINT indexCount;
		D3D12_VERTEX_BUFFER_VIEW vbView;
		D3D12_INDEX_BUFFER_VIEW  ibView;
		ComPtr<ID3D12Resource1> resourceVB;
		ComPtr<ID3D12Resource1> resourceIB;
	};

	struct InstanceData
	{
		XMFLOAT3 offsetpos;
		XMFLOAT4 color;
	};

	ModelData model_;
	float factor_;
	ComPtr<ID3DBlob> vertexshader_;
	ComPtr<ID3D10Blob> pixelshader_;
	ComPtr<ID3D12RootSignature> rootsignature_;
	ComPtr<ID3D12PipelineState> pipeline_;
	ComPtr<ID3D12Resource1> instancedata_;
	std::vector<ComPtr<ID3D12Resource1>> constantvuffers_;
	D3D12_VERTEX_BUFFER_VIEW streamview_;
	int instancingdatacount_;
	float cameraoffset_;

	ComPtr<ID3D12Resource1> createBufferResource(D3D12_HEAP_TYPE Type, UINT BufferSize, D3D12_RESOURCE_STATES State);
	void updateImGui();
	void renderImGui(Direct3D* Direct3D);
public:
	bool init();
	SceneBase* update();
	bool render();
	void destroy();
};

