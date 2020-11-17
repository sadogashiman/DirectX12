#pragma once
#include "ModelData.h"
class ShaderBase
{
protected:
	ModelData::DataType m_model;

public:
	struct SceneParameter
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 viewProj;
		DirectX::XMFLOAT4 lightPos;
		DirectX::XMFLOAT4 cameraPos;
		DirectX::XMFLOAT4 branchFrags;
	};

	ShaderBase() {};
	virtual ~ShaderBase() {};
	virtual void init() = 0;
	virtual void render() = 0;
	virtual void destroy() = 0;
};