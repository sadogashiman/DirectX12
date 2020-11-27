#include "stdafx.h"
#include "Game.h"
#include "Singleton.h"
#include "DescriptorManager.h"
#include "imgui_helper.h"
#include "System.h"
#include "imgui.h"
#include "examples\imgui_impl_dx12.h"
#include "examples\imgui_impl_win32.h"
#include "TeapotModel.h"
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
	ShaderData vertexshader, pixelshader;

	//コマンドリストの準備
	d3d->getCommandAllocatorVector()[frameindex]->Reset();
	d3d->getCommandList()->Reset(d3d->getCommandAllocatorVector()[frameindex].Get(), nullptr);

	void* mapped;
	HRESULT hr;
	CD3DX12_RANGE range(0, 0);
	UINT buffersize = sizeof(TeapotModel::TeapotVerticesPN);
	model_.resourceVB = createBufferResource(D3D12_HEAP_TYPE_DEFAULT, buffersize, D3D12_RESOURCE_STATE_COPY_DEST);
	auto uploadVB = createBufferResource(D3D12_HEAP_TYPE_UPLOAD, buffersize, D3D12_RESOURCE_STATE_GENERIC_READ);

	hr = uploadVB->Map(0, nullptr, &mapped);
	if (SUCCEEDED(hr))
	{
		memcpy(mapped, TeapotModel::TeapotVerticesPN, buffersize);
		uploadVB->Unmap(0, nullptr);
	}

	model_.vbView.BufferLocation = model_.resourceVB->GetGPUVirtualAddress();
	model_.vbView.SizeInBytes = buffersize;
	model_.vbView.StrideInBytes = sizeof(TeapotModel::Vertex);

	d3d->getCommandList()->CopyResource(model_.resourceVB.Get(), uploadVB.Get());

	buffersize = sizeof(TeapotModel::TeapotIndices);
	model_.resourceIB = createBufferResource(D3D12_HEAP_TYPE_DEFAULT, buffersize, D3D12_RESOURCE_STATE_COPY_DEST);
	auto uploadIB = createBufferResource(D3D12_HEAP_TYPE_UPLOAD, buffersize, D3D12_RESOURCE_STATE_GENERIC_READ);

	hr = uploadIB->Map(0, nullptr, &mapped);

	if (SUCCEEDED(hr))
	{
		memcpy(mapped, TeapotModel::TeapotIndices, buffersize);
		uploadIB->Unmap(0, nullptr);
	}

	model_.ibView.BufferLocation = model_.resourceIB->GetGPUVirtualAddress();
	model_.ibView.SizeInBytes = buffersize;
	model_.ibView.Format = DXGI_FORMAT_R32_UINT;
	model_.indexCount = _countof(TeapotModel::TeapotIndices);

	d3d->getCommandList()->CopyResource(model_.resourceIB.Get(), uploadIB.Get());

	//インスタンシング描画用のデータを準備
	buffersize = sizeof(InstanceData) * kInstanceDataMax;
	instancedata_ = createBufferResource(D3D12_HEAP_TYPE_DEFAULT, buffersize, D3D12_RESOURCE_STATE_COPY_DEST);

	auto uploadvb2 = createBufferResource(D3D12_HEAP_TYPE_UPLOAD, buffersize, D3D12_RESOURCE_STATE_GENERIC_READ);

	std::vector<InstanceData> data(kInstanceDataMax);
	for (UINT i = 0; i < kInstanceDataMax; ++i)
	{
		data[i].offsetpos.x = (i % 6) * 3.0F;
		data[i].offsetpos.y = 0.0F;
		data[i].offsetpos.z = (i / 6) * -3.0F;
		data[i].color = colorset[i % _countof(colorset)];
	}

	uploadvb2->Map(0, nullptr, &mapped);
	memcpy(mapped, data.data(), buffersize);
	uploadvb2->Unmap(0, nullptr);
	streamview_.BufferLocation = instancedata_->GetGPUVirtualAddress();
	streamview_.SizeInBytes = buffersize;
	streamview_.StrideInBytes = sizeof(InstanceData);

	d3d->getCommandList()->CopyResource(instancedata_.Get(), uploadvb2.Get());

	//コピー処理が終わった後は各バッファのステートを変更
	auto barrierVB = CD3DX12_RESOURCE_BARRIER::Transition(model_.resourceVB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	auto barrierIB = CD3DX12_RESOURCE_BARRIER::Transition(model_.resourceIB.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER);
	auto barrierVB2 = CD3DX12_RESOURCE_BARRIER::Transition(instancedata_.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	D3D12_RESOURCE_BARRIER barriers[] = {
		barrierVB,barrierIB,barrierVB2
	};

	d3d->getCommandList()->ResourceBarrier(_countof(barriers), barriers);
	d3d->getCommandList()->Close();
	ID3D12CommandList* command[] = { d3d->getCommandList().Get() };
	d3d->getCommandQueue()->ExecuteCommandLists(1, command);

	//処理の完了を待機
	d3d->waitForIdleGPU();

	ComPtr<ID3DBlob> errblob;
	ReadDataFromFile(L"instancing_vs.cso", &vertexshader.data, &vertexshader.size);
	ReadDataFromFile(L"instancing_ps.cso", &pixelshader.data, &pixelshader.size);

	//ルートシグネチャ
	CD3DX12_ROOT_PARAMETER rootparams[1];
	rootparams[0].InitAsConstantBufferView(0);
	CD3DX12_ROOT_SIGNATURE_DESC rootsigdesc{};
	rootsigdesc.Init(_countof(rootparams), rootparams, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	ComPtr<ID3DBlob> signature;
	
	hr = D3D12SerializeRootSignature(&rootsigdesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &signature, &errblob);
	ThrowIfFailed(hr);

	hr = d3d->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootsignature_));
	ThrowIfFailed(hr);

	//インプットレイアウトの作成
	D3D12_INPUT_ELEMENT_DESC polygonlayout[] = {
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,offsetof(TeapotModel::Vertex,Position),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,offsetof(TeapotModel::Vertex,Normal),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA},
		{"WORLD_POS",0,DXGI_FORMAT_R32G32B32_FLOAT,1,offsetof(InstanceData,offsetpos),D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},
		{"BASE_COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,offsetof(InstanceData,color),D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},
	};

	//パイプラインステートオブジェクトの生成
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psodesc{};

	//シェーダーのセット
	psodesc.VS.pShaderBytecode = vertexshader.data;
	psodesc.VS.BytecodeLength = vertexshader.size;

	psodesc.PS.pShaderBytecode = pixelshader.data;
	psodesc.PS.BytecodeLength = pixelshader.size;

	//ブレンドステート
	psodesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	//ラスタライザステート
	psodesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	//出力先を設定
	psodesc.NumRenderTargets = 1;
	psodesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	//デプスバッファのフォーマット設定
	psodesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psodesc.InputLayout = { polygonlayout,_countof(polygonlayout) };
	psodesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	//ルートシグネチャのセット
	psodesc.pRootSignature = rootsignature_.Get();
	psodesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	//マルチサンプル
	psodesc.SampleDesc = { 1,0 };
	psodesc.SampleMask = UINT_MAX;
		
	hr = d3d->getDevice()->CreateGraphicsPipelineState(&psodesc, IID_PPV_ARGS(&pipeline_));
	ThrowIfFailed(hr);

	//定数バッファの準備
	buffersize = book_util::RoundupConstantBufferSize(sizeof(SceneParamerter));
	for (UINT i = 0; i < kBufferCount; ++i)
	{
		auto cb = createBufferResource(D3D12_HEAP_TYPE_UPLOAD, buffersize, D3D12_RESOURCE_STATE_GENERIC_READ);
		constantvuffers_.push_back(cb);
	}

	//ImGuiのセットアップ
	CD3DX12_CPU_DESCRIPTOR_HANDLE hcpu(descriptor);
	CD3DX12_GPU_DESCRIPTOR_HANDLE hgpu(descriptor);

	imgui_helper::PrepareImGui(Singleton<System>::getPtr()->getWindowHandle(), Singleton<Direct3D>::getPtr()->getDevice(), Singleton<Direct3D>::getPtr()->getFormat(), kBufferCount, hcpu, hgpu);

	SAFE_DELETE(vertexshader.data);
	SAFE_DELETE(pixelshader.data);

	return true;
}

SceneBase* Game::update()
{
	//ImGui更新
	updateImGui();


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

	//描画先セット
	d3d->getCommandList()->OMSetRenderTargets(1, &(D3D12_CPU_DESCRIPTOR_HANDLE)rtv, FALSE, &(D3D12_CPU_DESCRIPTOR_HANDLE)dsv);

	auto viewport = CD3DX12_VIEWPORT(0.0F, 0.0F, float(kScreenWidth), float(kScreenHeight));
	auto scissorrect = CD3DX12_RECT(0, 0, LONG(kScreenWidth), LONG(kScreenHeight));

	//ビューポートとシザーのセット
	d3d->getCommandList()->RSSetViewports(1, &viewport);
	d3d->getCommandList()->RSSetScissorRects(1, &scissorrect);

	ID3D12DescriptorHeap* heaps[] = { d3d->GetDescriptorManager()->GetHeap().Get() };
	d3d->getCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	auto cb = constantvuffers_[frameindex];
	void* mapped;
	cb->Map(0, nullptr, &mapped);
	memcpy(mapped, &sceneparam, sizeof(sceneparam));
	cb->Unmap(0, nullptr);

	D3D12_VERTEX_BUFFER_VIEW vbview[] = {
		model_.vbView,streamview_
	};

	d3d->getCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	d3d->getCommandList()->IASetVertexBuffers(0, _countof(vbview), vbview);
	d3d->getCommandList()->IASetIndexBuffer(&model_.ibView);

	d3d->getCommandList()->SetGraphicsRootSignature(rootsignature_.Get());
	d3d->getCommandList()->SetPipelineState(pipeline_.Get());
	d3d->getCommandList()->SetGraphicsRootConstantBufferView(0, cb->GetGPUVirtualAddress());

	d3d->getCommandList()->DrawIndexedInstanced(model_.indexCount, instancingdatacount_, 0, 0, 0);

	//ImGui描画
	renderImGui(d3d);

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

ComPtr<ID3D12Resource1> Game::createBufferResource(D3D12_HEAP_TYPE Type, UINT BufferSize, D3D12_RESOURCE_STATES State)
{
	ComPtr<ID3D12Resource1> ret;
	HRESULT hr;

	hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(Type),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(BufferSize),
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