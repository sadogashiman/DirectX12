#include "stdafx.h"
#include "Direct3D.h"
#include "error.h"
#include "Support.h"
#include "Singleton.h"
#include "System.h"
#include "DXHelper.h"

Direct3D::Direct3D()
{
}

Direct3D::~Direct3D()
{
}

void Direct3D::init(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear, const wchar_t* MeshShaderFileName, const wchar_t* PixelShaderFileName)
{
	//�p�C�v���C�����쐬
	loadPipeline(ScreenWidth, ScreenHeight, Vsync, FullScreen, ScreenDepth, ScreenNear);


	//�r���[�|�[�g�̐ݒ�
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
	//�V�[���������_�����O���邽�߂ɕK�v�ȃR�}���h���R�}���h���X�g�ɋL�^
	populateCommandList();

	//�R�}���h���X�g���s
	ID3D12CommandList* commandlists[] = { cmdlist_.Get() };
	cmdqueue_->ExecuteCommandLists(_countof(commandlists), commandlists);

	//present
	hr = swapchain_->Present(1, 0);
	ThrowIfFailed(hr);

	moveToNextFrame();
}

void Direct3D::destroy()
{
	//GPU�Ŏ��s�҂��̃��\�[�X���Ȃ����m�F(�f�X�g���N�^�ŃN���[���A�b�v�N���[���A�b�v)
	waitForGPU();

	CloseHandle(fenceevent_);
}

void Direct3D::loadAssets(const wchar_t* MeshShaderFileName, const wchar_t* PixelShaderFileName)
{




}

void Direct3D::loadPipeline(const int ScreenWidth, const int ScreenHeight, const bool Vsync, const bool FullScreen, const float ScreenDepth, const float ScreenNear)
{
#ifdef _DEBUG
	//�f�o�b�O���̂݃f�o�b�O���C���[��L��������
	//�f�o�C�X�̍쐬��ɗL���ɂ���ƈӖ����Ȃ��̂Ńf�o�C�X�̍쐬�O�ɐݒ肷��
	UINT dxgidebugflag = 0;
	ComPtr<ID3D12Debug>debugcontroller;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(debugcontroller.ReleaseAndGetAddressOf()))))
	{
		//�f�o�b�O���C���[��L���ɂ���
		debugcontroller->EnableDebugLayer();

		//�ǉ��̃f�o�b�O���C���[��L���ɂ���
		dxgidebugflag |= DXGI_CREATE_FACTORY_DEBUG;
	}
#endif // _DEBUG

	D3D_FEATURE_LEVEL featurelevel;
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC commandqueuedesc;
	DXGI_SWAP_CHAIN_DESC1 swapchaindesc;


	//�@�\���x���̐ݒ�
	featurelevel = D3D_FEATURE_LEVEL_12_1;

	//Direct3D�f�o�C�X�̍쐬
	hr = D3D12CreateDevice(
		NULL,
		featurelevel,
		__uuidof(ID3D12Device),
		(void**)device_.ReleaseAndGetAddressOf()
	);

	//�t�@�N�g�����쐬
	ComPtr<IDXGIFactory4> factory;
	hr = CreateDXGIFactory2(dxgidebugflag, IID_PPV_ARGS(&factory));
	ThrowIfFailed(hr);


	//GPU���������݂��邱�Ƃ�O��ɏ�����
	if (kUseHardwareAdapter)
	{
		ComPtr<IDXGIAdapter1> hardwareadapter;
		Support::getHardwareAdapter(factory.Get(), &hardwareadapter);

		//Direct3D�f�o�C�X���쐬
		hr = D3D12CreateDevice(
			hardwareadapter.Get(),
			D3D_FEATURE_LEVEL_12_1,
			IID_PPV_ARGS(&device_)
		);
		if (FAILED(hr))
		{
			//�@�\���x���������邱�Ƃ��o�͂��ċ@�\���x�����Đݒ�
			OutputDebugString("It does not correspond to the Feature level set by the GPU. Lower the Feature level and reset it");
			hr = D3D12CreateDevice(
				hardwareadapter.Get(),
				D3D_FEATURE_LEVEL_12_0,
				IID_PPV_ARGS(&device_));
			ThrowIfFailed(hr);
		}
	}
	else
	{
		ComPtr<IDXGIAdapter> warpadapter;
		//�A�_�v�^�̗�
		hr = factory->EnumWarpAdapter(IID_PPV_ARGS(&warpadapter));
		ThrowIfFailed(hr);

		//warp�A�_�v�^���g�p����Ƃ��͋@�\���x���������Ă���
		hr = D3D12CreateDevice(
			warpadapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&device_)
		);
		ThrowIfFailed(hr);
	}

	//�R�}���h�L���[��������
	ZeroMemory(&commandqueuedesc, sizeof(commandqueuedesc));

	//�R�}���h�L���[�̐ݒ�
	commandqueuedesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;          //�R�}���h�L���[�̎��(DIRECT�EGPU�����s�ł���R�}���h�o�b�t�@���w��)
	commandqueuedesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL; //�R�}���h�L���[�̗D��x�@(�ʏ��NORMAL)
	commandqueuedesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;          //�R�}���h�L���[���쐬����Ƃ��̐ݒ�(�ʏ��NONE)
	commandqueuedesc.NodeMask = 0;                                   //0���w��(���GPU�݂̂��g�p)

	//�R�}���h�L���[�̍쐬
	hr = device_->CreateCommandQueue(&commandqueuedesc, IID_PPV_ARGS(&cmdqueue_));
	ThrowIfFailed(hr);

	//�X���b�v�`�F�C����������
	ZeroMemory(&swapchaindesc, sizeof(swapchaindesc));

	//�X���b�v�`�F�C���̐ݒ�
	swapchaindesc.BufferCount = kBufferCount;
	swapchaindesc.Width = kWindow_Width;
	swapchaindesc.Height = kWindow_Height;
	swapchaindesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapchaindesc.SampleDesc.Count = 1;
	swapchaindesc.SampleDesc.Quality = 0;
	swapchaindesc.Flags = 0;

	//�X���b�v�`�F�C�����쐬
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


	//Alt+Enter�𖳌��ɂ���
	hr = factory->MakeWindowAssociation(Singleton<System>::getPtr()->getWindowHandle(), DXGI_MWA_NO_ALT_ENTER);
	ThrowIfFailed(hr);

	hr = swapchain.As(&swapchain_);
	ThrowIfFailed(hr);

	//�o�b�N�o�b�t�@�̐����擾
	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//descriptor heap�̐ݒ�(RTV)
	D3D12_DESCRIPTOR_HEAP_DESC rtvheapdesc;
	ZeroMemory(&rtvheapdesc, sizeof(rtvheapdesc));
	rtvheapdesc.NumDescriptors = kBufferCount;
	rtvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;   //�����_�[�^�[�Q�b�g�Ƃ��Đݒ�
	rtvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvheapdesc.NodeMask = 0;

	hr = device_->CreateDescriptorHeap(&rtvheapdesc, IID_PPV_ARGS(&rendertargetviewheap_));
	ThrowIfFailed(hr);

	//�T�C�Y���擾
	rendertargetdescriptionsize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	//descriptor heap�̐ݒ�(DSV)
	D3D12_DESCRIPTOR_HEAP_DESC dsvheapdesc;
	ZeroMemory(&dsvheapdesc, sizeof(dsvheapdesc));
	dsvheapdesc.NumDescriptors = 1;
	dsvheapdesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvheapdesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = device_->CreateDescriptorHeap(&dsvheapdesc, IID_PPV_ARGS(&depthstencilviewheap_));
	ThrowIfFailed(hr);

	//�T�C�Y���擾
	depthstencildescriptionsize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//�t���[�����\�[�X���쐬
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart());

	//�t���[�����ƂɃR�}���h�A���P�[�^�ƃ����_�[�^�[�Q�b�g���r���[���쐬
	for (UINT i = 0; i < kBufferCount; i++)
	{
		hr = swapchain_->GetBuffer(i, IID_PPV_ARGS(&rendertargets[i]));
		ThrowIfFailed(hr);


		device_->CreateRenderTargetView(rendertargets[i].Get(), nullptr, rtvhandle);
		rtvhandle.Offset(1, rendertargetdescriptionsize_);

		hr = device_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&cmdallocator_[i]));
		ThrowIfFailed(hr);
	}

	//�f�v�X�X�e���V���r���[�̐ݒ�
	D3D12_DEPTH_STENCIL_VIEW_DESC depthstencildesc;
	ZeroMemory(&depthstencildesc, sizeof(depthstencildesc));
	depthstencildesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthstencildesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthstencildesc.Flags = D3D12_DSV_FLAG_NONE;

	//�N���A�l�̐ݒ�
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
	ThrowIfFailed(hr);

	NAME_D3D12_OBJECT(depthstencil_);

	//�f�v�X�X�e���V���̍쐬
	device_->CreateDepthStencilView(depthstencil_.Get(), &depthstencildesc, depthstencilviewheap_.Get()->GetCPUDescriptorHandleForHeapStart());

	//�R���X�^���g�o�b�t�@�̍쐬
	const UINT constantbuffersize = sizeof(ConstantBuffer) * kBufferCount;
	hr = device_->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(constantbuffersize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&constantbuffer_)
	);
	ThrowIfFailed(hr);

	//�R���X�^���g�o�b�t�@�r���[�̐ݒ�
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvdesc;
	ZeroMemory(&cbvdesc, sizeof(cbvdesc));
	cbvdesc.BufferLocation = constantbuffer_->GetGPUVirtualAddress();
	cbvdesc.SizeInBytes = constantbuffersize;

	//�R���X�^���g�o�b�t�@���}�b�v���ď�����
	CD3DX12_RANGE readrange(0, 0);
	hr = constantbuffer_->Map(0, &readrange, reinterpret_cast<void**>(&constantbufferviewbegin_));
	ThrowIfFailed(hr);
}

void Direct3D::populateCommandList()
{
	HRESULT hr;

	//�R�}���h���X�g�̏�����
	//�R�}���h�A���P�[�^�͊֘A�t������Ă���ꍇ�̂ݏ������\
	hr = cmdlist_->Reset(cmdallocator_[frameindex_].Get(), pipelinestate_.Get());
	ThrowIfFailed(hr);

	//�R�}���h���X�g�ɕK�p�ȏ����Z�b�g
	cmdlist_->SetGraphicsRootSignature(rootsignature_.Get());
	cmdlist_->RSSetViewports(1, &viewport_);
	cmdlist_->RSSetScissorRects(1, &scissorrect_);

	//�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g�ɐݒ�
	cmdlist_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rendertargets[frameindex_].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart(), frameindex_, rendertargetdescriptionsize_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvhandle(depthstencilviewheap_->GetCPUDescriptorHandleForHeapStart());

	//�R�}���h���L�^
	const float clearcolor[] {0.0F, 0.0F, 0.0F, 0.0F};
	cmdlist_->ClearRenderTargetView(rtvhandle, clearcolor, 0, nullptr);
	cmdlist_->ClearDepthStencilView(dsvhandle, D3D12_CLEAR_FLAG_DEPTH, 1.0F, 0, 0, nullptr);

	cmdlist_->SetGraphicsRootConstantBufferView(0, constantbuffer_->GetGPUVirtualAddress() + sizeof(ConstantBuffer) * frameindex_);

	for (auto& mesh : model_)
	{
		cmdlist_->SetGraphicsRoot32BitConstant(1, mesh.IndexSize, 0);
		cmdlist_->SetGraphicsRootShaderResourceView(2, mesh.VertexResources[0]->GetGPUVirtualAddress());
		cmdlist_->SetGraphicsRootShaderResourceView(3, mesh.MeshletResource->GetGPUVirtualAddress());
		cmdlist_->SetGraphicsRootShaderResourceView(4, mesh.UniqueVertexIndexResource->GetGPUVirtualAddress());
		cmdlist_->SetGraphicsRootShaderResourceView(5, mesh.PrimitiveIndexResource->GetGPUVirtualAddress());

		for (auto& Subset : mesh.MeshletSubsets)
		{
			cmdlist_->SetGraphicsRoot32BitConstant(1, Subset.Offset, 1);
			cmdlist_->DispatchMesh(Subset.Count, 1, 1);
		}
	}

	//�o�b�N�o�b�t�@�ɕ`��(Present�I�Ȋ֐�)
	cmdlist_->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(rendertargets[frameindex_].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(cmdlist_->Close());
}

void Direct3D::waitForGPU()
{
	HRESULT hr;
	hr = cmdqueue_->Signal(fence_.Get(), fencevalue_[frameindex_]);
	ThrowIfFailed(hr);

	//�t�F���X�̏����������܂őҋ@
	hr = fence_->SetEventOnCompletion(fencevalue_[frameindex_], fenceevent_);
	ThrowIfFailed(hr);
	WaitForSingleObjectEx(fenceevent_, INFINITE, FALSE);

	//���݂̃t���[���̃t�F���X�̒l���C���N�������g
	fencevalue_[frameindex_]++;
}

void Direct3D::moveToNextFrame()
{
	const UINT64 currentfencevalue = fencevalue_[frameindex_];
	ThrowIfFailed(cmdqueue_->Signal(fence_.Get(), currentfencevalue));

	//�t���[���C���f�b�N�X���X�V
	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//
	if (fence_->GetCompletedValue() < fencevalue_[frameindex_])
	{
		ThrowIfFailed(fence_->SetEventOnCompletion(fencevalue_[frameindex_], fenceevent_));
		WaitForSingleObjectEx(fenceevent_, INFINITE, FALSE);
	}

	//�t�F���X�̒l�����̃t���[����
	fencevalue_[frameindex_] = currentfencevalue + 1;
}
