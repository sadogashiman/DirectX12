#include "System.h"
#include "Singleton.h"
#include "error.h"
#include "Direct3D.h"
#include "DirectInput.h"
#include "DXHelper.h"
#include "Polygon.h"
#include "ColorShader.h"

System::System()
{
	//�������g��������
	ZeroMemory(this, sizeof(System));
}

System::~System()
{
}

bool System::init()
{
	bool result;

	//Windows��������
	initWindows();


	result = Singleton<DirectInput>::getPtr()->init();
	if (!result)
	{
		Error::showDialog("DirectInput�̏������Ɏ��s");
		return false;
	}





	return true;
}

void System::run()
{
	MSG msg;
	bool result;
	bool done;

	//�\���̏�����
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

	//LuaState���擾
	lua_State* initlua = luaL_newstate();
	luaL_openlibs(initlua);

	//Lua���̏������֐����Ă�
	if (luaL_dofile(initlua, "InitWindow.lua"))
	{
		Error::showDialog(lua_tostring(initlua, -1));
	}

	//�֐����w��
	lua_getglobal(initlua, "init");
	
	//�e�[�u�����X�^�b�N�ɐς�
	lua_pcall(initlua, 0, 2, 0); 

	//�e�[�u������p�����[�^�[���擾
	lua_getfield(initlua, 1, "width");
	lua_getfield(initlua, 1, "height");

	//�e�[�u������擾�����f�[�^�𔽉f
	if (lua_type(initlua, 2) == LUA_TNUMBER)
		wnddata.height = (unsigned int)lua_tonumber(initlua, 2);
	if (lua_type(initlua, 3) == LUA_TNUMBER)
		wnddata.width = (unsigned int)lua_tonumber(initlua, 3);

	//LuaState��j��
	lua_close(initlua);

	//���̃I�u�W�F�N�g�ւ̃|�C���^���쐬
	systemhandle = this;

	//���̃A�v���P�[�V�����̃C���X�^���X���쐬
	hinstance_ = GetModuleHandle(NULL);

	//�E�B���h�E�̐ݒ�
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

	//�E�B���h�E�N���X��o�^
	RegisterClassEx(&wc);

	//�X�N���[���T�C�Y���擾
	wnddata.width = GetSystemMetrics(SM_CXSCREEN);
	wnddata.height = GetSystemMetrics(SM_CYSCREEN);

	//�E�B���h�E���Z�b�g�A�b�v
	if (kFullScreen)
	{
		//�t���X�N���[���̏ꍇ
		memset(&demscreensettings, 0, sizeof(demscreensettings));
		demscreensettings.dmSize = sizeof(demscreensettings);
		demscreensettings.dmPelsHeight = (unsigned long)wnddata.height;
		demscreensettings.dmPelsWidth = (unsigned long)wnddata.width;
		demscreensettings.dmBitsPerPel = kWindowColorBit;
		demscreensettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		//�E�B���h�E���t���X�N���[���ɐݒ�
		ChangeDisplaySettings(&demscreensettings, CDS_FULLSCREEN);

		//�\�����W��ݒ�
		x = y = 0;
	}
	else
	{
		wnddata.height = kWindow_Height;
		wnddata.width = kWindow_Width;

		x = (GetSystemMetrics(SM_CXSCREEN) - wnddata.width) / 2;
		y = (GetSystemMetrics(SM_CYSCREEN) - wnddata.height) / 2;
	}

	//�E�B���h�E���쐬
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

	//�E�B���h�E�Ƀt�H�[�J�X���Z�b�g
	ShowWindow(hwnd_, SW_SHOW);
	SetForegroundWindow(hwnd_);
	SetFocus(hwnd_);

#ifdef _DEBUG
	//�f�o�b�O���̂݃}�E�X�J�[�\����\��
	ShowCursor(true);
#else
	ShowCursor(false);
#endif // _DEBUG
}

void System::destroyWindows()
{
	//�}�E�X�J�[�\����\��
	ShowCursor(true);

	//�t���X�N���[���̏ꍇ�E�B���h�E���[�h�ɐ؂�ւ���
	if (kFullScreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	//�E�B���h�E��j��
	DestroyWindow(hwnd_);
	hwnd_ = NULL;

	//�A�v���P�[�V�����C���X�^���X��j��
	UnregisterClass("DirectX12", hinstance_);
	hinstance_ = NULL;

	//���̃N���X�ւ̃|�C���^��j��
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
		Singleton<Direct3D>::getPtr()->init(kScreenWidth, kScreenHeight, kVsync, kFullScreen, kScreen_depth, kScreen_near, L"test", L"test",hWnd);
		Singleton<ColorShader>::getPtr()->init();

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
		//beginpaint()�Ȃǂ�windows�̕`��@�\���Ă΂Ȃ���WM_PAINT���Ă΂ꑱ���鋓���𗘗p����
		Singleton<Direct3D>::getPtr()->update();
		Singleton<Direct3D>::getPtr()->begin();
		Singleton<ColorShader>::getPtr()->makeCommand();
		Singleton<Direct3D>::getPtr()->end();
	}
	default:
	{
		return systemhandle->MessageHandler(hWnd, uMessage, wParam, lParam);
	}
	}
}
