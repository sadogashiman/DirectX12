#pragma once
class LuaSystem
{
private:
	std::vector<lua_State*> luastatearray_;

public:
	bool init();
	void destroy();
	inline void addLuaState(lua_State* LuaState) { luastatearray_.push_back(LuaState); } //�n���ꂽLuaState��z��̖����ɒǉ�

};

