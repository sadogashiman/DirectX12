#include "stdafx.h"
#include "Polygon.h"
#include "Support.h"
#include "error.h"
#include "Singleton.h"
#include "Direct3D.h"

void Polygon::init()
{
    HRESULT hr;

    //�C���f�b�N�X���쐬
    std::vector<unsigned int> indices;

    //���W�w��
    std::vector<VertexType> vertices = {
        { { 0.0F,0.25F,0.0F },{ 1.0F,0.0F,0.0F,1.0F }},
        {{ 0.25F,-0.25F,0.0F },{ 0.0F,1.0F,0.0F,1.0F }},
        {{ -0.25F,-0.25F,0.0F },{ 0.0F,0.0F,1.0F,1.0F }},
    };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    //���_�o�b�t�@�ƃC���f�b�N�X�o�b�t�@�̐���
    vertexbuffer_ = createBuffer(sizeof(vertices), &vertices[0]);
    indexbuffer_ = createBuffer(sizeof(indices), &indices[0]);
    indexcount_ = indices.size();

    //�e�o�b�t�@�r���[���쐬
    vertexbufferview_.BufferLocation = vertexbuffer_->GetGPUVirtualAddress();
    vertexbufferview_.SizeInBytes = sizeof(vertices);
    vertexbufferview_.StrideInBytes = sizeof(VertexType);
    indexbufferview_.BufferLocation = indexbuffer_->GetGPUVirtualAddress();
    indexbufferview_.SizeInBytes = sizeof(indices);
    indexbufferview_.Format = DXGI_FORMAT_R32_UINT;

    //�V�F�[�_�[�R���p�C��
    ComPtr<ID3DBlob> errBlob;
    hr = Support::createShaderV6("color_vs.cso", L"vs_6_0", vertexshader_, errBlob);
    if (FAILED(hr))
    {
        Error::showDialog((const char*)errBlob->GetBufferPointer());
    }
    hr = Support::createShaderV6("color_ps.cso", L"ps_6_0", pixelshader_, errBlob);
    if (FAILED(hr))
    {
        Error::showDialog((const char*)errBlob->GetBufferPointer());
    }

    //���[�g�V�O�l�`���̐ݒ�
    CD3DX12_ROOT_SIGNATURE_DESC rootsigdesc{};
    rootsigdesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    hr = D3D12SerializeRootSignature(&rootsigdesc, D3D_ROOT_SIGNATURE_VERSION_1_0,&signature, &errBlob);
    if (FAILED(hr))
    {
        throw std::runtime_error("D3D12SerializeRootSignature failed");
    }

    //���[�g�V�O�l�`���̍쐬
    hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootsignature_));
    if (FAILED(hr))
    {
        throw std::runtime_error("CreateRootSignature failed");
    }

    //���_���̓��C�A�E�g
    D3D12_INPUT_ELEMENT_DESC polygonlayout[2];
    polygonlayout[0].SemanticName = "POSITION";
    polygonlayout[0].SemanticIndex = 0;
    polygonlayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonlayout[0].InputSlot = 0;
    polygonlayout[0].AlignedByteOffset = 0;
    polygonlayout[0].InstanceDataStepRate = 0;

    polygonlayout[1].SemanticName = "COLOR";
    polygonlayout[1].SemanticIndex = 0;
    polygonlayout[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonlayout[1].InputSlot = 0;
    polygonlayout[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    polygonlayout[1].InstanceDataStepRate = 0;

    //�p�C�v���C���X�e�[�g�̐ݒ�
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelinestatedesc;

    //�V�F�[�_�[�̃Z�b�g
    pipelinestatedesc.VS = CD3DX12_SHADER_BYTECODE(vertexshader_.Get());
    pipelinestatedesc.PS = CD3DX12_SHADER_BYTECODE(pixelshader_.Get());

    //�u�����h�X�e�[�g�̐ݒ�
    pipelinestatedesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    //���X���^�C�U�̐ݒ�
    pipelinestatedesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    //�o�͐�̎w��
    pipelinestatedesc.NumRenderTargets = 1;
    pipelinestatedesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    //�f�v�X�o�b�t�@�̃t�H�[�}�b�g�ݒ�
    pipelinestatedesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelinestatedesc.InputLayout = { polygonlayout,_countof(polygonlayout) };
    pipelinestatedesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    //���[�g�V�O�l�`���̃Z�b�g
    pipelinestatedesc.pRootSignature = rootsignature_.Get();
    pipelinestatedesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    //�}���`�T���v��
    pipelinestatedesc.SampleDesc = { 1,0 };
    pipelinestatedesc.SampleMask = UINT_MAX; //�����Y���ƕ`�悳��Ȃ����x�����o�Ȃ����璍��

    //�p�C�v���C���X�e�[�g�̍쐬
    hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateGraphicsPipelineState(&pipelinestatedesc, IID_PPV_ARGS(&pipeline_));
    if (FAILED(hr))
    {
        throw std::runtime_error("CreateGraphicsPipelineState failed");
    }
}

void Polygon::destroy()
{
}

ComPtr<ID3D12Resource1> Polygon::createBuffer(unsigned int BufferSize, const void* InitialData)
{
    HRESULT hr;
    ComPtr<ID3D12Resource1> buffer;
    hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer)
    );

    //�����f�[�^�̎w�肪���݂���ꍇ�R�s�[����
    if (SUCCEEDED(hr) && InitialData != nullptr)
    {
        void* mapped;
        CD3DX12_RANGE range(0, 0);
        hr = buffer->Map(0, &range, &mapped);
        if (SUCCEEDED(hr))
        {
            memcpy(mapped, InitialData, BufferSize);
            buffer->Unmap(0, nullptr);
        }
    }

    return buffer;
}