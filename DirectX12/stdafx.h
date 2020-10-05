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
#include<Windows.h>
#include<tchar.h>
#include<math.h>
#include<wrl/client.h>
#include<mmsystem.h>
#include<Shlwapi.h>
#include<cassert>

//DirectX
#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3d12shader.h>
#include <SimpleMath.h>
#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>
#include <dinput.h>

//std
#include<fstream>
#include<vector>
#include<map>
#include<chrono>
#include<iostream>
#include<random>
#include<algorithm>
#include<thread>
#include<mutex>

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
using namespace SimpleMath;

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
const bool kvsync = true;
const bool kTgs = false; //TGS�u�[�X�W���p�{�^���z�u

//******************************
//�@�@		  �񋓑�
//******************************



//******************************
//�@�@		  �\����
//******************************