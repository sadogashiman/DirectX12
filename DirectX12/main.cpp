#include "System.h"
#include "Singleton.h"
#include "stdafx.h"
#include "lua_Helper.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pScmdline, int iCmdshow)
{
#ifdef _DEBUG
	//_CrtSetBreakAlloc(532);
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif // _DEBUG
	bool result;

	//システムをシングルトンで初期化
	result = Singleton<System>::getPtr()->init();
	if (result)
	{
		Singleton<System>::getPtr()->run();
	}

	//登録したシングルトンオブジェクトを逆順で解放
	SingletonFinalizer::finalize();

	return 0;
}