#include "stdafx.h"
#include "Model.h"
#include "Singleton.h"
#include "Direct3D.h"
#include "Loader.h"
#include "Support.h"
#include "D3D12BookUtil.h"

#define DRAW_GROUP_NORMAL std::string("normalDraw")
#define DRAW_GROUP_OUTLINE std::string("outlineDraw")
#define DRAW_GROUP_SHADOW std::string("shadowDraw")

inline Model::PMDVertex convertTo(const loader::PMDVertex& V)
{
	return Model::PMDVertex{
		V.getPosition(),
		V.getNormal(),
		V.getUV(),
		XMUINT2(V.getBoneIndex(0),V.getBoneIndex(1)),
		XMFLOAT2(V.getBoneWeight(0),V.getBoneWeight(1)),
		V.getEdgeFlag()
	};
}

inline XMFLOAT4 toFloat4(const XMFLOAT3& XYZ, float A)
{
	return XMFLOAT4(XYZ.x, XYZ.y, XYZ.z, A);
}

inline XMFLOAT3 operator+ (const XMFLOAT3& A, const XMFLOAT3& B)
{
	return XMFLOAT3{ A.x + B.x,A.y + B.y,A.z + B.z };
}

inline XMFLOAT3 operator- (const XMFLOAT3& A, const XMFLOAT3& B)
{
	return XMFLOAT3{ A.x - B.x,A.y - B.y,A.z - B.z };
}

inline XMFLOAT3 operator* (const XMFLOAT3& A, float B)
{
	return XMFLOAT3{ A.x * B,A.y * B,A.z * B };
}

inline XMFLOAT3& operator+= (XMFLOAT3& A, const XMFLOAT3& B)
{
	A.x += B.x;
	A.y += B.y;
	A.z += B.z;
	return A;
}

void Material::update()
{
	//更新
	Singleton<Direct3D>::getPtr()->writeToUploadHeapMemory(materialcbuffer_.resource.Get(), sizeof(materialparam_), &materialparam_);
}

Bone::Bone() :name_(), translation_(), rotation_(), initialtranslation_(), parent_(nullptr), localmatrix_(), invbindmatrix_()
{
	rotation_ = XMQuaternionIdentity();
}

Bone::Bone(const std::string& Name) : name_(Name), translation_(), rotation_(), initialtranslation_(), parent_(nullptr), localmatrix_(), invbindmatrix_()
{
	rotation_ = XMQuaternionIdentity();
}

void Model::initRootSignature()
{
	//t0
	CD3DX12_DESCRIPTOR_RANGE deffusetexrange;
	deffusetexrange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	//t1
	CD3DX12_DESCRIPTOR_RANGE shadowtexrange;
	shadowtexrange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

	std::array<CD3DX12_ROOT_PARAMETER, 5>rootparam;
	rootparam[0].InitAsConstantBufferView(0); //sceneparam
	rootparam[1].InitAsConstantBufferView(1); //boneparam
	rootparam[2].InitAsConstantBufferView(2); //materialparam
	rootparam[3].InitAsDescriptorTable(1, &deffusetexrange, D3D12_SHADER_VISIBILITY_PIXEL);
	rootparam[4].InitAsDescriptorTable(1, &shadowtexrange, D3D12_SHADER_VISIBILITY_PIXEL);

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 2> samplerdesc;
	samplerdesc[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	samplerdesc[1].Init(1, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_BORDER, D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	samplerdesc[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;

	CD3DX12_ROOT_SIGNATURE_DESC rootsignaturedesc{};
	rootsignaturedesc.Init(UINT(rootparam.size()), rootparam.data(), UINT(samplerdesc.size()), samplerdesc.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	HRESULT hr;
	ComPtr<ID3DBlob> signature, errblob;
	hr = D3D12SerializeRootSignature(&rootsignaturedesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &errblob);
	ThrowIfFailed(hr);

	//ルートシグネチャの作成
	auto device = Singleton<Direct3D>::getPtr()->getDevice();
	hr = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootsignature_));
	ThrowIfFailed(hr);
}

static void checkCompilerError(HRESULT Hr, ComPtr<ID3D10Blob>& ErrorBlob)
{
	if (FAILED(Hr) && ErrorBlob)
	{
		const char* msg = reinterpret_cast<const char*>(ErrorBlob->GetBufferPointer());
		OutputDebugString(msg);
		OutputDebugString("\n");
	}

	ThrowIfFailed(Hr);
}

void Model::initPipelineState()
{
	HRESULT hr;
	ComPtr<ID3DBlob> errblob;

	auto rasterizerdesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	rasterizerdesc.FrontCounterClockwise = true;
	rasterizerdesc.CullMode = D3D12_CULL_MODE_NONE;

	//シェーダーコンパイル
	ComPtr<ID3D10Blob> modelvs, modelps;
	checkCompilerError(Support::createShaderV6("model_vs.hlsl", L"vs_6_0", modelvs, errblob), errblob);
	checkCompilerError(Support::createShaderV6("model_ps.hlsl", L"ps_6_5", modelps, errblob), errblob);

	ComPtr<ID3D10Blob> outlinevs, outlineps;
	checkCompilerError(Support::createShaderV6("outline_vs.hlsl", L"vs_6_0", outlinevs, errblob), errblob);
	checkCompilerError(Support::createShaderV6("outline_ps.hlsl", L"ps_6_0", outlineps, errblob), errblob);

	ComPtr<ID3D10Blob> shadowvs, shadowps;
	checkCompilerError(Support::createShaderV6("shadow_vs.hlsl", L"vs_6_0", shadowvs, errblob), errblob);
	checkCompilerError(Support::createShaderV6("shadow_ps.hlsl", L"ps_6_0", shadowps, errblob), errblob);

	D3D12_INPUT_ELEMENT_DESC inputdesc[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BLENDINDICES",0,DXGI_FORMAT_R32G32_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"BLENDWEIGHTS",0,DXGI_FORMAT_R32G32_FLOAT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"EDGEFLAG",0,DXGI_FORMAT_R32_UINT,0,D3D12_APPEND_ALIGNED_ELEMENT,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	auto modelpsodesc = book_util::CreateDefaultPsoDesc(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		modelvs,
		modelps,
		rasterizerdesc,
		inputdesc,
		_countof(inputdesc),
		rootsignature_.Get()
	);

	//合成を有効に
	modelpsodesc.BlendState.RenderTarget[0].BlendEnable = true;

	auto outliners = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	outliners.FrontCounterClockwise = true;
	outliners.CullMode = D3D12_CULL_MODE_FRONT;
	auto outlinepso = book_util::CreateDefaultPsoDesc(
		DXGI_FORMAT_R8G8B8A8_UNORM,
		outlinevs,
		outlineps,
		outliners,
		inputdesc,
		_countof(inputdesc),
		rootsignature_.Get()
	);

	auto shadowpso = book_util::CreateDefaultPsoDesc(
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		shadowvs,
		shadowps,
		rasterizerdesc,
		inputdesc,
		_countof(inputdesc),
		rootsignature_.Get()
	);

	//グラフィックスパイプラインの作成
	auto device = Singleton<Direct3D>::getPtr()->getDevice();
	ComPtr<ID3D12PipelineState> pso;
	hr = device->CreateGraphicsPipelineState(&modelpsodesc, IID_PPV_ARGS(&pso));
	ThrowIfFailed(hr);

	pipelinestate_[DRAW_GROUP_NORMAL] = pso;
	hr = device->CreateGraphicsPipelineState(&outlinepso, IID_PPV_ARGS(&pso));
	ThrowIfFailed(hr);

	pipelinestate_[DRAW_GROUP_OUTLINE] = pso;
	hr = device->CreateGraphicsPipelineState(&shadowpso, IID_PPV_ARGS(&pso));
	ThrowIfFailed(hr);

	pipelinestate_[DRAW_GROUP_SHADOW] = pso;
}

void Model::initConstantBuffer()
{
	auto sceneparamdesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(SceneParameter));
	sceneparametercb_ = Singleton<Direct3D>::getPtr()->createConstantBuffers(sceneparamdesc);

	auto boneparamdesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(BoneParameter));
	boneparametercb_ = Singleton<Direct3D>::getPtr()->createConstantBuffers(boneparamdesc);
}

void Model::initBundles()
{
	auto imagecnt = kBufferCount;
	bundlenormaldraw_ = Singleton<Direct3D>::getPtr()->CreateBundleCommandList();
	ID3D12DescriptorHeap* heaps[] = {
		Singleton<Direct3D>::getPtr()->GetDescriptorManager()->GetHeap().Get(),
	};

	bundlenormaldraw_->SetDescriptorHeaps(1, heaps);
	bundlenormaldraw_->SetGraphicsRootSignature(rootsignature_.Get());
	bundlenormaldraw_->SetPipelineState(pipelinestate_[DRAW_GROUP_NORMAL].Get());
	bundlenormaldraw_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	D3D12_INDEX_BUFFER_VIEW ibview;
	ibview.BufferLocation = indexbuffer_->GetGPUVirtualAddress();
	ibview.Format = DXGI_FORMAT_R32_UINT;
	ibview.SizeInBytes = indexbuffersize_;
	bundlenormaldraw_->IASetIndexBuffer(&ibview);

	//輪郭線描画用Bundle
	bundleoutline_ = Singleton<Direct3D>::getPtr()->CreateBundleCommandList();
	bundleoutline_->SetDescriptorHeaps(1, heaps);
	bundleoutline_->SetGraphicsRootSignature(rootsignature_.Get());
	bundleoutline_->SetPipelineState(pipelinestate_[DRAW_GROUP_OUTLINE].Get());
	bundleoutline_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	bundleoutline_->IASetIndexBuffer(&ibview);
	for (uint32_t i = 0; i < uint32_t(material_.size()); ++i)
	{
		auto mesh = meshes_[i];
		const auto& material = material_[i];

		auto materialcb = material.getConstantBuffer().resource;
		bundlenormaldraw_->SetGraphicsRootConstantBufferView(2, materialcb->GetGPUVirtualAddress());

		auto texturedescriptor = dummytexdescriptor_;
		if (material.hasTexture())
		{
			texturedescriptor = material.getTextureDescriptor();
		}

		bundlenormaldraw_->SetGraphicsRootDescriptorTable(3, texturedescriptor);
		bundlenormaldraw_->DrawIndexedInstanced(mesh.indexcount, 1, mesh.indexoffset, 0, 0);
	}

	bundlenormaldraw_->Close();

	//シャドウ描画用Bundle
	bundleshadow_ = Singleton<Direct3D>::getPtr()->CreateBundleCommandList();
	bundleshadow_->SetDescriptorHeaps(1, heaps);
	bundleshadow_->SetGraphicsRootSignature(rootsignature_.Get());
	bundleshadow_->SetPipelineState(pipelinestate_[DRAW_GROUP_SHADOW].Get());
	bundleshadow_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	bundleshadow_->IASetIndexBuffer(&ibview);
	for (uint32_t i = 0; i < uint32_t(material_.size()); ++i)
	{
		auto mesh = meshes_[i];
		const auto& material = material_[i];
		auto materialcb = material.getConstantBuffer().resource;
		bundleshadow_->SetGraphicsRootConstantBufferView(2, materialcb->GetGPUVirtualAddress());
		bundleshadow_->DrawIndexedInstanced(mesh.indexcount, 1, mesh.indexoffset, 0, 0);
	}

	bundleshadow_->Close();
}

void Model::initDummyTexture()
{
	auto device = Singleton<Direct3D>::getPtr()->getDevice();
	ScratchImage image;
	image.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, 1, 1);
	auto metadata = image.GetMetadata();

	std::vector<D3D12_SUBRESOURCE_DATA> subresource;
	ComPtr<ID3D12Resource> texture;
	CreateTexture(device, metadata, &texture);

	//転送先ステージングバッファの用意
	PrepareUpload(device, image.GetImages(), image.GetImageCount(), metadata, subresource);

	auto totalbyte = GetRequiredIntermediateSize(texture.Get(), 0, UINT(subresource.size()));
	auto staging = Singleton<Direct3D>::getPtr()->createResource(CD3DX12_RESOURCE_DESC::Buffer(totalbyte), D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, D3D12_HEAP_TYPE_UPLOAD);

	auto command = Singleton<Direct3D>::getPtr()->CreateBundleCommandList();
	UpdateSubresources(command.Get(), texture.Get(), staging.Get(), 0, 0, UINT(subresource.size()), subresource.data());
	Singleton<Direct3D>::getPtr()->finishCommandList(command);

	//テクスチャのディスクリプタを準部
	D3D12_SHADER_RESOURCE_VIEW_DESC srvdesc{};
	srvdesc.Format = metadata.format;
	srvdesc.Texture2D.MipLevels = UINT(metadata.mipLevels);
	srvdesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvdesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	dummytexdescriptor_ = Singleton<Direct3D>::getPtr()->GetDescriptorManager()->Alloc();
	device->CreateShaderResourceView(texture.Get(), &srvdesc, dummytexdescriptor_);
}

void Model::computeMorph()
{
	auto vertexcnt = facebaseinfo_.verticespos.size();

	//位置のリセット
	for (uint32_t i = 0; i < vertexcnt; ++i)
	{
		auto offsetindex = facebaseinfo_.indices[i];
		hostmemvertices_[offsetindex].position = facebaseinfo_.verticespos[i];
	}

	//ウェイトに応じて頂点を変更
	for (uint32_t faceindex = 0; faceindex < faceoffsetinfo_.size(); ++faceindex)
	{
		const auto& face = faceoffsetinfo_[faceindex];
		float w = facemophweights_[faceindex];

		for (uint32_t i = 0; i < face.indices.size(); ++i)
		{
			auto basevertexindex = face.indices[i];
			auto dispplacement = face.verticesoffset[i];

			auto offsetindex = facebaseinfo_.indices[basevertexindex];
			XMFLOAT3 offset = dispplacement * w;
			hostmemvertices_[offsetindex].position += offset;
		}
	}
}

void Model::init(const char* Filename)
{
	std::ifstream ifs(Filename, std::ios::binary);
	loader::PMDFile loader(ifs);


}

void Model::destroy()
{
}
