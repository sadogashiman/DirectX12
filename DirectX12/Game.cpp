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
	Singleton<Direct3D>::getPtr()->begin();

	Singleton<ColorShader>::getPtr()->makeCommand();

	Singleton<Direct3D>::getPtr()->end();

	return true;
}

void Game::destroy()
{
	Singleton<ColorShader>::getPtr()->destroy();
}
