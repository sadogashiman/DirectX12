#pragma once
#include "SwapChain.h"
const bool kUseHardwareAdapter = true;	//ハードウェアを使用するかどうか
const unsigned int kGpuWaitTimeout = (10 * 1000);//10s
const unsigned int kBufferCount = 2;
const float kClearColor[4] = { 0.0F,0.0F,1.0F,1.0F };
const DXGI_FORMAT kSurfaceFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
const int kMaxDescriptorCount = 2048; // SRV,CBV,UAV など.
const int kMaxDescriptorCountRTV = 100;
const int kMaxDescriptorCountDSV = 100;

class Direct3D
{
private:
	_declspec(align(256u))struct ConstantBuffer
	{
		XMMATRIX world;
		XMMATRIX worldview;
		XMMATRIX worldviewproj;
		uint32_t drawmeshlts;
	};

	//ID3D12
	ComPtr<ID3D12Device> device_;							//d3dデバイス
	ComPtr<ID3D12CommandQueue> cmdqueue_;					//コマンドリストの送信・実行の同期
	std::vector<ComPtr<ID3D12Resource1>> rendertargets_;		//バックバッファ
	std::vector<ComPtr<ID3D12CommandAllocator>> cmdallocator_;			//コマンドの割り当て
	ComPtr<ID3D12CommandAllocator> oneshotcmdallocator_;
	ComPtr<ID3D12CommandAllocator> bundlecmdallocator_;
	ComPtr<ID3D12GraphicsCommandList> cmdlist_;				//レンダリング用のコマンドリストのカプセル化
	ComPtr<ID3D12Resource1> depthbuffer_;

	//IDXGI
	CD3DX12_VIEWPORT viewport_;								//ビューポート
	CD3DX12_RECT scissorrect_;

	//同期関係
	std::vector<ComPtr<ID3D12Fence1>> fence_;				//CPUとGPUの同期に使用されるオブジェクトを表す
	HANDLE fenceevent_;
	unsigned int frameindex_;
	std::vector<unsigned int> framecounter_;
	std::vector<unsigned long long> fencevalue_;

	char videocarddescription_[128];
	int videocardmemory_;
	unsigned int bufferindex_;
	UINT8* constantbufferviewbegin_;
	unsigned int rendertargetdescriptionsize_;
	unsigned int depthstencildescriptionsize_;
	std::shared_ptr<SwapChain> swapchain_;
	std::shared_ptr<DescriptorManager> heaprtv_;
	std::shared_ptr<DescriptorManager> heapdsv_;
	std::shared_ptr<DescriptorManager> heap_;
	DescriptorHandle defaultdepthdsv_;

	UINT width_;
	UINT height_;
	HANDLE waitfence_;

	bool isallowtearing_;
	void waitPrevFrame();
	void waitForIdleGPU();

	//create
	void createCommandAllocators();
	void createFrameFences();
	void createDefaultDepthBuffer(const int Width, const int Height);
	void prepareDescriptorHeaps();

public:
	Direct3D();
	~Direct3D();
	void init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear,HWND Hwnd);
	void render();
	void destroy();
	void finishCommandList(ComPtr<ID3D12GraphicsCommandList>& Command);


	//window関係
	void ToggleFullscreen();
	void OnSizeChanged(UINT width, UINT height, bool isMinimized);



	//create
	ComPtr<ID3D12Resource1> createResource(const CD3DX12_RESOURCE_DESC& Desc,D3D12_RESOURCE_STATES ResourceStates,const D3D12_CLEAR_VALUE* ClearValue,D3D12_HEAP_TYPE HeapType);
	std::vector<ComPtr<ID3D12Resource1>> createConstantBuffers(const CD3DX12_RESOURCE_DESC& Desc,int Count = kBufferCount);
	ComPtr<ID3D12GraphicsCommandList> CreateBundleCommandList();

	void writeToUploadHeapMemory(ID3D12Resource1* Resource, uint32_t Size, const void* Data);

	
	//get
	inline std::shared_ptr<SwapChain> getSwapChain() { return swapchain_; }
	inline ID3D12Device* getDevice()const { return device_.Get(); }
	inline ID3D12CommandQueue* getCommandQueue()const { return cmdqueue_.Get(); }
	inline ComPtr<ID3D12GraphicsCommandList> getCommandList(){ return cmdlist_; }
	inline CD3DX12_VIEWPORT getViewport()const { return viewport_; }
	inline CD3DX12_RECT getScissorRects()const { return scissorrect_; }
	inline HANDLE getFenceEventHandle()const { return fenceevent_; }
	inline std::vector<ComPtr<ID3D12Fence1>> getFence() { return fence_; }
	inline std::vector<unsigned long long >getFenceValue() { return fencevalue_; }
	inline std::shared_ptr<DescriptorManager> GetDescriptorManager() { return heap_; }



};

