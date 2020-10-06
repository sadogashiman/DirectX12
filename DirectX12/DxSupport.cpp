#include "stdafx.h"
#include "DxSupport.h"

void DxSupport::getAssetFullPath(LPCWSTR AssetName)
{
}

_Use_decl_annotations_
void DxSupport::getHardwareAdapter(IDXGIFactory1* Factory, IDXGIAdapter1** Adapter, bool RequestHighPerformanceAdapter)
{
	*Adapter = nullptr;

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

void DxSupport::setCustomWindowText(LPCWSTR Text)
{
}

void DxSupport::parseCommandLineArgs(WCHAR* args[], int argc)
{
}
