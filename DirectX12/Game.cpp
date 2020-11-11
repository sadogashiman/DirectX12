#include "stdafx.h"
#include "Game.h"
#include "ColorShader.h"
#include "Singleton.h"

bool Game::init()
{
	Singleton<ColorShader>::getPtr()->init();
	return true;
}

SceneBase* Game::update()
{
	return this;
}

bool Game::render()
{

	Singleton<ColorShader>::getPtr()->makeCommand();


	return true;
}

void Game::destroy()
{
	Singleton<ColorShader>::getPtr()->destroy();
}
