#pragma once
#include "DescriptorManager.h"
#include "D3D12BookUtil.h"
class SwapChain
{
private:
	ComPtr<IDXGISwapChain4> swapchain_;
	std::vector<ComPtr<ID3D12Resource1>> images_;
	std::vector<DescriptorHandle> imageRTV_;
	std::vector<UINT64> fencevalues_;
	std::vector<ComPtr<ID3D12Fence1>> fences_;
	DXGI_SWAP_CHAIN_DESC1 desc_;
	HANDLE waithandle_;

	void setMetaData();
public:
	SwapChain(
		ComPtr<IDXGISwapChain1> Swapchain,
		std::shared_ptr<DescriptorManager>& HeapRTV,
		bool HDR = false
	);
	~SwapChain();
	bool isFullScreen() const;
	void setFullScreen(bool FullScreen);
	inline void resizeTarget(const DXGI_MODE_DESC* NewTargetParameters) { swapchain_->ResizeTarget(NewTargetParameters); }

	void waitPreviousFrame(ComPtr<ID3D12CommandQueue> CommandQueue, int FrameIndex, DWORD GpuTimeOut);
	void resizeBuffers(UINT Width, UINT Height);

	//get
	inline UINT getCurrentBackBufferIndex()const { return swapchain_->GetCurrentBackBufferIndex(); }
	inline HRESULT present(UINT SyncInterval, UINT Flag) { return swapchain_->Present(SyncInterval, Flag); }
	inline DescriptorHandle getCurrentRTV()const { return imageRTV_[getCurrentBackBufferIndex()]; }
	inline ComPtr<ID3D12Resource1> getImage(UINT Index) { return images_[Index]; }
	inline DXGI_FORMAT getFormat()const { return desc_.Format; }
	inline CD3DX12_RESOURCE_BARRIER getBarrierToRenderTarget(){return CD3DX12_RESOURCE_BARRIER::Transition(images_[getCurrentBackBufferIndex()].Get(),D3D12_RESOURCE_STATE_PRESENT,D3D12_RESOURCE_STATE_RENDER_TARGET);}
	inline CD3DX12_RESOURCE_BARRIER getBarrierToPresent(){return CD3DX12_RESOURCE_BARRIER::Transition(images_[getCurrentBackBufferIndex()].Get(),D3D12_RESOURCE_STATE_RENDER_TARGET,D3D12_RESOURCE_STATE_PRESENT);}

};

