#include "stdafx.h"
#include "DXHelper.h"
#include "DirectXBase.h"
#include "Singleton.h"
#include "System.h"

DirectXBase::DirectXBase(UINT Width, UINT Height, std::wstring Name) :
	width_(Width),
	height_(Height),
	usewarpdevice_(false)
{
	WCHAR assetspath[512];
	GetAssetsPath(assetspath, _countof(assetspath));
	assetspath_ = assetspath;

	//アスペクト比を計算
	aspectratio_ = static_cast<float>(Width) / static_cast<float>(Height);
}

DirectXBase::~DirectXBase()
{
}


std::wstring DirectXBase::getAssetsFullPath(LPCWSTR AssetsName)
{
	return assetspath_ + AssetsName;
}
void DirectXBase::getHardwareAdapter(IDXGIFactory1* Factory, IDXGIAdapter1** Adapter, bool RequestHighPerformanceAdapter)
{
}