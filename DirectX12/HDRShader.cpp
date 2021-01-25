#include "stdafx.h"
#include "HDRShader.h"
#include "Singleton.h"
#include "SwapChain.h"
#include "Direct3D.h"
#include "Support.h"

HDRShader::HDRShader()
{
	d3d_ = Singleton<Direct3D>::getPtr();
}

HDRShader::~HDRShader()
{
}

void HDRShader::init()
{

	//バッファ転送のためにコマンドアロケーターを準備
	d3d_->getCommandAllocatorVector()[d3d_->getFrameIndex()]->Reset();

	initModel();

}

void HDRShader::render()
{
	auto swapchain = d3d_->getSwapChain();
	d3d_->setFrameIndex(swapchain->getCurrentBackBufferIndex());
	d3d_->getCommandAllocatorVector()[d3d_->getFrameIndex()]->Reset();
	d3d_->getCommandList()->Reset(d3d_->getCommandAllocatorVector()[d3d_->getFrameIndex()].Get(), nullptr);

	ID3D12DescriptorHeap* heaps[] = { d3d_->GetDescriptorManager()->GetHeap().Get() };
	d3d_->getCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	//スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barriertoRT = swapchain->getBarrierToRenderTarget();
	d3d_->getCommandList()->ResourceBarrier(1, &barriertoRT);

	renderModel();

	//レンダーターゲットからスワップチェイン表示可能へ
	auto barriertoPresent = swapchain->getBarrierToPresent();
	d3d_->getCommandList()->ResourceBarrier(1, &barriertoPresent);

	d3d_->getCommandList()->Close();
	ID3D12CommandList* lists[] = { d3d_->getCommandList().Get() };
	d3d_->getCommandQueue()->ExecuteCommandLists(1, lists);

	swapchain->present(1, 0);
	swapchain->waitPreviousFrame(d3d_->getCommandQueue(), d3d_->getFrameIndex(), kGpuWaitTimeout);
}

void HDRShader::destroy()
{
}

void HDRShader::initModel()
{
	auto cmdqueue = d3d_->getCommandQueue();
	auto cmdlist = d3d_->getCommandList();
	auto swapchain = d3d_->getSwapChain();
	auto frameindex = d3d_->getFrameIndex();
	auto cmdallocator = d3d_->getCommandAllocatorVector();

	void* mapped;
	HRESULT hr;
	CD3DX12_RANGE range(0, 0);

	//UINT buffersize = sizeof(TeapotModel::TeapotVerticesPN);
	//auto vbdesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);

	////バッファを設定
	//model_.resourceVB = d3d_->createResource(vbdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, D3D12_HEAP_TYPE_DEFAULT);
	//auto uploadvb = d3d_->createResource(vbdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, D3D12_HEAP_TYPE_UPLOAD);

	//hr = uploadvb->Map(0, nullptr, &mapped);
	//if (SUCCEEDED(hr))
	//{
	//	memcpy(mapped, TeapotModel::TeapotVerticesPN, buffersize);
	//	uploadvb->Unmap(0, nullptr);
	//}

	//model_.vbView.BufferLocation = model_.resourceVB->GetGPUVirtualAddress();
	//model_.vbView.SizeInBytes = buffersize;
	//model_.vbView.StrideInBytes = sizeof(TeapotModel::Vertex);

	//d3d_->getCommandList()->CopyResource(model_.resourceVB.Get(), uploadvb.Get());

	//buffersize = sizeof(TeapotModel::TeapotIndices);
	//auto ibdesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);

	//model_.resourceIB = d3d_->createResource(ibdesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, D3D12_HEAP_TYPE_DEFAULT);
	//auto uploadIB = d3d_->createResource(ibdesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, D3D12_HEAP_TYPE_UPLOAD);

	//hr = uploadIB->Map(0, nullptr, &mapped);
	//if (SUCCEEDED(hr))
	//{
	//	memcpy(mapped, TeapotModel::TeapotIndices, buffersize);
	//	uploadIB->Unmap(0, nullptr);
	//}

	//model_.ibView.BufferLocation = model_.resourceIB->GetGPUVirtualAddress();
	//model_.ibView.SizeInBytes = buffersize;
	//model_.ibView.Format = DXGI_FORMAT_R32_UINT;
	//model_.indexCount = _countof(TeapotModel::TeapotIndices);

	//d3d_->getCommandList()->CopyResource(model_.resourceVB.Get(), uploadvb.Get());

	//コピー処理が終わったので各バッファの状態を変更
	auto barriervb = CD3DX12_RESOURCE_BARRIER::Transition(
		model_.resourceVB.Get(), 
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);

	auto barrierib = CD3DX12_RESOURCE_BARRIER::Transition(
		model_.resourceIB.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_INDEX_BUFFER
	);

	D3D12_RESOURCE_BARRIER barriers[] = { barriervb,barrierib };

	d3d_->getCommandList()->ResourceBarrier(_countof(barriers), barriers);

	d3d_->getCommandList()->Close();

	ID3D12CommandList* command[] = { d3d_->getCommandList().Get() };
	d3d_->getCommandQueue()->ExecuteCommandLists(1, command);

	d3d_->waitForIdleGPU();


	ComPtr<ID3DBlob> errblob, vs, ps;
	hr = Support::createShaderV6(L"HDR_vs.hlsl", L"vs_6_0", vs, errblob);
	ThrowIfFailed(hr);
	hr = Support::createShaderV6(L"HDR_ps.hlsl", L"ps_6_0", ps, errblob);
	ThrowIfFailed(hr);

	//ルートシグネチャ構築
	CD3DX12_ROOT_PARAMETER rootparams[1];
	rootparams[0].InitAsConstantBufferView(0);
	CD3DX12_ROOT_SIGNATURE_DESC rootsigdesc{};
	rootsigdesc.Init(
		_countof(rootparams),
		rootparams,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	ComPtr<ID3DBlob> signature;
	hr = D3D12SerializeRootSignature(&rootsigdesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &errblob);
	ThrowIfFailed(hr);

	hr = d3d_->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&model_.rootSig));
	ThrowIfFailed(hr);



	auto surfaceformat = swapchain->getFormat();

	//パイプラインステート
	//auto psodesc = book_util::CreateDefaultPsoDesc(
	//	surfaceformat,
	//	vs,
	//	ps,
	//	book_util::CreateTeapotModelRasterizerDesc(),
	//	inputelementdesc,
	//	_countof(inputelementdesc),
	//	model_.rootSig
	//);
	//hr = d3d_->getDevice()->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(&model_.pipeline));
	//ThrowIfFailed(hr);

	////定数バッファの作成
	//buffersize = sizeof(SceneParameter);
	//buffersize = book_util::RoundupConstantBufferSize(buffersize);
	//auto cbdesc = CD3DX12_RESOURCE_DESC::Buffer(buffersize);
	//for (auto& cb : d3d_->createConstantBuffers(cbdesc))
	//{
	//	model_.sceneCB.push_back(cb);
	//}
}

void HDRShader::renderModel()
{
	float width = static_cast<float>(d3d_->getWidth());
	float height = static_cast<float>(d3d_->getHeight());
	auto cmdqueue = d3d_->getCommandQueue();
	auto cmdlist = d3d_->getCommandList();
	auto swapchain = d3d_->getSwapChain();
	auto frameindex = d3d_->getFrameIndex();

	auto rtv = swapchain->getCurrentRTV();
	auto dsv = d3d_->getDefaultDepthDSV();

	//描画先をセット
	//cmdlist->OMSetRenderTargets(1, &(D3D12_CPU_DESCRIPTOR_HANDLE)rtv, FALSE, &(D3D12_CPU_DESCRIPTOR_HANDLE)dsv);
	D3D12_CPU_DESCRIPTOR_HANDLE handlertvs[] = { rtv };
	D3D12_CPU_DESCRIPTOR_HANDLE handledsv = dsv;

	cmdlist->OMSetRenderTargets(1, handlertvs, FALSE, &handledsv);

	//ビューポートとシザーのセット
	auto viewport = CD3DX12_VIEWPORT(0.0F, 0.0F, float(d3d_->getWidth()), float(d3d_->getHeight()));
	auto scissorrect = CD3DX12_RECT(0, 0, LONG(d3d_->getWidth()), LONG(d3d_->getHeight()));

	cmdlist->RSSetViewports(1, &viewport);
	cmdlist->RSSetScissorRects(1, &scissorrect);

	//モデルの描画
	auto cb = model_.sceneCB[frameindex];
	SceneParameter sceneparam{};
	auto world = XMMatrixIdentity();
	auto camerapos = XMVectorSet(0.0F, 0.0F, 0.5F, 0.0F);
	auto view = XMMatrixLookAtLH(camerapos,
		XMVectorSet(0.0F, 0.0F, 0.0F, 0.0F),
		XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F)
	);

	auto projection = XMMatrixPerspectiveFovRH(XMConvertToRadians(45.0F), width/height, 0.1F, 1000.0F);
	XMStoreFloat4x4(&sceneparam.world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&sceneparam.viewProj, XMMatrixTranspose(view * projection));
	XMStoreFloat4(&sceneparam.lightPos, XMVectorSet(0.0F, 10.0F, 10.0F, 0.0F));
	XMStoreFloat4(&sceneparam.cameraPos, camerapos);
	sceneparam.branchFrags.x = swapchain->getFormat() == DXGI_FORMAT_R10G10B10A2_UNORM ? 1.0F : 0.0F;

	void* mapped;
	cb->Map(0, nullptr, &mapped);
	memcpy(mapped, &sceneparam, sizeof(sceneparam));
	cb->Unmap(0, nullptr);

	cmdlist->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdlist->SetGraphicsRootSignature(model_.rootSig.Get());
	cmdlist->SetPipelineState(model_.pipeline.Get());
	cmdlist->IASetVertexBuffers(0, 1, &model_.vbView);
	cmdlist->IASetIndexBuffer(&model_.ibView);
	cmdlist->SetGraphicsRootConstantBufferView(0, model_.sceneCB[d3d_->getFrameIndex()]->GetGPUVirtualAddress());
	cmdlist->DrawIndexedInstanced(model_.indexCount, 1, 0, 0, 0);
}
