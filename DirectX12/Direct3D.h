#pragma once
const bool kUseHardwareAdapter = true;	//�n�[�h�E�F�A���g�p���邩�ǂ���
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


	ComPtr<ID3D12Device> device_;							//d3d�f�o�C�X
	ComPtr<ID3D12CommandQueue> cmdqueue_;					//�R�}���h���X�g�̑��M�E���s�̓���
	ComPtr<ID3D12DescriptorHeap> rendertargetviewheap_;		//�L�q�q�q�[�v
	ComPtr<ID3D12DescriptorHeap> depthstencilviewheap_;		//�f�v�X�X�e���V���q�[�v
	ComPtr<ID3D12Resource> backbufferrendertarget_[2];		//�o�b�N�o�b�t�@
	ComPtr<ID3D12CommandAllocator> cmdallocator_;			//�R�}���h�̊��蓖��
	ComPtr<ID3D12GraphicsCommandList> cmdlist_;				//�����_�����O�p�̃R�}���h���X�g�̃J�v�Z����
	ComPtr<ID3D12PipelineState> pipelinestate_;				//�O���t�B�b�N�X�p�C�v���C���̃X�e�[�^�X
	ComPtr<ID3D12Resource> depthstencil_;					//�f�v�X�X�e���V��
	ComPtr<ID3D12Resource> constantbuffer_;					//�R���X�^���g�o�b�t�@
	
															//IDXGI
	ComPtr<IDXGISwapChain3> swapchain_;						//�X���b�v�`�F�C��
	ComPtr<IDXGIAdapter> warpadapter_;						//warp�h���C�o���g�p����Ƃ��̃A�_�v�^
	ComPtr<IDXGIAdapter1> hardwareadapter_;					//�n�[�h�E�F�A�h���C�o���g�p����Ƃ��̃A�_�v�^(���ʂ͂�����)

	//���������֌W
	ComPtr<ID3D12Fence> fence_;								//CPU��GPU�̓����Ɏg�p�����I�u�W�F�N�g��\��
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

