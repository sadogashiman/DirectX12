#include "stdafx.h"
#include "HDRShader.h"
#include "Singleton.h"
#include "SwapChain.h"

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

	//void* mapped;
	//HRESULT hr;
	CD3DX12_RANGE range(0, 0);

	cmdlist->Reset(cmdallocator[frameindex].Get(), nullptr);


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

	//クリア
	cmdlist->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);
	cmdlist->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);

	//描画先をセット
	cmdlist->OMSetRenderTargets(1, &(D3D12_CPU_DESCRIPTOR_HANDLE)rtv, FALSE, &(D3D12_CPU_DESCRIPTOR_HANDLE)dsv);
	
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
