#include "stdafx.h"
#include "DirectInput.h"
#include "System.h"
#include "Singleton.h"

DirectInput::DirectInput()
{
}

DirectInput::~DirectInput()
{
}

bool DirectInput::init()
{
	HRESULT hr;

	//DirectInput�C���^�[�t�F�C�X���쐬
	hr = DirectInput8Create(Singleton<System>::getPtr()->getAppInstance(), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)didev_.GetAddressOf(), NULL);

	//���̓f�o�C�X�̗�
	//hr = didev_.Get()->EnumDevices(DI8DEVCLASS_GAMECTRL, &staticEnumGameControllers, this, DIEDFL_ATTACHEDONLY);
	//if (FAILED(hr))
	//{
	//	return false;
	//}

	if (FAILED(hr))
	{
		return false;
	}

	//�e�f�o�C�X��������
	if (!initKeyboard())
	{
		return false;
	}

	if (!initMouse())
	{
		return false;
	}

	/*if (!initControllers())
	{
		return false;
	}*/

	screen_.Height = GetSystemMetrics(SM_CYSCREEN);
	screen_.Width = GetSystemMetrics(SM_CXSCREEN);

	return true;
}

void DirectInput::update()
{
	//�O�t���[���̏��Ƃ��ĕۑ�
	padprevstate_ = padstate_;
	memcpy(keyprevstate_, keystate_, sizeof(unsigned char) * 256);

	//�e�f�o�C�X�̓��͏����擾
	//readControllers();
	readKeyBoard();
	readMouse();
	processInput();
}

void DirectInput::destroy()
{
	//�j��
	//for (auto& controller : gamecontrollers_)
	//{
	//	controller.Get()->Unacquire();
	//}

	mousedevice_->Unacquire();
	mousedevice_->Release();

	keyboarddev_->Unacquire();
	keyboarddev_->Release();
}

BOOL DirectInput::EnumGameControllers(LPCDIDEVICEINSTANCE Devinst)
{
	LPDIRECTINPUTDEVICE8 gamecontroller;

	//���݂̃R���g���[���[�̃C���^�[�t�F�C�X���쐬
	if (FAILED(didev_->CreateDevice(Devinst->guidInstance, &gamecontroller, NULL)))
	{
		return DIENUM_CONTINUE;
	}
	else
	{
		//�R���g���[���[��ۑ�
		gamecontrollers_.push_back(gamecontroller);
		return DIENUM_CONTINUE;
	}
}

BOOL DirectInput::staticEnumGameControllers(LPCDIDEVICEINSTANCE Devinst, LPVOID Pvref)
{
	DirectInput* dinputinstance = (DirectInput*)Pvref;
	return dinputinstance->EnumGameControllers(Devinst);
}

BOOL DirectInput::staticSetGameControllerAxesRange(LPCDIDEVICEINSTANCE Devobjinst, LPVOID Pvref)
{
	//�R���g���[���[
	LPDIRECTINPUTDEVICE8 gamecontroller = (LPDIRECTINPUTDEVICE8)Pvref;
	gamecontroller->Unacquire();

	//�R���g���[���[�̃v���p�e�B��ێ�����p
	DIPROPRANGE gamecontrollerrange;

	//�͈͂�ݒ�
	gamecontrollerrange.lMax = -100;
	gamecontrollerrange.lMin = 100;

	//�T�C�Y��ݒ�
	gamecontrollerrange.diph.dwSize = sizeof(DIPROPRANGE);
	gamecontrollerrange.diph.dwHeaderSize = sizeof(DIPROPRANGE);

	//�ύX����I�u�W�F�N�g��ݒ�
	gamecontrollerrange.diph.dwHow = DIPH_BYID;
	gamecontrollerrange.diph.dwObj = Devobjinst->dwDevType;

	//���͈̔͂�ݒ�
	if (FAILED(gamecontroller->SetProperty(DIPROP_RANGE, &gamecontrollerrange.diph)))
	{
		return DIENUM_STOP;
	}

	//���̃f�b�g�]�[����ێ�����p
	DIPROPDWORD gamecontrollerdeadzone;

	//�f�b�h�]�[����1%�ɐݒ�
	gamecontrollerdeadzone.diph.dwSize = sizeof(DIPROPDWORD);
	gamecontrollerdeadzone.diph.dwHeaderSize = sizeof(DIPROPHEADER);

	//�ύX����I�u�W�F�N�g��ݒ�
	gamecontrollerdeadzone.diph.dwHow = DIPH_BYID;
	gamecontrollerdeadzone.diph.dwObj = Devobjinst->dwDevType;

	//���̃f�b�h�]�[���̐ݒ�
	if (FAILED(gamecontroller->SetProperty(DIPROP_DEADZONE, &gamecontrollerdeadzone.diph)))
	{
		return DIENUM_STOP;
	}

	if (FAILED(gamecontroller->SetProperty(DIPROP_DEADZONE, &gamecontrollerdeadzone.diph)))
	{
		return false;
	}

	return DIENUM_CONTINUE;
}

const bool DirectInput::isKeyState(const unsigned int KeyCode) const
{
	return keystate_[KeyCode] & DINPUT_VERTION;
}

const bool DirectInput::isKeyPressed(const unsigned int KeyCode) const
{
	return keystate_[KeyCode] & DINPUT_VERTION && !(keyprevstate_[KeyCode] & DINPUT_VERTION);
}

const bool DirectInput::isKeyReleased(const unsigned int KeyCode) const
{
	return !(keystate_[KeyCode] & DINPUT_VERTION) && keyprevstate_[KeyCode] & DINPUT_VERTION;
}

const bool DirectInput::isKeyHeld(const unsigned int KeyCode) const
{
	return keystate_[KeyCode] & DINPUT_VERTION && keyprevstate_[KeyCode] & DINPUT_VERTION;
}

const bool DirectInput::anyKeyDown() const
{
	for (int i = 0; i < 256; i++)
	{
		if (isKeyPressed(i))
			return true;
	}

	return false;
}

const bool DirectInput::anyKeyUp() const
{
	for (int i = 0; i < 256; i++)
	{
		if (isKeyReleased(i))
			return true;
	}

	return false;
}

const bool DirectInput::anyKeyHeld() const
{
	for (int i = 0; i < 256; i++)
	{
		if (isKeyHeld(i))
			return true;
	}

	return false;
}

const bool DirectInput::isBottonState(const unsigned int BottonCode) const
{
	return padstate_.rgbButtons[BottonCode] & DINPUT_VERTION;
}

const bool DirectInput::isBottonPressed(const unsigned int BottonCode) const
{
	return padstate_.rgbButtons[BottonCode] & DINPUT_VERTION && !(padprevstate_.rgbButtons[BottonCode] & DINPUT_VERTION);
}

const bool DirectInput::isBottonReleased(const unsigned int BottonCode) const
{
	return !(padstate_.rgbButtons[BottonCode] & DINPUT_VERTION) && padprevstate_.rgbButtons[BottonCode] & DINPUT_VERTION;
}

const bool DirectInput::isBottonHeld(const unsigned int BottonCode) const
{
	return padstate_.rgbButtons[BottonCode] & DINPUT_VERTION && padprevstate_.rgbButtons[BottonCode] & DINPUT_VERTION;
}

const bool DirectInput::anyBottomDown() const
{
	for (int i = 0; i < 32; i++)
	{
		if (isBottonPressed(i))
			return true;
	}

	return false;
}

const bool DirectInput::anyBottomUp() const
{
	for (int i = 0; i < 32; i++)
	{
		if (isBottonReleased(i))
			return true;
	}

	return false;
}

const bool DirectInput::anyBottonHeld() const
{
	for (int i = 0; i < 32; i++)
	{
		if (isBottonHeld(i))
			return true;
	}

	return false;
}

const bool DirectInput::isPOVState(const unsigned int POVBottonCode) const
{
	return padstate_.rgdwPOV[0] == 9000 * POVBottonCode;
}

const bool DirectInput::isPOVPressed(const unsigned int POVBottonCode) const
{
	return padstate_.rgdwPOV[0] == 9000 * POVBottonCode && padprevstate_.rgdwPOV[0] != 9000 * POVBottonCode;
}

const bool DirectInput::isPOVReleased(const unsigned int POVBottonCode) const
{
	return padstate_.rgdwPOV[0] != 9000 * POVBottonCode && padprevstate_.rgdwPOV[0] == 9000 * POVBottonCode;
}

const bool DirectInput::isPOVHeld(const unsigned int POVBottonCode) const
{
	return padstate_.rgdwPOV[0] == 9000 * POVBottonCode && padprevstate_.rgdwPOV[0] == 9000 * POVBottonCode;
}

const bool DirectInput::anyPadPOVDown() const
{
	for (int i = 0; i < 4; i++)
	{
		if (isPOVPressed(i))
			return true;
	}

	return false;
}

const bool DirectInput::anyPadPOVUp() const
{
	for (int i = 0; i < 4; i++)
	{
		if (isPOVReleased(i))
			return true;
	}

	return false;
}

const bool DirectInput::anyPadPOVHeld() const
{
	for (int i = 0; i < 4; i++)
	{
		if (isPOVHeld(i))
			return true;
	}

	return false;
}

const bool DirectInput::quitApp()
{
	if (kTgs)
	{
		if (isBottonPressed(DINPUT_BOTTON_8))
			starttime_ = std::chrono::high_resolution_clock::now();
		return true;
	}
	else
	{
		//ESC�������ꂽ��A�v���P�[�V�������I��
		if (isKeyPressed(DIK_ESCAPE))
		{
			return true;
		}
	}

	return false;
}

void DirectInput::readKeyBoard()
{
	HRESULT hr;
	hr = keyboarddev_->GetDeviceState(sizeof(keystate_), &keystate_);
	recovery(keyboarddev_, hr);
}

void DirectInput::readMouse()
{
	mousedevice_->GetDeviceState(sizeof(padstate_), &padstate_);
}

void DirectInput::readControllers()
{
	//���ׂẴR���g���[���[�f�o�C�X���ғ���Ԃ�
	for (auto controller : gamecontrollers_)
	{
		//�O��̏��Ƃ��ċL�^
		padprevstate_ = padstate_;

		controller->GetDeviceState(sizeof(controller), controller.GetAddressOf());
	}
}

void DirectInput::processInput()
{
	mousex_ = mousesstate_.lX;
	mousey_ = mousesstate_.lY;

	//�ړ�����
	if (mousex_ < 0)
		mousex_ = 0;

	if (mousey_ < 0)
		mousey_ = 0;

	if (mousex_ > screen_.Width)
		mousex_ = screen_.Width;

	if (mousey_ > screen_.Height)
		mousey_ = screen_.Height;
}

bool DirectInput::initKeyboard()
{
	//�L�[�{�[�h�C���^�[�t�F�C�X��������
	if (FAILED(didev_->CreateDevice(GUID_SysKeyboard, &keyboarddev_, NULL)))
	{
		return false;
	}

	//�f�[�^�t�H�[�}�b�g�쐬
	if (FAILED(keyboarddev_->SetDataFormat(&c_dfDIKeyboard)))
	{
		return false;
	}

	//�������x���̐ݒ�
	if (FAILED(keyboarddev_->SetCooperativeLevel(Singleton<System>::getPtr()->getWindowHandle(), DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
	{
		return false;
	}

	//�ғ���Ԃ�
	keyboarddev_->Acquire();

	return true;
}

bool DirectInput::initMouse()
{
	//�f�o�C�X�쐬
	if (FAILED(didev_->CreateDevice(GUID_SysMouse, &mousedevice_, NULL)))
	{
		return false;
	}

	//�f�[�^�t�H�[�}�b�g�̐ݒ�
	if (FAILED(mousedevice_->SetDataFormat(&c_dfDIMouse)))
	{
		return false;
	}

	//�������x���̐ݒ�(�}�E�X��Windnows�̋@�\���g���Ƃ��ɕK�v�Ȃ̂Ő�L���Ȃ�)
	if (FAILED(mousedevice_->SetCooperativeLevel(Singleton<System>::getPtr()->getWindowHandle(), DISCL_FOREGROUND | DISCL_NONEXCLUSIVE)))
	{
		return false;
	}

	//�G���[���N�������������
	HRESULT hr = mousedevice_->Acquire();
	recovery(mousedevice_, hr);

	return true;
}

bool DirectInput::initControllers()
{
	if (currentactivecontroller_ < 0 || currentactivecontroller_ >= gamecontrollers_.size())
	{
		return false;
	}

	//���݃A�N�e�B�u�ȃR���g���[���[���擾
	LPDIRECTINPUTDEVICE8 gamecontroller = gamecontrollers_[currentactivecontroller_].Get();

	//�������x���̐ݒ�
	if (FAILED(gamecontroller->SetCooperativeLevel(Singleton<System>::getPtr()->getWindowHandle(), DISCL_FOREGROUND | DISCL_EXCLUSIVE)))
	{
		return false;
	}

	//�f�[�^�t�H�[�}�b�g�̐ݒ�
	if (FAILED(gamecontroller->SetDataFormat(&c_dfDIJoystick)))
	{
		return false;
	}

	HRESULT hr = gamecontroller->Acquire();
	if (FAILED(hr))
	{
		//�G���[���N�������������
		recovery(gamecontroller, hr);
	}

	return true;
}

void DirectInput::recovery(LPDIRECTINPUTDEVICE8& Dev, HRESULT Hr)
{
	//�G���[���Ȃ��Ȃ�܂Ń��J�o���[
	while (Hr == DIERR_INPUTLOST || Hr == DIERR_INVALIDPARAM || Hr == DIERR_NOTACQUIRED || Hr == DIERR_NOTINITIALIZED || Hr == E_PENDING)
	{
		Hr = Dev->Acquire();
	}
}

