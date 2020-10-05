#pragma once
#include"InputKeycode.h"
struct ScreenSize
{
	int Width;
	int Height;
};

class Input
{
private:


	//入力情報読み取り
	void readKeyBoard();
	void readMouse();
	void readControllers();

	//マウスの移動制限
	void processInput();

	bool initKeyboard();
	bool initMouse();
	bool initControllers();

	//エラーが起きた時に解消する
	void recovery(LPDIRECTINPUTDEVICE8& Dev,HRESULT Hr);

	DIJOYSTATE padstate_;
	DIJOYSTATE padprevstate_;
	DIMOUSESTATE mousesstate_;
	unsigned char keystate_[256];
	unsigned char keyprevstate_[256];
	int mousex_, mousey_;
	ScreenSize screen_;
	std::chrono::high_resolution_clock::time_point starttime_;

	//入力情報保存用Vector
	std::vector<ComPtr< IDirectInputDevice8>>gamecontrollers_;
	LPDIRECTINPUTDEVICE8 keyboarddev_;
	LPDIRECTINPUTDEVICE8 mousedevice_;

	//メインデバイス
	ComPtr<IDirectInput8A> didev_;
	unsigned int currentactivecontroller_;
public:
	Input();
	~Input();

	bool init();
	void update();
	void destroy();

	BOOL EnumGameControllers(LPCDIDEVICEINSTANCE Devinst);

	static BOOL CALLBACK staticEnumGameControllers(LPCDIDEVICEINSTANCE Devinst, LPVOID Pvref);
	static BOOL CALLBACK staticSetGameControllerAxesRange(LPCDIDEVICEINSTANCE Devobjinst, LPVOID Pvref);

	//*******************************
	//		    Keyboard
	//*******************************

	//指定のキーの入力状態を返す
	const bool isKeyState(const unsigned int KeyCode)const;

	//特定のキーが押されたかを返す
	const bool isKeyPressed(const unsigned int KeyCode)const;

	//特定のキーが離されたかを返す
	const bool isKeyReleased(const unsigned int KeyCode)const;

	//特定のキーの入力が継続されているかを返す
	const bool isKeyHeld(const unsigned int KeyCode)const;

	//何らかのキーが入力されているかを返す
	const bool anyKeyDown()const;

	//何らかのキーの入力が終了したかを返す
	const bool anyKeyUp()const;

	//何らかキーの入力が継続されているかを返す
	const bool anyKeyHeld()const;


	//*******************************
	//		    GamePad
	//*******************************

	//指定のボタンの入力状態を返す
	const bool isBottonState(const unsigned int BottonCode)const;
	
	//指定のボタンの入力が開始されたかを返す
	const bool isBottonPressed(const unsigned int BottonCode)const;
	
	//指定のボタンの入力が終了されたかを返す
	const bool isBottonReleased(const unsigned int BottonCode)const;

	//指定のボタンの入力が継続されているかを返す
	const bool isBottonHeld(const unsigned int BottonCode)const;
	
	//何らかのボタンが入力されているかを返す
	const bool anyBottomDown()const;

	//何らかのボタンの入力が終了されたかを返す
	const bool anyBottomUp()const;

	//何らかのボタンの入力が継続されているかを返す
	const bool anyBottonHeld()const;

	//特定のPOVの入力状態を返す
	const bool isPOVState(const unsigned int POVBottonCode)const;

	//特定のPOVの入力が開始されたかを返す
	const bool isPOVPressed(const unsigned int POVBottonCode)const;

	//特定のPOVの入力が終了したかを返す
	const bool isPOVReleased(const unsigned int POVBottonCode)const;

	//特定のPOVの入力が継続されているかを返す
	const bool isPOVHeld(const unsigned int POVBottonCode)const;

	//何らかのPOVの入力が開始したかを返す
	const bool anyPadPOVDown()const;

	//何らかのPOVの入力が終了したかを返す
	const bool anyPadPOVUp()const;

	//何らかのPOVの入力が継続されているか返す
	const bool anyPadPOVHeld()const;

	//ゲーム終了キー
	const bool quitApp();

	static inline Input* getInstance()
	{
		static Input instance;
		return &instance;
	}

};

