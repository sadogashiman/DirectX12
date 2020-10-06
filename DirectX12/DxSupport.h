#pragma once
class DxSupport
{
private:

public:
	static void getAssetFullPath(LPCWSTR AssetName);
	static void getHardwareAdapter(_In_ IDXGIFactory1* Factory,_Outptr_opt_result_maybenull_ IDXGIAdapter1** Adapter, bool RequestHighPerformanceAdapter = false);
	static void setCustomWindowText(LPCWSTR Text);
	static void parseCommandLineArgs(_In_reads_(argc) WCHAR* args[], int argc);
};

#if defined(_DEBUG)||defined(DBG)
inline void setName(ID3D12Object* Object, LPCWSTR Name)
{
	Object->SetName(Name);
}

inline void setNameIndexed(ID3D12Object* Object, LPCWSTR Name, UINT Index)
{
	WCHAR fullname[50];
	if (swprintf_s(fullname, L"%s[%u]", Name, Index) > 0)
	{
		Object->SetName(fullname);
	}
}
#endif

#define NAME_D3D12_OBJECT(x) setName((x).Get(),L#x)
#define NAME_D3D12_OBJECT_INDEXED(x,n)setNameIndexed((x)[n].Get(),L#x,n)