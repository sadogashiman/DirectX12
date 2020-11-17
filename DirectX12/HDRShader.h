#pragma once
#include "ShaderBase.h"
#include "Direct3D.h"
#include "ModelData.h"

class HDRShader : public ShaderBase
{
private:
	void initModel();
	void renderModel();
	Direct3D* d3d_;
	ModelData::DataType model_;


public:
	HDRShader();
	~HDRShader();
	void init();
	void render();
	void destroy();
};

