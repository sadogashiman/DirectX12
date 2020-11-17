#pragma once
#include "ModelData.h"
class Model
{
private:
	ModelData::DataType model_;
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT3 normal;

		VertexType(float Px, float Py, float Pz, float Nx, float Ny, float Nz) :position(Px, Py, Pz), normal(Nx, Ny, Nz) {}
	};
public:
	Model();
	~Model();
	void init();
	void render();
};

