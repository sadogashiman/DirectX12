#include "stdafx.h"
#include "Direct3D.h"
#include "error.h"
#include "Support.h"
#include "Singleton.h"
#include "System.h"

Direct3D::Direct3D()
{
	ZeroMemory(this, sizeof(Direct3D));
}

Direct3D::~Direct3D()
{
}

bool Direct3D::init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear, const wchar_t* MeshShaderFileName, const wchar_t* PixelShaderFileName)
{
	bool result;

	//パイプラインを作成
	result = loadPipeline(ScreenWidth, ScreenHeight, Vsync, FullScreen, ScreenDepth, ScreenNear);
	if (!result)
	{
		Error::showDialog("パイプラインの作成に失敗");
		return false;
	}

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



	return true;
}

bool Direct3D::render()
{
	return true;
}

void Direct3D::destroy()
{
}

bool Direct3D::loadAssets(const wchar_t* MeshShaderFileName, const wchar_t* PixelShaderFileName)
{




	return true;
}

bool Direct3D::loadPipeline(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear)
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
#endif // _DEBUG

	D3D_FEATURE_LEVEL featurelevel;
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC commandqueuedesc;
	DXGI_SWAP_CHAIN_DESC1 swapchaindesc;


	//機能レベルの設定
	featurelevel = D3D_FEATURE_LEVEL_12_1;

	//Direct3Dデバイスの作成
	hr = D3D12CreateDevice(
		NULL,
		featurelevel,
		__uuidof(ID3D12Device),
		(void**)device_.ReleaseAndGetAddressOf()
	);

	//ファクトリを作成
	ComPtr<IDXGIFactory4> factory;
	hr = CreateDXGIFactory2(dxgidebugflag, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		Error::showDialog("IDXGIFactory4の作成に失敗");
		return false;
	}

	//GPUが複数個存在することを前提に初期化
	if (kUseHardwareAdapter)
	{
		ComPtr<IDXGIAdapter1> hardwareadapter;
		Support::getHardwareAdapter(factory.Get(), &hardwareadapter);

		//Direct3Dデバイスを作成
		hr = D3D12CreateDevice(
			hardwareadapter.Get(),
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(&device_)
		);
		if (FAILED(hr))
		{
			//機能レベルを下げることを出力して機能レベルを再設定
			OutputDebugString("It does not correspond to the Feature level set by the GPU. Lower the Feature level and reset it");
			hr = D3D12CreateDevice(
				hardwareadapter.Get(),
				D3D_FEATURE_LEVEL_12_0,
				IID_PPV_ARGS(&device_));
			if (FAILED(hr))
			{
				Error::showDialog("Direct3Dデバイスの作成に失敗");
				return false;
			}
		}
	}
	else
	{
		ComPtr<IDXGIAdapter> warpadapter;
		//アダプタの列挙
		hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpadapter));
		if (FAILED(hr))
		{
			Error::showDialog("warpアダプタの列挙に失敗");
			return false;
		}

		//warpアダプタを使用するときは機能レベルを下げておく
		hr = D3D12CreateDevice(
			warpadapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device_)
		);
		if (FAILED(hr))
		{
			Error::showDialog("warpアダプタでのDirect3Dデバイスの作成に失敗");
			return false;
		}
	}

	//コマンドキューを初期化
	ZeroMemory(&commandqueuedesc, sizeof(commandqueuedesc));

	//コマンドキューの設定
	commandqueuedesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;          //コマンドキューの種類(DIRECT・GPUが実行できるコマンドバッファを指定)
	commandqueuedesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; //コマンドキューの優先度　(通常はNORMAL)
	commandqueuedesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;          //コマンドキューを作成するときの設定(通常はNONE)
	commandqueuedesc.NodeMask = 0;                                   //0を指定(一つのGPUのみを使用)

	//コマンドキューの作成
	hr = device_->CreateCommandQueue(&commandqueuedesc, IID_PPV_ARGS(&cmdqueue_));
	if (FAILED(hr))
	{
		Error::showDialog("コマンドキューの作成に失敗");
		return false;
	}

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
	swapchaindesc.SampleDesc.Quality = 0;
	swapchaindesc.Flags = 0;

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
	if (FAILED(hr))
	{
		Error::showDialog("スワップチェインの作成に失敗");
		return false;
	}

	//Alt+Enterを無効にする
	hr = factory->MakeWindowAssociation(Singleton<System>::getPtr()->getWindowHandle(), DXGI_MWA_NO_ALT_ENTER);
	if (FAILED(hr))
	{
		Error::showDialog("フルスクリーントランジションの設定に失敗");
		return false;
	}

	hr = swapchain.As(&swapchain_);
	if (FAILED(hr))
	{
		Error::showDialog("スワップチェインの複製に失敗");
		return false;
	}

	//バックバッファの数を取得
	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//descriptor heapの設定(RTV)
	D3D12_DESCRIPTOR_HEAP_DESC rtvheapdesc;
	ZeroMemory(&rtvheapdesc, sizeof(rtvheapdesc));
	rtvheapdesc.NumDescriptors = kBufferCount;
	rtvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //レンダーターゲットとして設定
	rtvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvheapdesc.NodeMask = 0;

	hr = device_->CreateDescriptorHeap(&rtvheapdesc, IID_PPV_ARGS(&rendertargetviewheap_));
	if (FAILED(hr))
	{
		Error::showDialog("rendertargetview Heapの作成に失敗");
		return false;
	}

	//サイズを取得
	rendertargetdescriptionsize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//descriptor heapの設定(DSV)
	D3D12_DESCRIPTOR_HEAP_DESC dsvheapdesc;
	ZeroMemory(&dsvheapdesc, sizeof(dsvheapdesc));
	dsvheapdesc.NumDescriptors = 1;
	dsvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device_->CreateDescriptorHeap(&dsvheapdesc, IID_PPV_ARGS(&depthstencilviewheap_));
	if (FAILED(hr))
	{
		Error::showDialog("depthstencilview heapの作成に失敗");
		return false;
	}

	//サイズを取得
	depthstencildescriptionsize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//フレームリソースを作成
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart());

	//フレームごとにコマンドアロケータとレンダーターゲットっビューを作成
	for (UINT i = 0; i < kBufferCount; i++)
	{
		hr = swapchain_->GetBuffer(i, IID_PPV_ARGS(&rendertargets[i]));
		if (FAILED(hr))
		{
			Error::showDialog("レンダーターゲットバッファの取得に失敗");
			return false;
		}

		device_->CreateRenderTargetView(rendertargets[i].Get(), nullptr, rtvhandle);
		rtvhandle.Offset(1, rendertargetdescriptionsize_);

		hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdallocator_[i]));
		if (FAILED(hr))
		{
			Error::showDialog("コマンドアロケータの作成に失敗");
			return false;
		}
	}

	//デプスステンシルビューの設定
	D3D12_DEPTH_STENCIL_VIEW_DESC depthstencildesc;
	ZeroMemory(&depthstencildesc, sizeof(depthstencildesc));
	depthstencildesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthstencildesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthstencildesc.Flags = D3D12_DSV_FLAG_NONE;

	//クリア値の設定
	D3D12_CLEAR_VALUE depthoptimizedclearvalue;
	depthoptimizedclearvalue.Format = DXGI_FORMAT_D32_FLOAT;
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
	if (FAILED(hr))
	{
		Error::showDialog("CommittedResourceの作成に失敗");
		return false;
	}

	NAME_D3D12_OBJECT(depthstencil_);

	//デプスステンシルの作成
	device_->CreateDepthStencilView(depthstencil_.Get(), &depthstencildesc, depthstencilviewheap_.Get()->GetCPUDescriptorHandleForHeapStart());

	//コンスタントバッファの作成
	const UINT constantbuffersize = sizeof(ConstantBuffer) * kBufferCount;
	hr = device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(constantbuffersize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantbuffer_)
	);
	if (FAILED(hr))
	{
		Error::showDialog("CommittedResourceの作成に失敗");
		return false;
	}

	//コンスタントバッファビューの設定
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc;
	ZeroMemory(&cbvdesc, sizeof(cbvdesc));
	cbvdesc.BufferLocation = constantbuffer_->GetGPUVirtualAddress();
	cbvdesc.SizeInBytes = constantbuffersize;

	//コンスタントバッファをマップして初期化
	CD3DX12_RANGE readrange(0, 0);
	hr = constantbuffer_->Map(0, &readrange, reinterpret_cast<void**>(&constantbufferviewbegin_));
	if (FAILED(hr))
	{
		Error::showDialog("コンスタントバッファのマップに失敗");
		return false;
	}

	return true;
}

bool Direct3D::populateCommandList()
{
	HRESULT hr;

	//コマンドリストの初期化
	//コマンドアロケータは関連付けされている場合のみ初期化可能
	hr = cmdallocator_[frameindex_]->Reset();
	if (FAILED(hr))
	{
		Error::showDialog("コマンドアロケータの初期化に失敗");
		return false;
	}

	hr = cmdlist_->Reset(cmdallocator_[kBufferCount].Get(), pipelinestate_.Get());
	if (FAILED(hr))
	{
		Error::showDialog("コマンドリストの初期化に失敗");
		return false;
	}

	//コマンドリストに必用な情報をセット
	cmdlist_->SetGraphicsRootSignature(rootsignature_.Get());
	cmdlist_->RSSetViewports(1, &viewport_);
	cmdlist_->RSSetScissorRects(1, &scissorrect_);

	//バックバッファをレンダーターゲットに設定
	cmdlist_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rendertargets[frameindex_].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart(), frameindex_, rendertargetdescriptionsize_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvhandle(depthstencilviewheap_->GetCPUDescriptorHandleForHeapStart());

	//コマンドを記録
	const float clearcolor[] {0.0F, 0.0F, 0.0F, 0.0F};
	cmdlist_->ClearRenderTargetView(rtvhandle, clearcolor, 0, nullptr);
	cmdlist_->ClearDepthStencilView(dsvhandle, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);

	cmdlist_->SetGraphicsRootConstantBufferView(0, constantbuffer_->GetGPUVirtualAddress() + sizeof(ConstantBuffer) * frameindex_);


	return true;
}
