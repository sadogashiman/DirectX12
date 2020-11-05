#pragma once
#include "SceneBase.h"

class Game :public SceneBase
{
private:
public:
	bool init();
	SceneBase* update();
	bool render();
	void destroy();
};

