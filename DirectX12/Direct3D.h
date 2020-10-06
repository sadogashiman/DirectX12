#pragma once
const bool kUseHardwareAdapter = true;	//ハードウェアを使用するかどうか
const unsigned int kBufferCount = 2U;
class Direct3D
{
private:
	_declspec(align(256u))struct ConstantBuffer
	{
		Matrix world;
		Matrix worldview;
		Matrix worldviewproj;
		uint32_t drawmeshlts;
	};


	ComPtr<ID3D12Device> device_;							//d3dデバイス
	ComPtr<ID3D12CommandQueue> cmdqueue_;					//コマンドリストの送信・実行の同期
	ComPtr<ID3D12DescriptorHeap> rendertargetviewheap_;		//記述子ヒープ
	ComPtr<ID3D12DescriptorHeap> depthstencilviewheap_;		//デプスステンシルヒープ
	ComPtr<ID3D12Resource> backbufferrendertarget_[2];		//バックバッファ
	ComPtr<ID3D12CommandAllocator> cmdallocator_;			//コマンドの割り当て
	ComPtr<ID3D12GraphicsCommandList> cmdlist_;				//レンダリング用のコマンドリストのカプセル化
	ComPtr<ID3D12PipelineState> pipelinestate_;				//グラフィックスパイプラインのステータス
	ComPtr<ID3D12Resource> depthstencil_;					//デプスステンシル
	ComPtr<ID3D12Resource> constantbuffer_;					//コンスタントバッファ
	
															//IDXGI
	ComPtr<IDXGISwapChain3> swapchain_;						//スワップチェイン
	ComPtr<IDXGIAdapter> warpadapter_;						//warpドライバを使用するときのアダプタ
	ComPtr<IDXGIAdapter1> hardwareadapter_;					//ハードウェアドライバを使用するときのアダプタ(普通はこっち)

	//垂直同期関係
	ComPtr<ID3D12Fence> fence_;								//CPUとGPUの同期に使用されるオブジェクトを表す
	HANDLE fenceevent_;
	unsigned int frameindex_;
	unsigned int framecounter_;
	unsigned long long fencevalue_[kBufferCount];

	char videocarddescription_[128];
	int videocardmemory_;
	unsigned int bufferindex_;
	UINT8* constantbufferviewbegin_;
public:
	Direct3D();
	~Direct3D();
	bool init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear);
	bool render();
	void destroy();
};

