#include "stdafx.h"
#include "Game.h"
#include "ColorShader.h"
#include "Singleton.h"
#include "HDRShader.h"

bool Game::init()
{
	return true;
}

SceneBase* Game::update()
{
	return this;
}

bool Game::render()
{


	return true;
}

void Game::destroy()
{
}
