#pragma once
//******************************
//		�@�@ �}�N��
//******************************
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN //�g���Ă��Ȃ�API�̈ꕔ�����O����}�N��
#ifdef _DEBUG
#define D3DCOMPILE_DEBUG 1
#endif // _DEBUG



//******************************
//�@�@		include
//******************************
#include <Windows.h>
#include <tchar.h>
#include <math.h>
#include <wrl/client.h>
#include <mmsystem.h>
#include <Shlwapi.h>
#include <cassert>
#include <shellapi.h>
#include <wrl/wrappers/corewrappers.h>

//DirectX
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3d12shader.h>
#include <dinput.h>
#include <DirectXMath.h>
#include "d3dx12.h"
#include <DirectXCollision.h>

//std
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

//******************************
//�@�@		�����N
//******************************
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxguid.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"dinput8.lib")
#pragma comment(lib,"shlwapi.lib")

//******************************
//�@�@		���O���
//******************************
using namespace Microsoft::WRL;
using namespace DirectX;

//******************************
//�@�@		  �萔
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
const bool kTgs = false; //TGS�u�[�X�W���p�{�^���z�u
const std::string kResourceRootPath = "Resource/";

//******************************
//�@�@		  �񋓑�
//******************************



//******************************
//�@�@		  �\����
//******************************
