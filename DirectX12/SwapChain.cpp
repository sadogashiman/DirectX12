#include "stdafx.h"
#include "SwapChain.h"


SwapChain::SwapChain(ComPtr<IDXGISwapChain1> Swapchain, std::shared_ptr<DescriptorManager>& HeapRTV, bool HDR)
{
	//メンバに保存
	Swapchain.As(&swapchain_);
	swapchain_->GetDesc1(&desc_);

	ComPtr<ID3D12Device> device;
	Swapchain->GetDevice(IID_PPV_ARGS(&device));

	//配列サイズ更新
	images_.resize(desc_.BufferCount);
	imageRTV_.resize(desc_.BufferCount);
	fences_.resize(desc_.BufferCount);
	fencevalues_.resize(desc_.BufferCount);
	waithandle_ = CreateEvent(NULL, FALSE, FALSE, NULL);

	HRESULT hr;
	for (UINT i = 0; i < desc_.BufferCount; ++i)
	{
		hr = device->CreateFence(
			0, D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&fences_[i]));
		ThrowIfFailed(hr);

	   imageRTV_[i] = HeapRTV->Alloc();

		// Swapchain イメージの RTV 生成.
		hr = swapchain_->GetBuffer(i, IID_PPV_ARGS(&images_[i]));
		ThrowIfFailed(hr);
		device->CreateRenderTargetView(images_[i].Get(), nullptr, imageRTV_[i]);
	}

	// フォーマットに応じてカラースペースを設定.
	DXGI_COLOR_SPACE_TYPE colorspace;
	switch (desc_.Format)
	{
	default:
		colorspace = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		break;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		colorspace = DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709;
		break;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		colorspace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
		break;
	}
	swapchain_->SetColorSpace1(colorspace);

	if (HDR)
	{
		setMetaData();
	}
}

SwapChain::~SwapChain()
{
	BOOL isfullscreen;
	swapchain_->GetFullscreenState(&isfullscreen, nullptr);
	if (isfullscreen)
	{
		swapchain_->SetFullscreenState(FALSE, nullptr);
	}
	CloseHandle(waithandle_);
}

bool SwapChain::isFullScreen() const
{
	BOOL fullscreen;
	if (FAILED(swapchain_->GetFullscreenState(&fullscreen, nullptr)))
	{
		fullscreen = FALSE;
	}
	return fullscreen == TRUE;
}

void SwapChain::setFullScreen(bool FullScreen)
{
	if (FullScreen)
	{
		ComPtr<IDXGIOutput> output;
		swapchain_->GetContainingOutput(&output);
		if (output)
		{
			DXGI_OUTPUT_DESC desc{};
			output->GetDesc(&desc);
		}

		swapchain_->SetFullscreenState(TRUE,nullptr);
	}
	else
	{
		swapchain_->SetFullscreenState(FALSE, nullptr);
	}

}

void SwapChain::waitPreviousFrame(ComPtr<ID3D12CommandQueue> CommandQueue, int FrameIndex, DWORD GpuTimeOut)
{
	auto fence = fences_[FrameIndex];
	// 現在のフェンスに GPU が到達後設定される値をセット.
	auto value = ++fencevalues_[FrameIndex];
	CommandQueue->Signal(fence.Get(), value);

	// 次フレームで処理するコマンドの実行完了を待機する.
	auto nextindex = getCurrentBackBufferIndex();
	auto finishvalue = fencevalues_[nextindex];
	fence = fences_[nextindex];
	value = fence->GetCompletedValue();
	if (value < finishvalue)
	{
		// 未完了のためイベントで待機.
		fence->SetEventOnCompletion(finishvalue, waithandle_);
		WaitForSingleObject(waithandle_, GpuTimeOut);
	}
}

void SwapChain::resizeBuffers(UINT Width, UINT Height)
{
	// リサイズのためにいったん解放.
	for (auto& v : images_) {
		v.Reset();
	}
	HRESULT hr = swapchain_->ResizeBuffers(
		desc_.BufferCount,
		Width, Height, desc_.Format, desc_.Flags
	);
	ThrowIfFailed(hr);

	// イメージを取り直して RTV を再生成.
	ComPtr<ID3D12Device> device;
	swapchain_->GetDevice(IID_PPV_ARGS(&device));
	for (UINT i = 0; i < desc_.BufferCount; ++i) {
		swapchain_->GetBuffer(i, IID_PPV_ARGS(&images_[i]));
		device->CreateRenderTargetView(
			images_[i].Get(),
			nullptr,
			imageRTV_[i]);
	}
}

void SwapChain::setMetaData()
{
	struct DisplayChromacities
	{
		float RedX;
		float RedY;
		float GreenX;
		float GreenY;
		float BlueX;
		float BlueY;
		float WhiteX;
		float WhiteY;
	};
	DisplayChromacities displaychromacitylist[] = {
	  { 0.64000f, 0.33000f, 0.30000f, 0.60000f, 0.15000f, 0.06000f, 0.31270f, 0.32900f }, // Rec709 
	  { 0.70800f, 0.29200f, 0.17000f, 0.79700f, 0.13100f, 0.04600f, 0.31270f, 0.32900f }, // Rec2020
	};

	int useindex = 0;
	if (desc_.Format == DXGI_FORMAT_R16G16B16A16_FLOAT)
	{
		useindex = 1;
	}

	const auto& chroma = displaychromacitylist[useindex];
	DXGI_HDR_METADATA_HDR10 HDR10metadata{};
	HDR10metadata.RedPrimary[0] = UINT16(chroma.RedX * 50000.0f);
	HDR10metadata.RedPrimary[1] = UINT16(chroma.RedY * 50000.0f);
	HDR10metadata.GreenPrimary[0] = UINT16(chroma.GreenX * 50000.0f);
	HDR10metadata.GreenPrimary[1] = UINT16(chroma.GreenY * 50000.0f);
	HDR10metadata.BluePrimary[0] = UINT16(chroma.BlueX * 50000.0f);
	HDR10metadata.BluePrimary[1] = UINT16(chroma.BlueY * 50000.0f);
	HDR10metadata.WhitePoint[0] = UINT16(chroma.WhiteX * 50000.0f);
	HDR10metadata.WhitePoint[1] = UINT16(chroma.WhiteY * 50000.0f);
	HDR10metadata.MaxMasteringLuminance = UINT(1000.0f * 10000.0f);
	HDR10metadata.MinMasteringLuminance = UINT(0.001f * 10000.0f);
	HDR10metadata.MaxContentLightLevel = UINT16(2000.0f);
	HDR10metadata.MaxFrameAverageLightLevel = UINT16(500.0f);
	swapchain_->SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, sizeof(HDR10metadata), &HDR10metadata);
}