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

	//シェーダーモデル6.5をサポートしているかチェック
	D3D12_FEATURE_DATA_SHADER_MODEL shadermodel = { D3D_SHADER_MODEL_6_5 };
	if (FAILED(Singleton<Direct3D>::getPtr()->getDevice()->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &shadermodel, sizeof(shadermodel)))
		||(shadermodel.HighestShaderModel<D3D_SHADER_MODEL_6_5))
	{
		OutputDebugString("ERROR: Shader Model 6.5 is not supported\n");
		//throw std::exception("Shader Model 6.5 not Supported");
	}

	//Meshシェーダーをサポートしているかチェック
	D3D12_FEATURE_DATA_D3D12_OPTIONS7 features = {};
	if (FAILED(Singleton<Direct3D>::getPtr()->getDevice()->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS7, &features, sizeof(features)))
		|| (features.MeshShaderTier == D3D12_MESH_SHADER_TIER_NOT_SUPPORTED))
	{
		OutputDebugStringA("ERROR: Mesh Shaders aren't supported!\n");
		//throw std::exception("Mesh Shaders aren't supported!");
	}

	//パスが有効か確認
	if (!std::filesystem::exists(ShaderPath))
	{
		//拡張子をhlslに変換して再度パスを確認
		ShaderPath.replace_extension("hlsl");
		if (std::filesystem::exists(ShaderPath))
		{
			return STG_E_PATHNOTFOUND;
		}
	}

	//ファイル展開
	std::ifstream ifs;
	ifs.open(ShaderPath, std::ios::binary);
	if (!ifs.is_open())
	{
		return E_FAIL;
	}

	std::vector<char> data;

	//配列サイズ変更
	data.resize(ifs.seekg(0, ifs.end).tellg());

	//ポインタを最初に戻してデータを取得
	ifs.seekg(0, ifs.beg).read(data.data(), data.size());

	//DXCによるコンパイル
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
			"L/02" //リリースビルド時は最適化
#endif // _DEBUG
	};

	compiler->Compile(source.Get(), ShaderPath.wstring().c_str(), L"main", Profile.c_str(), compileflag, _countof(compileflag), nullptr, 0, nullptr, &dxcresult);

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
	HRESULT hr;

	//パスが有効か確認
	if (!std::filesystem::exists(ShaderPath))
	{
		return STG_E_PATHNOTFOUND;
	}

	////ファイルから情報を読み取り
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

	//パスを確認
	if (std::filesystem::exists(ShaderPath))
	{
		//シェーダーコンパイル
	}
	else
	{
		return STG_E_PATHNOTFOUND;
	}

	return hr;
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
				//アダプタータイプがSoftwareだった場合コンティニュー
				continue;
			}

			//アダプターがDirectX12に対応しているかどうかを判定(作成はしない)
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
				//アダプタータイプがSoftwareだった場合コンティニュー
				continue;
			}

			//アダプターがDirectX12に対応しているかどうかを判定(作成はしない)
			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
				break;
		}
	}

	*Adapter = adapter.Detach();
}
