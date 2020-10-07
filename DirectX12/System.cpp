#include "System.h"
#include "Input.h"
#include "Singleton.h"
#include "error.h"
#include "Direct3D.h"

System::System()
{
	//自分自身を初期化
	ZeroMemory(this, sizeof(System));
}

System::~System()
{
}

bool System::init()
{
	bool result;

	//Windowsを初期化
	initWindows();

	//各クラスを初期化
	result = Singleton<Input>::getPtr()->init();
	if (!result)
	{
		Error::showDialog("DirectInput 初期化エラー");
		return false;
	}

	result = Singleton<Direct3D>::getPtr()->init(kScreenWidth, kScreenHeight, kVsync, kFullScreen, kScreen_depth, kScreen_near,L"test_ms.hlsl",L"test_ps.hlsl");
	if (!result)
	{
		Error::showDialog("Direct3Dクラスの初期化に失敗");
		return false;
	}




	return true;
}

void System::run()
{
	MSG msg;
	bool result;
	bool done;

	//構造体初期化
	ZeroMemory(&msg, sizeof(MSG));

	done = false;

	while (!done)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_QUIT)
		{
			done = true;
		}
		else
		{
			result = update();
			if (!result)
			{
				done = true;
			}
		}
	}
}

void System::destroy()
{
}

bool System::update()
{
	//更新
	Singleton<Input>::getPtr()->update();

	//アプリケーション終了
	if (Singleton<Input>::getPtr()->quitApp())
		return false;

	return true;
}

void System::initWindows()
{
	WNDCLASSEX wc;
	DEVMODE demscreensettings;
	ScreenData wnddata;
	int x, y;

	//このオブジェクトへのポインタを作成
	systemhandle = this;

	//このアプリケーションのインスタンスを作成
	hinstance_ = GetModuleHandle(NULL);

	//ウィンドウの設定
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hinstance_;
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
	wc.hIconSm = wc.hIcon;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = "DirectX12";
	wc.cbSize = sizeof(WNDCLASSEX);

	//ウィンドウクラスを登録
	RegisterClassEx(&wc);

	//スクリーンサイズを取得
	wnddata.width = GetSystemMetrics(SM_CXSCREEN);
	wnddata.height = GetSystemMetrics(SM_CYSCREEN);

	//ウィンドウをセットアップ
	if (kFullScreen)
	{
		//フルスクリーンの場合
		memset(&demscreensettings, 0, sizeof(demscreensettings));
		demscreensettings.dmSize = sizeof(demscreensettings);
		demscreensettings.dmPelsHeight = (unsigned long)wnddata.height;
		demscreensettings.dmPelsWidth = (unsigned long)wnddata.width;
		demscreensettings.dmBitsPerPel = kWindowColorBit;
		demscreensettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		//ウィンドウをフルスクリーンに設定
		ChangeDisplaySettings(&demscreensettings, CDS_FULLSCREEN);

		//表示座標を設定
		x = y = 0;
	}
	else
	{
		wnddata.height = kWindow_Height;
		wnddata.width = kWindow_Width;

		x = (GetSystemMetrics(SM_CXSCREEN) - wnddata.width) / 2;
		y = (GetSystemMetrics(SM_CYSCREEN) - wnddata.height) / 2;
	}

	//ウィンドウを作成
	hwnd_ = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		"DirectX12",
		"DirectX12",
		WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
		x,
		y,
		wnddata.width,
		wnddata.height,
		NULL,
		NULL,
		hinstance_,
		NULL
	);

	//ウィンドウにフォーカスをセット
	ShowWindow(hwnd_, SW_SHOW);
	SetForegroundWindow(hwnd_);
	SetFocus(hwnd_);

#ifdef _DEBUG
	//デバッグ時のみマウスカーソルを表示
	ShowCursor(true);
#else
	ShowCursor(false);
#endif // _DEBUG
}

void System::destroyWindows()
{
	//マウスカーソルを表示
	ShowCursor(true);

	//フルスクリーンの場合ウィンドウモードに切り替える
	if (kFullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	//ウィンドウを破棄
	DestroyWindow(hwnd_);
	hwnd_ = NULL;

	//アプリケーションインスタンスを破棄
	UnregisterClass("DirectX12", hinstance_);
	hinstance_ = NULL;

	//このクラスへのポインタを破棄
	systemhandle = NULL;
}

LRESULT CALLBACK System::MessageHandler(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_KEYDOWN:
	{
		Singleton<Input>::getPtr()->isKeyPressed((unsigned int)wParam);
		return 0;
	}
	case WM_KEYUP:
	{
		Singleton<Input>::getPtr()->isKeyReleased((unsigned int)wParam);
		return 0;
	}
	default:
	{
		return DefWindowProc(hWnd, uMessage, wParam, lParam);
	}

	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_DESTROY:
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}
	default:
	{
		return systemhandle->MessageHandler(hWnd, uMessage, wParam, lParam);
	}
	}
}
