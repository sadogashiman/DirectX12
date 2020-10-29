#pragma once
#include "Model.h"
const bool kUseHardwareAdapter = true;	//ハードウェアを使用するかどうか
const unsigned int kGpuWaitTimeout = (10 * 1000);//10s
const unsigned int kBufferCount = 2U;
const float kClearColor[4] = { 0.0F,0.0F,0.0F,1.0F };

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
	ComPtr<ID3D12DescriptorHeap> rendertargetviewheap_;		//記述子ヒープ
	ComPtr<ID3D12DescriptorHeap> depthstencilviewheap_;		//デプスステンシルヒープ
	std::vector<ComPtr<ID3D12Resource1>> rendertargets_;		//バックバッファ
	std::vector<ComPtr<ID3D12CommandAllocator>> cmdallocator_;			//コマンドの割り当て
	ComPtr<ID3D12GraphicsCommandList> cmdlist_;				//レンダリング用のコマンドリストのカプセル化
	ComPtr<ID3D12PipelineState> pipelinestate_;				//グラフィックスパイプラインのステータス
	ComPtr<ID3D12Resource> depthstencil_;					//デプスステンシル
	ComPtr<ID3D12Resource> constantbuffer_;					//コンスタントバッファ
	ComPtr<ID3D12RootSignature> rootsignature_;				//ルートシグネチャ

	//IDXGI
	ComPtr<IDXGISwapChain4> swapchain_;						//スワップチェイン
	ComPtr<IDXGIAdapter> warpadapter_;						//warpドライバを使用するときのアダプタ
	ComPtr<IDXGIAdapter1> hardwareadapter_;					//ハードウェアドライバを使用するときのアダプタ(普通はこっち)
	CD3DX12_VIEWPORT viewport_;								//ビューポート
	CD3DX12_RECT scissorrect_;

	//同期関係
	std::vector<ComPtr<ID3D12Fence1>> fence_;								//CPUとGPUの同期に使用されるオブジェクトを表す
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

	Model model_;

	void waiPrevFrame();

	//create
	void createCommandAllocators();
	void createFrameFences();
	void createDescriptorHeaps();
	void prepareRenderTargetView();
	void createDepthBuffer();

public:
	Direct3D();
	~Direct3D();
	void init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear,const wchar_t* MeshShaderFileName,const wchar_t* PixelShaderFileName,HWND Hwnd);
	void update();
	void begin();
	void end();
	void destroy();

	//描画系
	inline void present(UINT SystemInterval, UINT Flag) { swapchain_->Present(SystemInterval, Flag); }
	
	//get
	inline ID3D12Device* getDevice()const { return device_.Get(); }
	inline IDXGISwapChain4* getSwapChain()const { return swapchain_.Get(); }
	inline ID3D12CommandQueue* getCommandQueue()const { return cmdqueue_.Get(); }
	inline ID3D12PipelineState* getPipelineState()const { return pipelinestate_.Get(); }
	inline ComPtr<ID3D12GraphicsCommandList> getCommandList(){ return cmdlist_; }
	inline CD3DX12_VIEWPORT getViewport()const { return viewport_; }
	inline CD3DX12_RECT getScissorRects()const { return scissorrect_; }
	inline HANDLE getFenceEventHandle()const { return fenceevent_; }
	inline std::vector<ComPtr<ID3D12Fence1>> getFence() { return fence_; }
	inline std::vector<unsigned long long >getFenceValue() { return fencevalue_; }
};

