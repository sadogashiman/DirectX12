#include "System.h"
#include "Singleton.h"
#include "error.h"
#include "Direct3D.h"
#include "DirectXBase.h"

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

	return true;
}

void System::initWindows()
{
	WNDCLASSEX wc;
	DEVMODE demscreensettings;
	ScreenData wnddata;
	int x, y;

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
		return 0;
	}
	case WM_KEYUP:
	{
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
		//beginpaint()�Ȃǂ�windows�̕`��@�\���Ă΂Ȃ���WM_PAINT���Ă΂ꑱ���鋓���𗘗p����

	}
	default:
	{
		return systemhandle->MessageHandler(hWnd, uMessage, wParam, lParam);
	}
	}
}
