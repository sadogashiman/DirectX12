#pragma once
#define WIN32_LEAN_AND_MEAN
#include<Windows.h>
#include "SceneBase.h"
class System
{
private:

	struct ScreenData
	{
		int width;
		int height;
	};
	LPCWSTR appname_;
	HINSTANCE hinstance_;
	HWND hwnd_;
	std::unique_ptr<SceneBase> scene_;

	bool update();
	bool render();
	void initWindows();
	void destroyWindows();

public:
	System();
	~System();
	bool init();
	void run();
	void destroy();

	//get
	inline LPCWSTR getAppName() { return appname_; }
	inline HINSTANCE getAppInstance() { return hinstance_; }
	inline HWND getWindowHandle() { return hwnd_; }

	//callback
	LRESULT CALLBACK MessageHandler(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
};
static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);
static System* systemhandle = 0;