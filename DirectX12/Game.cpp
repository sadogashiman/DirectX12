#include "stdafx.h"
#include "Game.h"
#include "Singleton.h"
#include "DescriptorManager.h"
#include "imgui_helper.h"
#include "System.h"
#include "imgui.h"
#include "examples\imgui_impl_dx12.h"
#include "examples\imgui_impl_win32.h"
#include "Support.h"

static XMFLOAT4 colorset[] = {
	 XMFLOAT4(1.0F,1.0F,1.0F,1.0F),
	 XMFLOAT4(1.0f, 0.65f, 1.0f, 1.0f),
	 XMFLOAT4(0.1f, 0.5f, 1.0f, 1.0f),
	 XMFLOAT4(0.6f, 1.0f, 0.8f, 1.0f),
};


bool Game::init()
{
	auto descriptor = Singleton<Direct3D>::getPtr()->GetDescriptorManager().get()->Alloc();
	auto d3d = Singleton<Direct3D>::getPtr();
	auto frameindex = d3d->getFrameIndex();
	HRESULT hr;
	void* mapped;
	CD3DX12_RANGE range(0, 0);


	//コマンドリストの準備
	d3d->getCommandAllocatorVector()[frameindex]->Reset();
	d3d->getCommandList()->Reset(d3d->getCommandAllocatorVector()[frameindex].Get(), nullptr);

	colorshader_.reset(new ColorShader);

	//初期化
	colorshader_.get()->init();


	return true;
}

SceneBase* Game::update()
{
	//ImGui更新
	//updateImGui();



	return this;
}

bool Game::render()
{
	auto d3d = Singleton<Direct3D>::getPtr();
	auto swapchain = d3d->getSwapChain();
	auto frameindex = swapchain->getCurrentBackBufferIndex();

	//行列を初期化
	SceneParamerter sceneparam;
	XMStoreFloat4x4(&sceneparam.world, XMMatrixIdentity());
	XMStoreFloat4x4(&sceneparam.view, XMMatrixTranspose(XMMatrixLookAtLH(XMVectorSet(3.0F, 5.0F, 10.0F - cameraoffset_, 0.0F), XMVectorSet(3.0F, 2.0F, 0.0F - cameraoffset_, 0.0F), XMVectorSet(0.0F, 1.0F, 0.0F, 0.0F))));
	XMStoreFloat4x4(&sceneparam.projection, XMMatrixTranspose(XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0F), d3d->getViewport().Width / d3d->getViewport().Height, 0.1F, 1000.0F)));

	//d3d->setFrameIndex(frameindex);
	d3d->getCommandAllocatorVector()[frameindex]->Reset();
	d3d->getCommandList()->Reset(d3d->getCommandAllocatorVector()[frameindex].Get(), nullptr);

	//スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barriertoRT = swapchain->getBarrierToRenderTarget();
	d3d->getCommandList()->ResourceBarrier(1, &barriertoRT);

	auto rtv = swapchain->getCurrentRTV();
	auto dsv = d3d->getDefaultDepthDSV();

	//カラーバッファクリア
	d3d->getCommandList()->ClearRenderTargetView(rtv, d3d->getClearColorPtr(), 0, nullptr);

	//デプスバッファ
	d3d->getCommandList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);

	colorshader_.get()->makeCommand();

	//ImGui描画
	//renderImGui(d3d);

	//レンダーターゲットからスワップチェイン表示可能へ
	auto barriertopresent = swapchain->getBarrierToPresent();
	d3d->getCommandList()->ResourceBarrier(1, &barriertopresent);

	d3d->getCommandList()->Close();

	ID3D12CommandList* lists[] = { d3d->getCommandList().Get() };

	d3d->getCommandQueue()->ExecuteCommandLists(1, lists);

	swapchain->present(1, 0);
	swapchain->waitPreviousFrame(d3d->getCommandQueue(), frameindex, kGpuWaitTimeout);

	return true;
}

void Game::destroy()
{
	//ImGui_ImplWin32_Shutdown();
	//ImGui_ImplDX12_Shutdown();
	//imgui_helper::CleanupImGui();
}

ComPtr<ID3D12Resource1> Game::createBufferResource(D3D12_HEAP_TYPE Type, UINT BufferSize, D3D12_RESOURCE_STATES State)
{
	ComPtr<ID3D12Resource1> ret;
	HRESULT hr;
	const auto heapprops = CD3DX12_HEAP_PROPERTIES(Type);
	const auto resourcedesc = CD3DX12_RESOURCE_DESC::Buffer(BufferSize);
	hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateCommittedResource(
		&heapprops,
		D3D12_HEAP_FLAG_NONE,
		&resourcedesc,
		State,
		nullptr,
		IID_PPV_ARGS(&ret)
	);

	ThrowIfFailed(hr);
	return ret;
}

void Game::updateImGui()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Control");
	ImGui::Text("Using VertexBufferStream");
	ImGui::Text("Framerate(avg)%.3F ms/frame(%.1f FPS)", 1000.0F / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::SliderInt("Count", &instancingdatacount_, 1, kInstanceDataMax);
	ImGui::SliderFloat("Camera", &cameraoffset_, 0.0F, 50.0F);

	//ImGui::SliderFloat("Factor", &factor_, 0.0F, 100.0F);
	//ImGui::ColorEdit4("ClearColor", Singleton<Direct3D>::getPtr()->getClearColorPtr(), ImGuiColorEditFlags_PickerHueWheel);

	//ImGui::ColorPicker4("ClearColor", Singleton<Direct3D>::getPtr()->getClearColorPtr());
	ImGui::End();
}

void Game::renderImGui(Direct3D* D3d)
{
	auto descroptorhandle = Singleton<Direct3D>::getPtr()->getCommandList().Get();



	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), descroptorhandle);
}