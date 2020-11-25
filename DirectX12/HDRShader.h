#pragma once
#include "Direct3D.h"
#include "ModelData.h"

class HDRShader
{
private:
	void initModel();
	void renderModel();
	Direct3D* d3d_;
	ModelData::DataType model_;
	struct SceneParameter
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 viewProj;
		DirectX::XMFLOAT4 lightPos;
		DirectX::XMFLOAT4 cameraPos;
		DirectX::XMFLOAT4 branchFrags;
	};

public:
	HDRShader();
	~HDRShader();
	void init();
	void render();
	void destroy();
};

