#pragma once

struct ShaderData
{
	byte* data;
	uint32_t size;
};

class Support
{
private:
	LPVOID vertexblob_;
	LPVOID pixelblob_;

public:
	Support();
	~Support();

	static HRESULT createShaderV6(std::filesystem::path ShaderPath, std::wstring Profile, ComPtr<ID3DBlob>& ShaderBlob, ComPtr<ID3DBlob>& ErrorMessage);
	static HRESULT createShaderForCSOFile(std::filesystem::path ShaderPath, ShaderData** ShaderData);
	static HRESULT createShader(std::filesystem::path ShdaerPath, const wchar_t* Profile, ComPtr<ID3D10Blob>& ShaderBlob, ComPtr<ID3D10Blob>& ErrorMessage);
	
	//create
	
	//get
	static void getHardwareAdapter(_In_ IDXGIFactory1* Factory,_Outptr_opt_result_maybenull_ IDXGIAdapter1** Adapter, bool RequestHighPerformanceAdapter = false);
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