#include "stdafx.h"
#include "Polygon.h"
#include "Support.h"
#include "error.h"
#include "Singleton.h"
#include "Direct3D.h"

void Polygon::init()
{
    HRESULT hr;

    //インデックスを作成
    std::vector<unsigned int> indices;

    //座標指定
    std::vector<VertexType> vertices = {
        { { 0.0F,0.25F,0.0F },{ 1.0F,0.0F,0.0F,1.0F }},
        {{ 0.25F,-0.25F,0.0F },{ 0.0F,1.0F,0.0F,1.0F }},
        {{ -0.25F,-0.25F,0.0F },{ 0.0F,0.0F,1.0F,1.0F }},
    };

    indices[0] = 0;
    indices[1] = 1;
    indices[2] = 2;

    //頂点バッファとインデックスバッファの生成
    vertexbuffer_ = createBuffer(sizeof(vertices), &vertices[0]);
    indexbuffer_ = createBuffer(sizeof(indices), &indices[0]);
    indexcount_ = indices.size();

    //各バッファビューを作成
    vertexbufferview_.BufferLocation = vertexbuffer_->GetGPUVirtualAddress();
    vertexbufferview_.SizeInBytes = sizeof(vertices);
    vertexbufferview_.StrideInBytes = sizeof(VertexType);
    indexbufferview_.BufferLocation = indexbuffer_->GetGPUVirtualAddress();
    indexbufferview_.SizeInBytes = sizeof(indices);
    indexbufferview_.Format = DXGI_FORMAT_R32_UINT;

    //シェーダーコンパイル
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

    //ルートシグネチャの設定
    CD3DX12_ROOT_SIGNATURE_DESC rootsigdesc{};
    rootsigdesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    hr = D3D12SerializeRootSignature(&rootsigdesc, D3D_ROOT_SIGNATURE_VERSION_1_0,&signature, &errBlob);
    if (FAILED(hr))
    {
        throw std::runtime_error("D3D12SerializeRootSignature failed");
    }

    //ルートシグネチャの作成
    hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootsignature_));
    if (FAILED(hr))
    {
        throw std::runtime_error("CreateRootSignature failed");
    }

    //頂点入力レイアウト
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

    //パイプラインステートの設定
    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelinestatedesc;

    //シェーダーのセット
    pipelinestatedesc.VS = CD3DX12_SHADER_BYTECODE(vertexshader_.Get());
    pipelinestatedesc.PS = CD3DX12_SHADER_BYTECODE(pixelshader_.Get());

    //ブレンドステートの設定
    pipelinestatedesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    //ラスラタイザの設定
    pipelinestatedesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

    //出力先の指定
    pipelinestatedesc.NumRenderTargets = 1;
    pipelinestatedesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

    //デプスバッファのフォーマット設定
    pipelinestatedesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    pipelinestatedesc.InputLayout = { polygonlayout,_countof(polygonlayout) };
    pipelinestatedesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    //ルートシグネチャのセット
    pipelinestatedesc.pRootSignature = rootsignature_.Get();
    pipelinestatedesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

    //マルチサンプル
    pipelinestatedesc.SampleDesc = { 1,0 };
    pipelinestatedesc.SampleMask = UINT_MAX; //これを忘れると描画されない＆警告も出ないから注意

    //パイプラインステートの作成
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

    //初期データの指定が存在する場合コピーする
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