#pragma once
//******************************
//		　　 マクロ
//******************************
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN //使われていないAPIの一部を除外するマクロ
//#if _MSC_VER > 1922 && !defined(_SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING)
//#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING //VST基本ライブラリのコンパイルエラー回避マクロ
//#endif
#ifdef _DEBUG
#define D3DCOMPILE_DEBUG 1
#endif // _DEBUG

//******************************
//　　		include
//******************************
//WindowsAPI
#include <Windows.h>
#include <tchar.h>
#include <math.h>
#include <wrl/client.h>
#include <mmsystem.h>
#include <Shlwapi.h>
#include <cassert>
#include <shellapi.h>
#include <wrl/wrappers/corewrappers.h>

//DirectX関係
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12shader.h>
#include <d3dcompiler.h>
#include <dinput.h>
#include <DirectXMath.h>
#include <dxcapi.h>
#include "d3dx12.h"
#include "dxc\Support\ErrorCodes.h"
#include "DirectXTex.h"

//Lua関係
#include <lua.hpp>

//std関係
#include <fstream>
#include <vector>
#include <map>
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include <thread>
#include <mutex>
#include <string>
#include <filesystem>
#include <unordered_set>
#include <array>
#include <unordered_map>

//******************************
//　　		リンク
//******************************
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dxcompiler.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"shlwapi.lib")
#pragma comment(lib,"lua.lib")

//******************************
//　　		名前空間
//******************************
using namespace Microsoft::WRL;
using namespace DirectX;

//******************************
//　　		  定数
//******************************
const int kScreenWidth = 1280;
const int kScreenHeight = 720;
const int kWindow_Width = 1280;
const int kWindow_Height = 720;
const int kWindowColorBit = 32;
const int kExtensionTypeNum = 5;
const float kScreen_depth = 1000.0F;
const float kScreen_near = 0.1F;
const bool kFullScreen = false;
const bool kVsync = true;
const bool kTgs = false; //TGSブース展示用ボタン配置

//******************************
//　　		  列挙体
//******************************



//******************************
//　　		  構造体
//******************************


//******************************
//　　		  クラス
//******************************
