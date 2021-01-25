#include "Support.h"
#include "Singleton.h"
#include "Direct3D.h"
#include "DXHelper.h"
Support::Support()
{
}

Support::~Support()
{
}

HRESULT Support::createShaderV6(std::filesystem::path ShaderPath, std::wstring Profile, ComPtr<ID3DBlob>& ShaderBlob, ComPtr<ID3DBlob>& ErrorMessage)
{
	HRESULT hr;

	////�V�F�[�_�[���f��6.5���T�|�[�g���Ă��邩�`�F�b�N
	//D3D12_FEATURE_DATA_SHADER_MODEL shadermodel = { D3D_SHADER_MODEL_6_5 };
	//if (FAILED(Singleton<Direct3D>::getPtr()->getDevice()->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shadermodel, sizeof(shadermodel)))
	//	||(shadermodel.HighestShaderModel<D3D_SHADER_MODEL_6_5))
	//{
	//	OutputDebugString("ERROR: <GPU> Shader Model 6.5 is not supported\n");
	//	//throw std::exception(" <GPU> Shader Model 6.5 not Supported");
	//}

	////Mesh�V�F�[�_�[���T�|�[�g���Ă��邩�`�F�b�N
	//D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
	//if (FAILED(Singleton<Direct3D>::getPtr()->getDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
	//	|| (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
	//{
	//	OutputDebugStringA("ERROR: <GPU> Mesh Shaders aren't supported!\n");
	//	//throw std::exception(" <GPU> Mesh Shaders aren't supported!");
	//}

	//�p�X���L�����m�F
	if (!std::filesystem::exists(ShaderPath))
	{
		//�g���q��hlsl�ɕϊ����čēx�p�X���m�F
		ShaderPath.replace_extension("hlsl");
		if (std::filesystem::exists(ShaderPath))
		{
			return STG_E_PATHNOTFOUND;
		}
	}

	//�t�@�C���W�J
	std::ifstream ifs;
	ifs.open(ShaderPath, std::ios::binary);
	if (!ifs.is_open())
	{
		return E_FAIL;
	}

	std::vector<char> data;

	//�z��T�C�Y�ύX
	UINT size = static_cast<UINT>(ifs.seekg(0, ifs.end).tellg());
	data.resize(size);

	//�|�C���^���ŏ��ɖ߂��ăf�[�^���擾
	ifs.seekg(0, ifs.beg).read(data.data(), data.size());

	//DXC�ɂ��R���p�C��
	ComPtr<IDxcLibrary> library;
	ComPtr<IDxcCompiler> compiler;
	ComPtr<IDxcBlobEncoding> source;
	ComPtr<IDxcOperationResult> dxcresult;

	DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
	hr = library->CreateBlobWithEncodingFromPinned(data.data(), UINT32(data.size()), CP_ACP, &source);
	if (FAILED(hr))
		return hr;

	DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
	LPCWSTR compileflag[] = {
#ifdef _DEBUG
			L"/Zi",L"/08",
#else
			"L/02" //�����[�X�r���h���͍œK��
#endif // _DEBUG
	};

	hr = compiler->Compile(source.Get(), ShaderPath.wstring().c_str(), L"main", Profile.c_str(), compileflag, _countof(compileflag), nullptr, 0, nullptr, &dxcresult);
	if (FAILED(hr))
	{
		return hr;
	}
	dxcresult->GetStatus(&hr);
	if (SUCCEEDED(hr))
	{
		dxcresult->GetResult(reinterpret_cast<IDxcBlob**>(ShaderBlob.GetAddressOf()));
	}
	else
	{
		dxcresult->GetErrorBuffer(reinterpret_cast<IDxcBlobEncoding**>(ErrorMessage.GetAddressOf()));
	}

	return hr;
}

HRESULT Support::createShaderForCSOFile(std::filesystem::path ShaderPath, ShaderData** ShaderData)
{
	//HRESULT hr;

	//�p�X���L�����m�F
	if (!std::filesystem::exists(ShaderPath))
	{
		return STG_E_PATHNOTFOUND;
	}

	////�t�@�C���������ǂݎ��
	//hr = ReadDataFromFile(ShaderPath.c_str(),);
	//if (FAILED(hr))
	//{
	//	return hr;
	//}

	return S_OK;
}

HRESULT Support::createShader(std::filesystem::path ShaderPath, const wchar_t* Profile, ComPtr<ID3D10Blob>& ShaderBlob, ComPtr<ID3D10Blob>& ErrorMessage)
{
	HRESULT hr = S_OK;
	std::ifstream fs;
	ComPtr<ID3D10Blob> vertexshaderbuff = nullptr;

	//�p�X���m�F
	if (std::filesystem::exists(ShaderPath))
	{
		//�V�F�[�_�[�R���p�C��
	}
	else
	{
		return STG_E_PATHNOTFOUND;
	}

	return hr;
}

ComPtr<ID3D12Resource1> Support::createBuffer(unsigned int Buffersize, const void* InitialData, D3D12_HEAP_TYPE Heaptype, D3D12_RESOURCE_STATES ResourceState)
{
	HRESULT hr;
	ComPtr<ID3D12Resource1> buffer;
	const auto heapprops = CD3DX12_HEAP_PROPERTIES(Heaptype);
	const auto resourcedesc = CD3DX12_RESOURCE_DESC::Buffer(Buffersize);

	hr = Singleton<Direct3D>::getPtr()->getDevice()->CreateCommittedResource(
		&heapprops,
		D3D12_HEAP_FLAG_NONE,
		&resourcedesc,
		ResourceState,
		nullptr,
		IID_PPV_ARGS(&buffer)
	);

	//�����f�[�^�̎w�肪����ꍇ�R�s�[
	if (SUCCEEDED(hr)&&InitialData!=nullptr)
	{
		void* mapped;
		CD3DX12_RANGE range(0, 0);
		hr = buffer->Map(0, &range, &mapped);
		if (SUCCEEDED(hr))
		{
			memcpy(mapped, InitialData, Buffersize);
			buffer->Unmap(0, nullptr);
		}
	}

	return buffer;
}

_Use_decl_annotations_
void Support::getHardwareAdapter(IDXGIFactory1* Factory, IDXGIAdapter1** Adapter, bool RequestHighPerformanceAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;

	ComPtr<IDXGIFactory6> factory6;
	if (SUCCEEDED(Factory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (UINT adapterindex = 0;
			DXGI_ERROR_NOT_FOUND != factory6->EnumAdapterByGpuPreference(
				adapterindex,
				RequestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
				IID_PPV_ARGS(&adapter));
			++adapterindex
			)
		{
			DXGI_ADAPTER_DESC1 adapterdesc;
			adapter->GetDesc1(&adapterdesc);

			if (adapterdesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				//�A�_�v�^�[�^�C�v��Software�������ꍇ�R���e�B�j���[
				continue;
			}

			//�A�_�v�^�[��DirectX12�ɑΉ����Ă��邩�ǂ����𔻒�(�쐬�͂��Ȃ�)
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
				break;
		}
	}
	else
	{
		for (UINT adapterindex = 0;
			DXGI_ERROR_NOT_FOUND != Factory->EnumAdapters1(adapterindex, &adapter);
			++adapterindex
			)
		{
			DXGI_ADAPTER_DESC1 adapterdesc;
			adapter->GetDesc1(&adapterdesc);

			if (adapterdesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{
				//�A�_�v�^�[�^�C�v��Software�������ꍇ�R���e�B�j���[
				continue;
			}

			//�A�_�v�^�[��DirectX12�ɑΉ����Ă��邩�ǂ����𔻒�(�쐬�͂��Ȃ�)
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
				break;
		}
	}

	*Adapter = adapter.Detach();
}
