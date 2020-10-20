#include "System.h"
#include "Singleton.h"
#include "error.h"
#include "Direct3D.h"
#include "DirectInput.h"
#include "DXHelper.h"
#include "Polygon.h"

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


	result = Singleton<DirectInput>::getPtr()->init();
	if (!result)
	{
		Error::showDialog("DirectInputの初期化に失敗");
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

		if (msg.message == WM_QUIT||Singleton<DirectInput>::getPtr()->isKeyPressed(DIK_ESCAPE))
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

	return true;
}

void System::initWindows()
{
	WNDCLASSEX wc;
	DEVMODE demscreensettings;
	ScreenData wnddata;
	int x, y;

	//LuaStateを取得
	lua_State* initlua = luaL_newstate();
	luaL_openlibs(initlua);

	//Lua内の初期化関数を呼ぶ
	if (luaL_dofile(initlua, "InitWindow.lua"))
	{
		Error::showDialog(lua_tostring(initlua, -1));
	}

	//関数を指定
	lua_getglobal(initlua, "init");
	
	//テーブルをスタックに積む
	lua_pcall(initlua, 0, 2, 0); 

	//テーブルからパラメーターを取得
	lua_getfield(initlua, 1, "width");
	lua_getfield(initlua, 1, "height");

	//テーブルから取得したデータを反映
	if (lua_type(initlua, 2) == LUA_TNUMBER)
		wnddata.height = (unsigned int)lua_tonumber(initlua, 2);
	if (lua_type(initlua, 3) == LUA_TNUMBER)
		wnddata.width = (unsigned int)lua_tonumber(initlua, 3);

	//LuaStateを破棄
	lua_close(initlua);


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
	Singleton<Direct3D>::getPtr()->init(kScreenWidth, kScreenHeight, kVsync, kFullScreen, kScreen_depth, kScreen_near, L"test", L"test");


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
		Singleton<DirectInput>::getPtr()->isKeyState(static_cast<unsigned int>(wParam));
		return 0;
	}
	case WM_KEYUP:
	{
		Singleton<DirectInput>::getPtr()->isKeyState(static_cast<unsigned int>(wParam));
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
	case WM_CREATE:
	{
		LPCREATESTRUCT createstruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createstruct->lpCreateParams));
	}
	return 0;
	case WM_DESTROY:
	case WM_CLOSE:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_PAINT:
	{
		//beginpaint()などのwindowsの描画機能を呼ばないとWM_PAINTが呼ばれ続ける挙動を利用する
		Singleton<Direct3D>::getPtr()->update();
		Singleton<Direct3D>::getPtr()->render();

	}
	default:
	{
		return systemhandle->MessageHandler(hWnd, uMessage, wParam, lParam);
	}
	}
}
