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

	//���_�o�b�t�@�ƃC���f�b�N�X�o�b�t�@�̍쐬
	vertexbuffer_ = Support::createBuffer(sizeof(vertices), vertices);
	indexbuffer_ = Support::createBuffer(sizeof(indices), indices);
	indexcount_ = _countof(indices);

	//���_�o�b�t�@�r���[�ƃC���f�b�N�X�o�b�t�@�r���[�̍쐬
	vertexbufferview_.BufferLocation = vertexbuffer_->GetGPUVirtualAddress();
	vertexbufferview_.SizeInBytes = sizeof(vertices);
	vertexbufferview_.StrideInBytes = sizeof(Vertex);
	indexbufferview_.BufferLocation = indexbuffer_->GetGPUVirtualAddress();
	indexbufferview_.SizeInBytes = sizeof(indices);
	indexbufferview_.Format = DXGI_FORMAT_R32_UINT;

	//�V�F�[�_�[�R���p�C��
	//ReadDataFromFile(L"color_ps.cso", &pixel.data, &pixel.size);
	//ReadDataFromFile(L"color_vs.cso", &vertex.data, &vertex.size);

	ComPtr<ID3D10Blob> errblob;
	hr = Support::createShaderV6("Resource/Shader/color_vs.hlsl", L"vs_6_0", vs, errblob);
	ThrowIfFailed(hr);
	hr = Support::createShaderV6("Resource/Shader/color_ps.hlsl", L"ps_6_0", ps, errblob);
	ThrowIfFailed(hr);

	//���[�g�V�O�l�`���̍\�z
	CD3DX12_ROOT_SIGNATURE_DESC rootsigdesc{};
	rootsigdesc.Init(
		0,
		nullptr,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	//���[�g�V�O�l�`���̍쐬
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

	//�C���v�b�g���C�A�E�g
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
	//�p�C�v���C���̐���
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelinestatedesc{};

	//�V�O�l�`���̃Z�b�g
	pipelinestatedesc.pRootSignature = rootsignature_.Get();

	//�V�F�[�_�[�̃Z�b�g
	pipelinestatedesc.VS = CD3DX12_SHADER_BYTECODE(vs.Get());
	pipelinestatedesc.PS = CD3DX12_SHADER_BYTECODE(ps.Get());

	//�u�����h�X�e�[�g�̃Z�b�g
	pipelinestatedesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//���X�^���C�U�̃Z�b�g
	pipelinestatedesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//�o�͐��ݒ�
	pipelinestatedesc.NumRenderTargets = 1;
	pipelinestatedesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//�f�v�X�o�b�t�@�̃t�H�[�}�b�g�ݒ�
	pipelinestatedesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	pipelinestatedesc.InputLayout = { inputelementdesc,_countof(inputelementdesc) };
	pipelinestatedesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	//���[�g�V�O�l�`���̃Z�b�g
	pipelinestatedesc.pRootSignature = rootsignature_.Get();
	pipelinestatedesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//�}���`�T���v���ݒ�
	pipelinestatedesc.SampleDesc = { 1,0 };
	pipelinestatedesc.SampleMask = UINT_MAX; //�����Y���ƕ`�悳��Ȃ�

	//�����_�����O�p�C�v���C���̍쐬
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
	//�p�C�v���C���X�e�[�g�̃Z�b�g
	Singleton<Direct3D>::getPtr()->getCommandList()->SetPipelineState(pipeline_.Get());

	//���[�g�V�O�l�`���̃Z�b�g
	Singleton<Direct3D>::getPtr()->getCommandList()->SetGraphicsRootSignature(rootsignature_.Get());

	//�r���[�|�[�g�ƃV�U�[�̃Z�b�g
	Singleton<Direct3D>::getPtr()->getCommandList()->RSSetViewports(1, &viewport);
	Singleton<Direct3D>::getPtr()->getCommandList()->RSSetScissorRects(1, &scissorrect);

	//�v���~�e�B�u�^�C�v�̃Z�b�g
	Singleton<Direct3D>::getPtr()->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	//�o�b�t�@�̃Z�b�g
	Singleton<Direct3D>::getPtr()->getCommandList()->IASetVertexBuffers(0, 1, &vertexbufferview_);
	Singleton<Direct3D>::getPtr()->getCommandList()->IASetIndexBuffer(&indexbufferview_);

	//�`��(�C���X�^���V���O�`��)
	Singleton<Direct3D>::getPtr()->getCommandList()->DrawIndexedInstanced(indexcount_, 1, 0, 0, 0);
}
