#include "stdafx.h"
#include "Direct3D.h"
#include "error.h"
#include "Singleton.h"
#include "System.h"
#include "DXHelper.h"

Direct3D::Direct3D()
{
	rendertargets_.resize(kBufferCount);
	fencevalue_.resize(kBufferCount);
	frameindex_ = 0;
	fenceevent_ = CreateEvent(NULL, FALSE, FALSE, NULL);
}

Direct3D::~Direct3D()
{
}

void Direct3D::init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear, const wchar_t* MeshShaderFileName, const wchar_t* PixelShaderFileName)
{
	//パイプラインを作成
	loadPipeline(ScreenWidth, ScreenHeight, Vsync, FullScreen, ScreenDepth, ScreenNear);


	//ビューポートの設定
	viewport_.Height = kWindow_Height;
	viewport_.Width = kWindow_Width;
	viewport_.MaxDepth = kScreen_depth;
	viewport_.MinDepth = 0.0F;
	viewport_.TopLeftX = 0.0F;
	viewport_.TopLeftY = 0.0F;

	scissorrect_.top = 0;
	scissorrect_.left = 0;
	scissorrect_.right = static_cast<LONG>(kWindow_Width);
	scissorrect_.bottom = static_cast<LONG>(kWindow_Height);

}

void Direct3D::update()
{
}

void Direct3D::render()
{
	HRESULT hr;

	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//コマンドリセット
	cmdallocator_[frameindex_]->Reset();
	cmdlist_->Reset(cmdallocator_[frameindex_].Get(),nullptr);

	//スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barrirtorendertarget = CD3DX12_RESOURCE_BARRIER::Transition(
		rendertargets_[frameindex_].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdlist_->ResourceBarrier(1, &barrirtorendertarget);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart(),
		frameindex_,
		rendertargetdescriptionsize_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(
		depthstencilviewheap_->GetCPUDescriptorHandleForHeapStart());

	//カラーバッファ（レンダーターゲットビューのクリア)
	cmdlist_->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

	//深度バッファのクリア
	cmdlist_->ClearDepthStencilView(
		dsv,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0F,
		0,
		0,
		nullptr
	);

	//描画先をセット
	cmdlist_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	auto barrirtopresent = CD3DX12_RESOURCE_BARRIER::Transition(
		rendertargets_[frameindex_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	cmdlist_->ResourceBarrier(1, &barrirtopresent);
	cmdlist_->Close();

	ID3D12CommandList* lists[] = { cmdlist_.Get() };
	cmdqueue_->ExecuteCommandLists(1, lists);

	swapchain_->Present(1, 0);
	
	waiPrevFrame();
}

void Direct3D::destroy()
{
	CloseHandle(fenceevent_);
}

void Direct3D::loadPipeline(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear)
{
#ifdef _DEBUG
	//デバッグ時のみデバッグレイヤーを有効化する
	//デバイスの作成後に有効にすると意味がないのでデバイスの作成前に設定する
	UINT dxgidebugflag = 0;
	ComPtr<ID3D12Debug>debugcontroller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugcontroller.ReleaseAndGetAddressOf()))))
	{
		//デバッグレイヤーを有効にする
		debugcontroller->EnableDebugLayer();

		//追加のデバッグレイヤーを有効にする
		dxgidebugflag |= DXGI_CREATE_FACTORY_DEBUG;
	}

	//GBVの有効化
	ComPtr<ID3D12Debug3>gbvdebug;
	debugcontroller.As(&gbvdebug);
	gbvdebug->SetEnableGPUBasedValidation(true);
#endif // _DEBUG

	D3D_FEATURE_LEVEL featurelevel = D3D_FEATURE_LEVEL_12_1;
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC commandqueuedesc;
	DXGI_SWAP_CHAIN_DESC1 swapchaindesc;


	//ファクトリを作成
	ComPtr<IDXGIFactory3> factory;
	hr = CreateDXGIFactory2(dxgidebugflag, IID_PPV_ARGS(&factory));
	ThrowIfFailed(hr);

	//ハードウェアアダプタの検索
	ComPtr<IDXGIAdapter1> useadapter;
	ComPtr<IDXGIAdapter1>adapter;
	unsigned int adapterindex = 0;
	while (DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterindex, &adapter))
	{
		DXGI_ADAPTER_DESC1 desc1{};
		adapter->GetDesc1(&desc1);
		++adapterindex;
		if (desc1.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)
			continue;

		hr = D3D12CreateDevice(
			adapter.Get(),
			featurelevel,
			__uuidof(ID3D12Device),
			nullptr
		);
		if (SUCCEEDED(hr))
			break;
	}

	//使用するアダプタ
	adapter.As(&useadapter);

	//D3Dデバイスを作成
	hr = D3D12CreateDevice(
		useadapter.Get(),
		featurelevel,
		IID_PPV_ARGS(&device_)
	);
	ThrowIfFailed(hr);


	//コマンドキューを初期化
	ZeroMemory(&commandqueuedesc, sizeof(commandqueuedesc));

	//コマンドキューの設定
	commandqueuedesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;          //コマンドキューの種類(DIRECT・GPUが実行できるコマンドバッファを指定)
	commandqueuedesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; //コマンドキューの優先度　(通常はNORMAL)
	commandqueuedesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;          //コマンドキューを作成するときの設定(通常はNONE)
	commandqueuedesc.NodeMask = 0;                                   //0を指定(一つのGPUのみを使用)

	//コマンドキューの作成
	hr = device_->CreateCommandQueue(&commandqueuedesc, IID_PPV_ARGS(&cmdqueue_));
	ThrowIfFailed(hr);

	//スワップチェインを初期化
	ZeroMemory(&swapchaindesc, sizeof(swapchaindesc));

	//スワップチェインの設定
	swapchaindesc.BufferCount = kBufferCount;
	swapchaindesc.Width = kWindow_Width;
	swapchaindesc.Height = kWindow_Height;
	swapchaindesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchaindesc.SampleDesc.Count = 1;

	//スワップチェインを作成
	ComPtr<IDXGISwapChain1> swapchain;
	hr = factory->CreateSwapChainForHwnd(
		cmdqueue_.Get(),
		Singleton<System>::getPtr()->getWindowHandle(),
		&swapchaindesc,
		nullptr,
		nullptr,
		&swapchain
	);
	ThrowIfFailed(hr);

	//Alt+Enterを無効にする
	hr = factory->MakeWindowAssociation(Singleton<System>::getPtr()->getWindowHandle(), DXGI_MWA_NO_ALT_ENTER);
	ThrowIfFailed(hr);

	hr = swapchain.As(&swapchain_); //IDXGISwapChain4取得
	ThrowIfFailed(hr);

	//バックバッファの数を取得
	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//各ディスクリプターヒープの作成
	createDescriptorHeaps();

	//レンダーターゲットビューの作成
	prepareRenderTargetView();

	//デプスバッファ関連の作成
	createDepthBuffer();

	//コマンドアロケーターの準備
	createCommandAllocators();

	//描画フレーム同期用フェンス生成
	createFrameFences();

	//コマンドリストの初期化
	hr = device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdallocator_[0].Get(),
		nullptr,
		IID_PPV_ARGS(&cmdlist_)
	);

	cmdlist_->Close();
}

void Direct3D::waiPrevFrame()
{
	auto& fence = fence_[frameindex_];
	const auto currentvalue = ++fencevalue_[frameindex_];
	cmdqueue_->Signal(fence.Get(), currentvalue);

	//次処理するコマンド(アロケーター)のものは実行完了済みかをついになっているフェンスで確認
	auto nextindex = (frameindex_ + 1) % kBufferCount;
	const auto finishexpect = fencevalue_[nextindex];
	const auto nextfencevalue = fence_[nextindex]->GetCompletedValue();
	if (nextfencevalue < finishexpect)
	{
		fence_[nextindex]->SetEventOnCompletion(finishexpect, fenceevent_);
		WaitForSingleObject(fenceevent_, kGpuWaitTimeout);
	}
}


void Direct3D::createCommandAllocators()
{
	HRESULT hr;
	//配列サイズ変更
	cmdallocator_.resize(kBufferCount);

	//フレームごとにコマンドアロケータとレンダーターゲットっビューを作成
	for (UINT i = 0; i < kBufferCount; i++)
	{
		hr = device_->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&cmdallocator_[i]));
		ThrowIfFailed(hr);
	}
}

void Direct3D::createFrameFences()
{
	HRESULT hr;

	//配列サイズ変更
	fence_.resize(kBufferCount);

	for (int i = 0; i < kBufferCount; i++)
	{
		hr = device_->CreateFence(
			0,
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&fence_[i])
		);
		ThrowIfFailed(hr);
	}
}

void Direct3D::createDescriptorHeaps()
{
	HRESULT hr;
	//descriptor heapの設定(RTV)
	D3D12_DESCRIPTOR_HEAP_DESC rtvheapdesc;
	ZeroMemory(&rtvheapdesc, sizeof(rtvheapdesc));
	rtvheapdesc.NumDescriptors = kBufferCount;
	rtvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //レンダーターゲットとして設定
	rtvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvheapdesc.NodeMask = 0;

	hr = device_->CreateDescriptorHeap(&rtvheapdesc, IID_PPV_ARGS(&rendertargetviewheap_));
	ThrowIfFailed(hr);

	//サイズを取得
	rendertargetdescriptionsize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//descriptor heapの設定(DSV)
	D3D12_DESCRIPTOR_HEAP_DESC dsvheapdesc;
	ZeroMemory(&dsvheapdesc, sizeof(dsvheapdesc));
	dsvheapdesc.NumDescriptors = 1;
	dsvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device_->CreateDescriptorHeap(&dsvheapdesc, IID_PPV_ARGS(&depthstencilviewheap_));
	ThrowIfFailed(hr);

	//サイズを取得
	depthstencildescriptionsize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}

void Direct3D::prepareRenderTargetView()
{
	HRESULT hr;
	//フレームリソースを作成
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < kBufferCount; ++i)
	{
		hr = swapchain_->GetBuffer(i, IID_PPV_ARGS(&rendertargets_[i]));
		ThrowIfFailed(hr);

		device_->CreateRenderTargetView(rendertargets_[i].Get(), nullptr, rtvhandle);
		rtvhandle.Offset(1, rendertargetdescriptionsize_);
	}
}

void Direct3D::createDepthBuffer()
{
	HRESULT hr;

	auto depthbufferdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		kWindow_Width,
		kWindow_Height,
		1,
		0,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	//クリア値の設定
	D3D12_CLEAR_VALUE depthoptimizedclearvalue;
	depthoptimizedclearvalue.Format = depthbufferdesc.Format;
	depthoptimizedclearvalue.DepthStencil.Depth = 1.0F;
	depthoptimizedclearvalue.DepthStencil.Stencil = 0;

	hr = device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, kWindow_Width, kWindow_Height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthoptimizedclearvalue,
		IID_PPV_ARGS(&depthstencil_)
	);
	ThrowIfFailed(hr);

	//デプスステンシルビュー作成
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc
	{
		DXGI_FORMAT_D32_FLOAT,
		D3D12_DSV_DIMENSION_TEXTURE2D,
		D3D12_DSV_FLAG_NONE,
		{
			0
		}
	};
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvhandle(depthstencilviewheap_->GetCPUDescriptorHandleForHeapStart());

	//デプスステンシルの作成
	device_->CreateDepthStencilView(depthstencil_.Get(), &dsvdesc, depthstencilviewheap_.Get()->GetCPUDescriptorHandleForHeapStart());
}
