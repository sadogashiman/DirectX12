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

void Direct3D::init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear, HWND Hwnd)
{
	UINT dxgidebugflag = 0;

#ifdef TRUE
	//デバッグ時のみデバッグレイヤーを有効化する
	//デバイスの作成後に有効にすると意味がないのでデバイスの作成前に設定する
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


	//ファクトリを作成
	ComPtr<IDXGIFactory5> factory;
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

	//各ディスクリプターヒープの作成
	prepareDescriptorHeaps();

	width_ = kWindow_Width;
	height_ = kWindow_Height;

	//HDRを使用するかの設定
	auto format = kSurfaceFormat;
	bool useHDR = format == DXGI_FORMAT_R16G16B16A16_FLOAT || format == DXGI_FORMAT_R10G10B10A2_UNORM;
	if (useHDR)
	{
		bool isDisplayHDR10 = false;
		UINT index = 0;
		ComPtr<IDXGIOutput> current;
		while (adapter->EnumOutputs(index, &current) != DXGI_ERROR_NOT_FOUND)
		{
			ComPtr<IDXGIOutput6> output6;
			current.As(&output6);

			DXGI_OUTPUT_DESC1 desc;
			output6->GetDesc1(&desc);
			isDisplayHDR10 |= desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
			++index;
		}

		if (!isDisplayHDR10)
		{
			format = DXGI_FORMAT_R8G8B8A8_UNORM;
			useHDR = false;
		}
	}

	BOOL allowTearing = FALSE;
	hr = factory->CheckFeatureSupport(
		DXGI_FEATURE_PRESENT_ALLOW_TEARING,
		&allowTearing,
		sizeof(allowTearing)
	);
	isallowtearing_ = SUCCEEDED(hr) && allowTearing;

	//スワップチェインを初期化
	DXGI_SWAP_CHAIN_DESC1 swapchaindesc;
	ZeroMemory(&swapchaindesc, sizeof(swapchaindesc));

	//ウィンドウモードの設定
	swapchaindesc.BufferCount = kBufferCount;
	swapchaindesc.Width = width_;
	swapchaindesc.Height = height_;
	swapchaindesc.Format = format;
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
	swapchaindesc.SampleDesc.Count = 1;

	//フルスクリーンの設定
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsdesc;
	ZeroMemory(&fsdesc, sizeof(fsdesc));

	fsdesc.Windowed = FullScreen ? TRUE : FALSE;
	fsdesc.RefreshRate.Denominator = 1000;
	fsdesc.RefreshRate.Numerator = 60317;
	fsdesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	fsdesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;

	//スワップチェインを作成
	ComPtr<IDXGISwapChain1> swapchain;
	hr = factory->CreateSwapChainForHwnd(
		cmdqueue_.Get(),
		Hwnd,
		&swapchaindesc,
		&fsdesc,
		nullptr,
		&swapchain
	);
	ThrowIfFailed(hr);
	swapchain_ = std::make_shared<SwapChain>(swapchain, heaprtv_);
	surfaceformat_ = swapchain_->getFormat();

	//Alt+Enterを無効にする
	hr = factory->MakeWindowAssociation(Hwnd, DXGI_MWA_NO_ALT_ENTER);
	ThrowIfFailed(hr);

	//デプスバッファ関連の作成
	createDefaultDepthBuffer(width_, height_);

	//コマンドアロケーターの準備
	createCommandAllocators();

	//コマンドリストの作成
	hr = device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdallocator_[0].Get(),
		nullptr,
		IID_PPV_ARGS(&cmdlist_)
	);
	ThrowIfFailed(hr);
	cmdlist_->Close();

	//ビューポートの設定
	viewport_.Height = static_cast<float>(height_);
	viewport_.Width = static_cast<float>(width_);
	viewport_.MaxDepth = 1.0F;
	viewport_.MinDepth = 0.0F;
	viewport_.TopLeftX = 0.0F;
	viewport_.TopLeftY = 0.0F;

	scissorrect_.top = 0;
	scissorrect_.left = 0;
	scissorrect_.right = static_cast<LONG>(width_);
	scissorrect_.bottom = static_cast<LONG>(height_);

	//デフォルトの背景色を決定
	clearcolor[0] = kClearColor[0];
	clearcolor[1] = kClearColor[1];
	clearcolor[2] = kClearColor[2];
	clearcolor[3] = kClearColor[3];
}

void Direct3D::render()
{
	frameindex_ = swapchain_->getCurrentBackBufferIndex();

	cmdallocator_[frameindex_].Get()->Reset();
	cmdlist_->Reset(cmdallocator_[frameindex_].Get(), nullptr);

	//スワップチェイン表示可能からレンダーターゲット描画可能へ
	auto barriertoRT = swapchain_->getBarrierToRenderTarget();
	cmdlist_->ResourceBarrier(1, &barriertoRT);

	auto rtv = swapchain_->getCurrentRTV();
	auto dsv = defaultdepthdsv_;

	//レンダーターゲットクリア
	cmdlist_->ClearRenderTargetView(rtv, clearcolor, 0, nullptr);

	//深度バッファクリア
	cmdlist_->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.0F,0,0,nullptr);

	//描画先をセット
	cmdlist_->OMSetRenderTargets(1, &(D3D12_CPU_DESCRIPTOR_HANDLE)rtv, FALSE, &(D3D12_CPU_DESCRIPTOR_HANDLE)dsv);

	ID3D12DescriptorHeap* heaps[] = { heap_->GetHeap().Get() };
	cmdlist_->SetDescriptorHeaps(_countof(heaps), heaps);

	//レンダーターゲットからスワップチェイン表示可能へ
	auto barriertoPresent = swapchain_->getBarrierToPresent();
	cmdlist_->ResourceBarrier(1, &barriertoPresent);

	cmdlist_->Close();

	ID3D12CommandList* lists[] = { cmdlist_.Get() };
	cmdqueue_->ExecuteCommandLists(1, lists);

	swapchain_->present(1, 0);

	swapchain_->waitPreviousFrame(cmdqueue_, frameindex_, kGpuWaitTimeout);
}

void Direct3D::waitPrevFrame()
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

void Direct3D::waitForIdleGPU()
{
	// 全ての発行済みコマンドの終了を待つ.
	ComPtr<ID3D12Fence1> fence;
	const UINT64 expectValue = 1;
	HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	ThrowIfFailed(hr);

	cmdqueue_->Signal(fence.Get(), expectValue);
	if (fence->GetCompletedValue() != expectValue)
	{
		fence->SetEventOnCompletion(expectValue, waitfence_);
		WaitForSingleObject(waitfence_, INFINITE);
	}
}


void Direct3D::destroy()
{
	CloseHandle(fenceevent_);
}

void Direct3D::finishCommandList(ComPtr<ID3D12GraphicsCommandList>& Command)
{
	ID3D12CommandList* commandList[] = {
  Command.Get()
	};
	Command->Close();
	cmdqueue_->ExecuteCommandLists(1, commandList);
	HRESULT hr;
	ComPtr<ID3D12Fence1> fence;
	hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	ThrowIfFailed(hr);
	const UINT64 expectValue = 1;
	cmdqueue_->Signal(fence.Get(), expectValue);
	do
	{
	} while (fence->GetCompletedValue() != expectValue);
	oneshotcmdallocator_->Reset();
}

void Direct3D::ToggleFullscreen()
{
	if (swapchain_->isFullScreen())
	{
		// FullScreen -> Windowed
		swapchain_->setFullScreen(false);
		SetWindowLong(Singleton<System>::getPtr()->getWindowHandle(), GWL_STYLE, WS_OVERLAPPEDWINDOW);
		ShowWindow(Singleton<System>::getPtr()->getWindowHandle(), SW_NORMAL);
	}
	else
	{
		// Windowed -> FullScreen
		DXGI_MODE_DESC desc;
		desc.Format = kSurfaceFormat;
		desc.Width = kScreenWidth;
		desc.Height = kScreenHeight;
		desc.RefreshRate.Denominator = 1;
		desc.RefreshRate.Numerator = 60;
		desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapchain_->resizeTarget(&desc);
		swapchain_->setFullScreen(true);
	}
	OnSizeChanged(kScreenWidth, kScreenHeight, false);
}

void Direct3D::OnSizeChanged(UINT width, UINT height, bool isMinimized)
{
	width_ = width;
	height_ = height;
	if (!swapchain_ || isMinimized)
		return;

	// 処理の完了を待ってからサイズ変更の処理を開始.
	waitForIdleGPU();
	swapchain_->resizeBuffers(width, height);

	// デプスバッファの作り直し.
	depthbuffer_.Reset();
	heapdsv_->Free(defaultdepthdsv_);
	createDefaultDepthBuffer(width_, height_);

	frameindex_= swapchain_->getCurrentBackBufferIndex();

	viewport_.Width = float(width_);
	viewport_.Height = float(height_);
	scissorrect_.right = width_;
	scissorrect_.bottom = height_;
}

ComPtr<ID3D12Resource1> Direct3D::createResource(const CD3DX12_RESOURCE_DESC& Desc, D3D12_RESOURCE_STATES ResourceStates, const D3D12_CLEAR_VALUE* ClearValue, D3D12_HEAP_TYPE HeapType)
{
	HRESULT hr;
	ComPtr<ID3D12Resource1> ret;
	hr = device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(HeapType),
		D3D12_HEAP_FLAG_NONE,
		&Desc,
		ResourceStates,
		ClearValue,
		IID_PPV_ARGS(&ret)
	);
	ThrowIfFailed(hr);
	return ret;
}

std::vector<ComPtr<ID3D12Resource1>> Direct3D::createConstantBuffers(const CD3DX12_RESOURCE_DESC& Desc, int Count)
{
	std::vector<ComPtr<ID3D12Resource1>> buffers;
	for (int i = 0; i < Count; ++i)
	{
		buffers.emplace_back(
			createResource(Desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, D3D12_HEAP_TYPE_UPLOAD)
		);
	}
	return buffers;
}

ComPtr<ID3D12GraphicsCommandList> Direct3D::CreateBundleCommandList()
{
	ComPtr<ID3D12GraphicsCommandList> cmd;
	device_->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_BUNDLE,
		bundlecmdallocator_.Get(),
		nullptr, IID_PPV_ARGS(&cmd)
	);
	return cmd;
}

void Direct3D::writeToUploadHeapMemory(ID3D12Resource1* Resource, uint32_t Size, const void* Data)
{
	void* mapped;
	HRESULT hr = Resource->Map(0, nullptr, &mapped);
	if (SUCCEEDED(hr))
	{
		memcpy(mapped, Data, Size);
		Resource->Unmap(0, nullptr);
	}
	ThrowIfFailed(hr);
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

void Direct3D::prepareDescriptorHeaps()
{
	// RTV のディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDescRTV{
	  D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	  kMaxDescriptorCountRTV,
	  D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	  0
	};
	heaprtv_ = std::make_shared<DescriptorManager>(device_, heapDescRTV);

	// DSV のディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDescDSV{
	  D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
	  kMaxDescriptorCountDSV,
	  D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
	  0
	};
	heapdsv_ = std::make_shared<DescriptorManager>(device_, heapDescDSV);

	// SRV のディスクリプタヒープ
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc{
	  D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
	  kMaxDescriptorCount,
	  D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
	  0
	};
	heap_ = std::make_shared<DescriptorManager>(device_, heapDesc);
}

void Direct3D::createDefaultDepthBuffer(const int Width, const int Height)
{
	// デプスバッファの生成
	auto depthbufferdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		Width,
		Height,
		1, 0,
		1, 0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);
	D3D12_CLEAR_VALUE depthclearvalue{};
	depthclearvalue.Format = depthbufferdesc.Format;
	depthclearvalue.DepthStencil.Depth = 1.0f;
	depthclearvalue.DepthStencil.Stencil = 0;

	HRESULT hr;
	hr = device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthbufferdesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthclearvalue,
		IID_PPV_ARGS(&depthbuffer_)
	);
	ThrowIfFailed(hr);

	// デプスステンシルビュー生成
	defaultdepthdsv_ = heapdsv_->Alloc();
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;

	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Texture2D.MipSlice = 0;

	device_->CreateDepthStencilView(depthbuffer_.Get(), &dsvDesc, defaultdepthdsv_);

}
