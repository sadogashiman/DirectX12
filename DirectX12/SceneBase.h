#pragma once
#include "Direct3D.h"
#include "DirectInput.h"
class SceneBase
{
protected:
	Direct3D* direct3d_;
	DirectInput* dinput_;

public:
	virtual bool init() = 0;
	virtual SceneBase* update() = 0;
	virtual bool render() = 0;
	virtual void destroy() = 0;

};