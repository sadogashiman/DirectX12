#pragma once
class Debug
{
private:
	std::ofstream ofs_;
	unsigned int errorcnt_;
public:
	void init();
	void destroy();
	void addErrorLog(std::string ErrorMessage);
};

