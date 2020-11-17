#pragma once
namespace ModelData
{
	struct DataType
	{
		UINT indexCount;
		D3D12_VERTEX_BUFFER_VIEW vbView;
		D3D12_INDEX_BUFFER_VIEW  ibView;
		ComPtr<ID3D12Resource1> resourceVB;
		ComPtr<ID3D12Resource1> resourceIB;

		ComPtr<ID3D12RootSignature> rootSig;
		ComPtr<ID3D12PipelineState> pipeline;
		std::vector< ComPtr<ID3D12Resource1>> sceneCB;
	};
}