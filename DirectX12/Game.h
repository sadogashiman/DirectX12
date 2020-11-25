#pragma once
#include "SceneBase.h"

class Game :public SceneBase
{
private:
	float factor_;
public:
	bool init();
	SceneBase* update();
	bool render();
	void destroy();
};

