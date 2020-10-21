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

	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//�R�}���h���Z�b�g
	cmdallocator_[frameindex_]->Reset();
	cmdlist_->Reset(cmdallocator_[frameindex_].Get(),nullptr);

	//�X���b�v�`�F�C���\���\���烌���_�[�^�[�Q�b�g�`��\��
	auto barrirtorendertarget = CD3DX12_RESOURCE_BARRIER::Transition(
		rendertargets_[frameindex_].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdlist_->ResourceBarrier(1, &barrirtorendertarget);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(
		rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart(),
		frameindex_,
		rendertargetdescriptionsize_);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(
		depthstencilviewheap_->GetCPUDescriptorHandleForHeapStart());

	//�J���[�o�b�t�@�i�����_�[�^�[�Q�b�g�r���[�̃N���A)
	cmdlist_->ClearRenderTargetView(rtv, kClearColor, 0, nullptr);

	//�[�x�o�b�t�@�̃N���A
	cmdlist_->ClearDepthStencilView(
		dsv,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0F,
		0,
		0,
		nullptr
	);

	//�`�����Z�b�g
	cmdlist_->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	auto barrirtopresent = CD3DX12_RESOURCE_BARRIER::Transition(
		rendertargets_[frameindex_].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	cmdlist_->ResourceBarrier(1, &barrirtopresent);
	cmdlist_->Close();

	ID3D12CommandList* lists[] = { cmdlist_.Get() };
	cmdqueue_->ExecuteCommandLists(1, lists);

	swapchain_->Present(1, 0);
	
	waiPrevFrame();
}

void Direct3D::destroy()
{
	CloseHandle(fenceevent_);
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

	//GBV�̗L����
	ComPtr<ID3D12Debug3>gbvdebug;
	debugcontroller.As(&gbvdebug);
	gbvdebug->SetEnableGPUBasedValidation(true);
#endif // _DEBUG

	D3D_FEATURE_LEVEL featurelevel = D3D_FEATURE_LEVEL_12_1;
	HRESULT hr;
	D3D12_COMMAND_QUEUE_DESC commandqueuedesc;
	DXGI_SWAP_CHAIN_DESC1 swapchaindesc;


	//�t�@�N�g�����쐬
	ComPtr<IDXGIFactory3> factory;
	hr = CreateDXGIFactory2(dxgidebugflag, IID_PPV_ARGS(&factory));
	ThrowIfFailed(hr);

	//�n�[�h�E�F�A�A�_�v�^�̌���
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

	//�g�p����A�_�v�^
	adapter.As(&useadapter);

	//D3D�f�o�C�X���쐬
	hr = D3D12CreateDevice(
		useadapter.Get(),
		featurelevel,
		IID_PPV_ARGS(&device_)
	);
	ThrowIfFailed(hr);


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

	hr = swapchain.As(&swapchain_); //IDXGISwapChain4�擾
	ThrowIfFailed(hr);

	//�o�b�N�o�b�t�@�̐����擾
	frameindex_ = swapchain_->GetCurrentBackBufferIndex();

	//�e�f�B�X�N���v�^�[�q�[�v�̍쐬
	createDescriptorHeaps();

	//�����_�[�^�[�Q�b�g�r���[�̍쐬
	prepareRenderTargetView();

	//�f�v�X�o�b�t�@�֘A�̍쐬
	createDepthBuffer();

	//�R�}���h�A���P�[�^�[�̏���
	createCommandAllocators();

	//�`��t���[�������p�t�F���X����
	createFrameFences();

	//�R�}���h���X�g�̏�����
	hr = device_->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		cmdallocator_[0].Get(),
		nullptr,
		IID_PPV_ARGS(&cmdlist_)
	);

	cmdlist_->Close();
}

void Direct3D::waiPrevFrame()
{
	auto& fence = fence_[frameindex_];
	const auto currentvalue = ++fencevalue_[frameindex_];
	cmdqueue_->Signal(fence.Get(), currentvalue);

	//����������R�}���h(�A���P�[�^�[)�̂��͎̂��s�����ς݂������ɂȂ��Ă���t�F���X�Ŋm�F
	auto nextindex = (frameindex_ + 1) % kBufferCount;
	const auto finishexpect = fencevalue_[nextindex];
	const auto nextfencevalue = fence_[nextindex]->GetCompletedValue();
	if (nextfencevalue < finishexpect)
	{
		fence_[nextindex]->SetEventOnCompletion(finishexpect, fenceevent_);
		WaitForSingleObject(fenceevent_, kGpuWaitTimeout);
	}
}


void Direct3D::createCommandAllocators()
{
	HRESULT hr;
	//�z��T�C�Y�ύX
	cmdallocator_.resize(kBufferCount);

	//�t���[�����ƂɃR�}���h�A���P�[�^�ƃ����_�[�^�[�Q�b�g���r���[���쐬
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

	//�z��T�C�Y�ύX
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

void Direct3D::createDescriptorHeaps()
{
	HRESULT hr;
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

}

void Direct3D::prepareRenderTargetView()
{
	HRESULT hr;
	//�t���[�����\�[�X���쐬
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvhandle(rendertargetviewheap_->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < kBufferCount; ++i)
	{
		hr = swapchain_->GetBuffer(i, IID_PPV_ARGS(&rendertargets_[i]));
		ThrowIfFailed(hr);

		device_->CreateRenderTargetView(rendertargets_[i].Get(), nullptr, rtvhandle);
		rtvhandle.Offset(1, rendertargetdescriptionsize_);
	}
}

void Direct3D::createDepthBuffer()
{
	HRESULT hr;

	auto depthbufferdesc = CD3DX12_RESOURCE_DESC::Tex2D(
		DXGI_FORMAT_D32_FLOAT,
		kWindow_Width,
		kWindow_Height,
		1,
		0,
		1,
		0,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	//�N���A�l�̐ݒ�
	D3D12_CLEAR_VALUE depthoptimizedclearvalue;
	depthoptimizedclearvalue.Format = depthbufferdesc.Format;
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

	//�f�v�X�X�e���V���r���[�쐬
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvdesc
	{
		DXGI_FORMAT_D32_FLOAT,
		D3D12_DSV_DIMENSION_TEXTURE2D,
		D3D12_DSV_FLAG_NONE,
		{
			0
		}
	};
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvhandle(depthstencilviewheap_->GetCPUDescriptorHandleForHeapStart());

	//�f�v�X�X�e���V���̍쐬
	device_->CreateDepthStencilView(depthstencil_.Get(), &dsvdesc, depthstencilviewheap_.Get()->GetCPUDescriptorHandleForHeapStart());
}
