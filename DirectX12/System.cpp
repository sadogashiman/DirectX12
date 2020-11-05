#include "System.h"
#include "Singleton.h"
#include "error.h"
#include "Direct3D.h"
#include "DirectInput.h"
#include "DXHelper.h"
#include "Polygon.h"
#include "ColorShader.h"
#include "Game.h"
#include "Timer.h"


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

	scene_.reset(new Game);
	if (!scene_.get()->init())
	{
		Error::showDialog("Sceneの初期化に失敗");
		return false;
	}

	Singleton<Timer>::getPtr()->setTimerStatus(true);

	return true;
}

void System::run()
{
	MSG message;

	//メッセージ構造体初期化
	ZeroMemory(&message, sizeof(MSG));

	//メインループ
	while (1)
	{
		if (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}

		if (message.message == WM_QUIT)
		{
			break;
		}
		else
		{
			Singleton<Timer>::getPtr()->update();

			//60fpsに固定
			if (Singleton<Timer>::getPtr()->fpsControl())
			{
				if (!update())
				{
					break;
				}

			}
		}
	}




}

void System::destroy()
{
}

bool System::update()
{
	SceneBase* tmp;
	Singleton<DirectInput>::getPtr()->update();

	if (Singleton<DirectInput>::getPtr()->isKeyPressed(DIK_ESCAPE))
	{
		return false;
	}
	//更新
	tmp = scene_.get()->update();

	//nullptrなら終了
	if (tmp == nullptr)
	{
		return false;
	}

	//シーンチェンジ
	if (scene_.get() != tmp)
	{
		scene_.get()->destroy();
	}

	return true;
}

bool System::render()
{
	bool result;
	Singleton<Direct3D>::getPtr()->begin();
	result = scene_.get()->render();
	if (!result)
		return result;
	Singleton<Direct3D>::getPtr()->end();

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
