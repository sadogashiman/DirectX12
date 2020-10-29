#pragma once
#include "Model.h"
const bool kUseHardwareAdapter = true;	//�n�[�h�E�F�A���g�p���邩�ǂ���
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
	ComPtr<ID3D12Device> device_;							//d3d�f�o�C�X
	ComPtr<ID3D12CommandQueue> cmdqueue_;					//�R�}���h���X�g�̑��M�E���s�̓���
	ComPtr<ID3D12DescriptorHeap> rendertargetviewheap_;		//�L�q�q�q�[�v
	ComPtr<ID3D12DescriptorHeap> depthstencilviewheap_;		//�f�v�X�X�e���V���q�[�v
	std::vector<ComPtr<ID3D12Resource1>> rendertargets_;		//�o�b�N�o�b�t�@
	std::vector<ComPtr<ID3D12CommandAllocator>> cmdallocator_;			//�R�}���h�̊��蓖��
	ComPtr<ID3D12GraphicsCommandList> cmdlist_;				//�����_�����O�p�̃R�}���h���X�g�̃J�v�Z����
	ComPtr<ID3D12PipelineState> pipelinestate_;				//�O���t�B�b�N�X�p�C�v���C���̃X�e�[�^�X
	ComPtr<ID3D12Resource> depthstencil_;					//�f�v�X�X�e���V��
	ComPtr<ID3D12Resource> constantbuffer_;					//�R���X�^���g�o�b�t�@
	ComPtr<ID3D12RootSignature> rootsignature_;				//���[�g�V�O�l�`��

	//IDXGI
	ComPtr<IDXGISwapChain4> swapchain_;						//�X���b�v�`�F�C��
	ComPtr<IDXGIAdapter> warpadapter_;						//warp�h���C�o���g�p����Ƃ��̃A�_�v�^
	ComPtr<IDXGIAdapter1> hardwareadapter_;					//�n�[�h�E�F�A�h���C�o���g�p����Ƃ��̃A�_�v�^(���ʂ͂�����)
	CD3DX12_VIEWPORT viewport_;								//�r���[�|�[�g
	CD3DX12_RECT scissorrect_;

	//�����֌W
	std::vector<ComPtr<ID3D12Fence1>> fence_;								//CPU��GPU�̓����Ɏg�p�����I�u�W�F�N�g��\��
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

	//�`��n
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

