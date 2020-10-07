#pragma once
class DirectXBase
{
private:
	std::wstring assetspath_;
protected:
	//ビューポート用
	UINT width_;
	UINT height_;
	float aspectratio_;
	
	bool usewarpdevice_;

	std::wstring getAssetsFullPath(LPCWSTR AssetsName);
	void getHardwareAdapter(_In_ IDXGIFactory1* Factory, _Outptr_opt_result_maybenull_ IDXGIAdapter1** Adapter,bool RequestHighPerformanceAdapter = false);
public:
	DirectXBase(UINT Width, UINT Height, std::wstring Name);
	virtual ~DirectXBase();
	virtual void init() = 0;
	virtual void update() = 0;
	virtual void render() = 0;
	virtual void destroy() = 0;

	virtual void isKeyDown(UINT8 KeyCode) {};
	virtual void isKeyUp(UINT8 KeyCode) {};

	//get
	const unsigned int getWidth()const { return width_; }
	const unsigned int getHeight()const { return height_; }


};