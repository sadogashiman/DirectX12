#include "stdafx.h"
#include "LuaSystem.h"

bool LuaSystem::init()
{
    return true;
}

void LuaSystem::destroy()
{
    //�ǉ������t����Lua���폜
    int size = luastatearray_.size() - 1;
    for (int i = size; i > 0; i--)
    {
        lua_close(luastatearray_[i]);
    }
}
