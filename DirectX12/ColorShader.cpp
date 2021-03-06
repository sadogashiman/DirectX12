#include "stdafx.h"
#include "ColorShader.h"
#include "Singleton.h"
#include "Direct3D.h"
#include "DXHelper.h"
#include "Support.h"

ColorShader::ColorShader()
{
}

ColorShader::~ColorShader()
{
}

void ColorShader::init()
{
	HRESULT hr;
	ComPtr<ID3D10Blob> vs, ps;
	Vertex vertices[] = {
	{ {  0.0f, 0.25f, 0.5f }, { 1.0f, 0.0f,0.0f,1.0f} },
	{ { 0.25f,-0.25f, 0.5f }, { 0.0f, 1.0f,0.0f,1.0f} },
	{ {-0.25f,-0.25f, 0.5f }, { 0.0f, 0.0f,1.0f,1.0f} },
	};

	uint32_t indices[] = { 0,1,2 };

	//頂点バッファとインデックスバッファの作成
	vertexbuffer_ = Support::createBuffer(sizeof(vertices), vertices);
	indexbuffer_ = Support::createBuffer(sizeof(indices), indices);
	indexcount_ = _countof(indices);

	//頂点バッファビューとインデックスバッファビューの作成
	vertexbufferview_.BufferLocation = vertexbuffer_->GetGPUVirtualAddress();
	vertexbufferview_.SizeInBytes = sizeof(vertices);
	vertexbufferview_.StrideInBytes = sizeof(Vertex);
	indexbufferview_.BufferLocation = indexbuffer_->GetGPUVirtualAddress();
	indexbufferview_.SizeInBytes = sizeof(indices);
	indexbufferview_.Format = DXGI_FORMAT_R32_UINT;

	//シェーダーコンパイル
	//ReadDataFromFile(L"color_ps.cso", &pixel.data, &pixel.size);
	//ReadDataFromFile(L"color_vs.cso", &vertex.data, &vertex.size);

	ComPtr<ID3D10Blob> errblob;
	hr = Support::createShaderV6("Resource/Shader/color_vs.hlsl", L"vs_6_0", vs, errblob);
	ThrowIfFailed(hr);
	hr = Support::createShaderV6("Resource/Shader/color_ps.hlsl", L"ps_6_0", ps, errblob);
	ThrowIfFailed(hr);

	//ルートシグネチャの構築
	CD3DX12_ROOT_SIGNATURE_DESC rootsigdesc{};
	rootsigdesc.Init(
		0,
		nullptr,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	//ルートシグネチャの作成
	ComPtr<ID3DBlob> signature;
	hr = D3D12SerializeRootSignature(&rootsigdesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &errblob);
	if (FAILED(hr))
	{
		throw std::runtime_error("D3D12SerializeRootSignature faild.");
	}

	hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateRootSignature(
		0,
		signature.Get()->GetBufferPointer(),
		signature.Get()->GetBufferSize(),
		IID_PPV_ARGS(&rootsignature_)
	);
	if (FAILED(hr))
	{
		throw std::runtime_error("CreateRootSignature faild ");
	}

	//インプットレイアウト
	//D3D12_INPUT_ELEMENT_DESC inputelementdesc[2];
	//inputelementdesc[0].SemanticName = "POSITION";
	//inputelementdesc[0].SemanticIndex = 0;
	//inputelementdesc[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//inputelementdesc[0].InputSlot = 0;
	//inputelementdesc[0].AlignedByteOffset = offsetof(Vertex,position);
	//inputelementdesc[0].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	//inputelementdesc[0].InstanceDataStepRate = 0;

	//inputelementdesc[1].SemanticName = "COLOR";
	//inputelementdesc[1].SemanticIndex = 0;
	//inputelementdesc[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//inputelementdesc[1].InputSlot = 0;
	//inputelementdesc[1].AlignedByteOffset = offsetof(Vertex, color);
	//inputelementdesc[1].InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
	//inputelementdesc[1].InstanceDataStepRate = 0;


	D3D12_INPUT_ELEMENT_DESC inputelementdesc[] = {
  { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
  { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT,0, offsetof(Vertex,color), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
	};
	//パイプラインの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelinestatedesc{};

	//シグネチャのセット
	pipelinestatedesc.pRootSignature = rootsignature_.Get();

	//シェーダーのセット
	pipelinestatedesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	pipelinestatedesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	//ブレンドステートのセット
	pipelinestatedesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//ラスタライザのセット
	pipelinestatedesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//出力先を設定
	pipelinestatedesc.NumRenderTargets = 1;
	pipelinestatedesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//デプスバッファのフォーマット設定
	pipelinestatedesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelinestatedesc.InputLayout = { inputelementdesc,_countof(inputelementdesc) };
	pipelinestatedesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	//ルートシグネチャのセット
	pipelinestatedesc.pRootSignature = rootsignature_.Get();
	pipelinestatedesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//マルチサンプル設定
	pipelinestatedesc.SampleDesc = { 1,0 };
	pipelinestatedesc.SampleMask = UINT_MAX; //これを忘れると描画されない

	//レンダリングパイプラインの作成
	hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateGraphicsPipelineState(&pipelinestatedesc, IID_PPV_ARGS(&pipeline_));
	if (FAILED(hr))
	{
		throw std::runtime_error("CreateGraphicsPipelineState faild.");
	}
}

void ColorShader::destroy()
{
	auto index = Singleton<Direct3D>::getPtr()->getSwapChain().get()->getCurrentBackBufferIndex();
	auto fence = Singleton<Direct3D>::getPtr()->getFence()[index];
	auto value = ++Singleton<Direct3D>::getPtr()->getFenceValue()[index];
	Singleton<Direct3D>::getPtr()->getCommandQueue()->Signal(fence.Get(), value);
	fence->SetEventOnCompletion(value, Singleton<Direct3D>::getPtr()->getFenceEventHandle());
	WaitForSingleObject(Singleton<Direct3D>::getPtr()->getFenceEventHandle(), kGpuWaitTimeout);
}

void ColorShader::makeCommand()
{
	auto viewport = Singleton<Direct3D>::getPtr()->getViewport();
	auto scissorrect = Singleton<Direct3D>::getPtr()->getScissorRects();
	//パイプラインステートのセット
	Singleton<Direct3D>::getPtr()->getCommandList()->SetPipelineState(pipeline_.Get());

	//ルートシグネチャのセット
	Singleton<Direct3D>::getPtr()->getCommandList()->SetGraphicsRootSignature(rootsignature_.Get());

	//ビューポートとシザーのセット
	Singleton<Direct3D>::getPtr()->getCommandList()->RSSetViewports(1, &viewport);
	Singleton<Direct3D>::getPtr()->getCommandList()->RSSetScissorRects(1, &scissorrect);

	//プリミティブタイプのセット
	Singleton<Direct3D>::getPtr()->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//バッファのセット
	Singleton<Direct3D>::getPtr()->getCommandList()->IASetVertexBuffers(0, 1, &vertexbufferview_);
	Singleton<Direct3D>::getPtr()->getCommandList()->IASetIndexBuffer(&indexbufferview_);

	//描画(インスタンシング描画)
	Singleton<Direct3D>::getPtr()->getCommandList()->DrawIndexedInstanced(indexcount_, 1, 0, 0, 0);
}
