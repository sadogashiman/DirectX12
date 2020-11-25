#include "stdafx.h"
#include "Game.h"
#include "Singleton.h"
#include "DescriptorManager.h"
#include "imgui_helper.h"
#include "System.h"
#include "imgui.h"
#include "examples\imgui_impl_dx12.h"
#include "examples\imgui_impl_win32.h"


bool Game::init()
{
	auto descriptor = Singleton<Direct3D>::getPtr()->GetDescriptorManager().get()->Alloc();

	CD3DX12_CPU_DESCRIPTOR_HANDLE hcpu(descriptor);
	CD3DX12_GPU_DESCRIPTOR_HANDLE hgpu(descriptor);

	imgui_helper::PrepareImGui(Singleton<System>::getPtr()->getWindowHandle(), Singleton<Direct3D>::getPtr()->getDevice(), Singleton<Direct3D>::getPtr()->getFormat(), kBufferCount, hcpu, hgpu);

	return true;
}

SceneBase* Game::update()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	static float f = 0.0F;
	static int counter = 0;

	ImGui::Begin("Information");
	ImGui::Text("Hello Imgui World");
	ImGui::Text("Framerate(avg)%.3F ms/frame(%.1f FPS)", 1000.0F / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

	if (ImGui::Button("Button"))
	{

	}

	ImGui::SliderFloat("Factor", &factor_, 0.0F, 100.0F);
	ImGui::ColorEdit4("ClearColor", Singleton<Direct3D>::getPtr()->getClearColorPtr(), ImGuiColorEditFlags_PickerHueWheel);

	ImGui::ColorPicker4("ClearColor", Singleton<Direct3D>::getPtr()->getClearColorPtr());
	ImGui::End();

	return this;
}

bool Game::render()
{
	auto d3d = Singleton<Direct3D>::getPtr();
	auto swapchain = d3d->getSwapChain();
	auto frameindex = swapchain->getCurrentBackBufferIndex();
	d3d->setFrameIndex(frameindex);

	d3d->getCommandAllocatorVector()[frameindex]->Reset();
	d3d->getCommandList()->Reset(d3d->getCommandAllocatorVector()[frameindex].Get(),nullptr);

	//スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barriertoRT = swapchain->getBarrierToRenderTarget();
	d3d->getCommandList()->ResourceBarrier(1, &barriertoRT);

	auto rtv = swapchain->getCurrentRTV();
	auto dsv = d3d->getDefaultDepthDSV();

	//カラーバッファクリア
	d3d->getCommandList()->ClearRenderTargetView(rtv, d3d->getClearColorPtr(), 0, nullptr);

	//デプスバッファ
	d3d->getCommandList()->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);

	//描画先セット
	d3d->getCommandList()->OMSetRenderTargets(1, &(D3D12_CPU_DESCRIPTOR_HANDLE)rtv, FALSE, &(D3D12_CPU_DESCRIPTOR_HANDLE)dsv);

	ID3D12DescriptorHeap* heaps[] = { d3d->GetDescriptorManager()->GetHeap().Get() };
	d3d->getCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	ImGui::Render();
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), d3d->getCommandList().Get());

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
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX12_Shutdown();
	imgui_helper::CleanupImGui();
}
